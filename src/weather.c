#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "weather.h"
#include <curl/curl.h>

#define API_KEY "your-openweather-api-key-here" // Replace with your OpenWeatherMap API key

typedef struct MemoryStruct
{
    char *response;
    size_t size;
} MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        // out of memory!
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

int get_weather_data(const char *city, WeatherData *weather)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.response = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl)
    {
        // URL-encode city to handle spaces and special characters
        size_t city_len = strlen(city);
        size_t enc_size = city_len * 3 + 1; // worst-case every char encoded as %XX
        char *encoded_city = malloc(enc_size);
        if (!encoded_city)
        {
            fprintf(stderr, "Failed to allocate memory for encoded city\n");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            free(chunk.response);
            return -1;
        }

        // simple URL-encode (percent-encode non-alnum characters)
        size_t ei = 0;
        for (size_t i = 0; i < city_len; ++i)
        {
            unsigned char c = (unsigned char)city[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encoded_city[ei++] = c;
            }
            else
            {
                // percent-encode
                if (ei + 3 >= enc_size) break; // safety
                snprintf(&encoded_city[ei], 4, "%%%02X", c);
                ei += 3;
            }
        }
        encoded_city[ei] = '\0';

        char url[512];
        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric", encoded_city, API_KEY);

        free(encoded_city);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    if (res != CURLE_OK)
    {
        free(chunk.response);
        return -1;
    }

    // Print the raw JSON response
    // printf("Raw JSON Response: %s\n", chunk.response);

    cJSON *root = cJSON_Parse(chunk.response);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJSON Error before: %s\n", error_ptr);
        }
        free(chunk.response);
        return -1;
    }

    cJSON *main = cJSON_GetObjectItemCaseSensitive(root, "main");
    if (cJSON_IsObject(main))
    {
        cJSON *temp = cJSON_GetObjectItemCaseSensitive(main, "temp");
        cJSON *humidity = cJSON_GetObjectItemCaseSensitive(main, "humidity");
        cJSON *pressure = cJSON_GetObjectItemCaseSensitive(main, "pressure");
        cJSON *temp_min = cJSON_GetObjectItemCaseSensitive(main, "temp_min");
        cJSON *temp_max = cJSON_GetObjectItemCaseSensitive(main, "temp_max");
        cJSON *feels_like = cJSON_GetObjectItemCaseSensitive(main, "feels_like");

        if (cJSON_IsNumber(temp) && cJSON_IsNumber(humidity) && cJSON_IsNumber(pressure) && cJSON_IsNumber(temp_min) && cJSON_IsNumber(temp_max) && cJSON_IsNumber(feels_like))
        {
            weather->temperature = temp->valuedouble;
            weather->humidity = humidity->valueint;
            weather->pressure = pressure->valueint;
            weather->temp_min = temp_min->valuedouble;
            weather->temp_max = temp_max->valuedouble;
            weather->feels_like_temp = feels_like->valuedouble;
        }
        else
        {
            cJSON_Delete(root);
            free(chunk.response);
            return -1;
        }
    }
    else
    {
        cJSON_Delete(root);
        free(chunk.response);
        return -1;
    }

    /* initialize wind defaults */
    weather->wind_speed = 0.0f;
    weather->wind_deg = 0.0f;
    weather->wind_gust = 0.0f;

    cJSON *wind = cJSON_GetObjectItemCaseSensitive(root, "wind");
    if (cJSON_IsObject(wind))
    {
        cJSON *speed = cJSON_GetObjectItemCaseSensitive(wind, "speed");
        cJSON *deg = cJSON_GetObjectItemCaseSensitive(wind, "deg");
        cJSON *gust = cJSON_GetObjectItemCaseSensitive(wind, "gust");

        if (cJSON_IsNumber(speed))
        {
            weather->wind_speed = speed->valuedouble;
        }
        if (cJSON_IsNumber(deg))
        {
            weather->wind_deg = deg->valuedouble;
        }
        if (cJSON_IsNumber(gust))
        {
            weather->wind_gust = gust->valuedouble;
        }
        /* don't treat missing wind fields as fatal */
    }

    /* default cloudiness */
    weather->cloudiness = 0.0f;
    cJSON *clouds = cJSON_GetObjectItemCaseSensitive(root, "clouds");
    if (cJSON_IsObject(clouds))
    {
        cJSON *all = cJSON_GetObjectItemCaseSensitive(clouds, "all");
        if (cJSON_IsNumber(all))
        {
            weather->cloudiness = all->valuedouble;
        }
        /* non-numeric or missing 'all' is not fatal */
    }

    /* weather array: main and description are helpful, id and icon optional */
    weather->main = NULL;
    weather->description = NULL;
    weather->icon = NULL;
    weather->id = 0;

    cJSON *weather_array = cJSON_GetObjectItemCaseSensitive(root, "weather");
    if (cJSON_IsArray(weather_array))
    {
        cJSON *weather_item = cJSON_GetArrayItem(weather_array, 0);
        if (cJSON_IsObject(weather_item))
        {
            cJSON *main = cJSON_GetObjectItemCaseSensitive(weather_item, "main");
            cJSON *description = cJSON_GetObjectItemCaseSensitive(weather_item, "description");
            cJSON *id = cJSON_GetObjectItemCaseSensitive(weather_item, "id");
            cJSON *icon = cJSON_GetObjectItemCaseSensitive(weather_item, "icon");

            if (cJSON_IsString(main))
            {
                weather->main = strdup(main->valuestring);
            }
            if (cJSON_IsString(description))
            {
                weather->description = strdup(description->valuestring);
            }
            if (cJSON_IsNumber(id))
            {
                weather->id = id->valueint;
            }
            if (cJSON_IsString(icon))
            {
                weather->icon = strdup(icon->valuestring);
            }
            /* allow missing optional fields; main/description may still be NULL */
        }
        /* if weather_item isn't an object, continue with defaults */
    }

    /* default sunrise/sunset to NULL */
    /* Convert sunrise/sunset from UNIX timestamp to human-readable local time */
    weather->sunrise = NULL;
    weather->sunset = NULL;

    cJSON *sys = cJSON_GetObjectItemCaseSensitive(root, "sys");
    cJSON *timezone_obj = cJSON_GetObjectItemCaseSensitive(root, "timezone");

    if (cJSON_IsObject(sys))
    {
        cJSON *sunrise = cJSON_GetObjectItemCaseSensitive(sys, "sunrise");
        cJSON *sunset = cJSON_GetObjectItemCaseSensitive(sys, "sunset");

        int timezone_offset = 0;
        if (cJSON_IsNumber(timezone_obj))
        {
            timezone_offset = timezone_obj->valueint; // seconds offset from UTC
        }

        if (cJSON_IsNumber(sunrise))
        {
            time_t sunrise_time = sunrise->valueint + timezone_offset;
            struct tm *timeinfo = gmtime(&sunrise_time);
            weather->sunrise = malloc(32);
            strftime(weather->sunrise, 32, "%Y-%m-%d %I:%M:%S %p", timeinfo);
        }

        if (cJSON_IsNumber(sunset))
        {
            time_t sunset_time = sunset->valueint + timezone_offset;
            struct tm *timeinfo = gmtime(&sunset_time);
            weather->sunset = malloc(32);
            strftime(weather->sunset, 32, "%Y-%m-%d %I:%M:%S %p", timeinfo);
        }
    }

    cJSON_Delete(root);
    free(chunk.response);

    return 0;
}

int get_geolocation(char *ip, char *city){
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.response = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl)
    {
        char url[256];
        snprintf(url, sizeof(url), "http://ip-api.com/json/%s", ip);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    if (res != CURLE_OK)
    {
        free(chunk.response);
        return -1;
    }

    cJSON *root = cJSON_Parse(chunk.response);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJSON Error before: %s\n", error_ptr);
        }
        free(chunk.response);
        return -1;
    }

    cJSON *city_json = cJSON_GetObjectItemCaseSensitive(root, "city");
    if (cJSON_IsString(city_json) && (city_json->valuestring != NULL))
    {
        strcpy(city, city_json->valuestring);
    }
    else
    {
        cJSON_Delete(root);
        free(chunk.response);
        return -1;
    }

    cJSON_Delete(root);
    free(chunk.response);

    return 0;
}