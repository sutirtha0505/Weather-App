#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "weather.h"

// --- Helper callback to collect CURL response ---
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t total_size = size * nmemb;
    strncat((char *)userp, (char *)contents, total_size);
    return total_size;
}

// --- Function to get Public IP Address ---
int get_public_ip(char *public_ip, size_t size)
{
    const char *apis[] = {
        "https://ifconfig.me/ip",
        "https://checkip.amazonaws.com/",
        "https://api.ipify.org/",
        NULL};

    CURL *curl;
    CURLcode res;
    int success = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl)
    {
        fprintf(stderr, "Failed to initialize CURL\n");
        curl_global_cleanup();
        return -1;
    }

    for (int i = 0; apis[i] != NULL; i++)
    {
        memset(public_ip, 0, size);
        printf("Trying %s ...\n", apis[i]);

        curl_easy_setopt(curl, CURLOPT_URL, apis[i]);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, public_ip);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK && strlen(public_ip) > 0)
        {
            success = 1;
            break;
        }

        fprintf(stderr, "Failed with %s: %s\n", apis[i], curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (!success)
    {
        fprintf(stderr, "All IP services failed\n");
        return -1;
    }

    public_ip[strcspn(public_ip, "\r\n")] = '\0'; // Trim newline
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "locate") == 0)
    {
        char public_ip[64] = {0};

        // ✅ Step 1: Get public IP
        if (get_public_ip(public_ip, sizeof(public_ip)) != 0)
        {
            fprintf(stderr, "Failed to retrieve public IP\n");
            return 1;
        }

        printf("Public IP: %s\n", public_ip);

        // ✅ Step 2: Get geolocation and nearest city
        char city[64] = {0};
        if (get_geolocation(public_ip, city) == 0)
        {
            printf("Nearest City: %s\n", city);

            // ✅ Step 3: Show the weather of that city
            WeatherData weather;
            if (get_weather_data(city, &weather) == 0)
            {
                printf("\nWeather in %s:\n", city);
                printf("Overall Weather: %s\n", weather.main);
                printf("Description: %s\n", weather.description);
                printf("Temperature: %.2f°C\n", weather.temperature);
                printf("Humidity: %d%%\n", weather.humidity);
                printf("Pressure: %d hPa\n", weather.pressure);
                printf("Feels Like: %.2f°C\n", weather.feels_like_temp);
                printf("Wind Speed: %.2f m/s\n", weather.wind_speed);
                printf("Wind Direction: %.2f°\n", weather.wind_deg);
                printf("Wind Gust: %.2f m/s\n", weather.wind_gust);
                printf("Cloudiness: %.2f%%\n", weather.cloudiness);
                printf("Today's Sunrise at: %s\n", weather.sunrise);
                printf("Today's Sunset at: %s\n", weather.sunset);
                free(weather.main);
                free(weather.description);
                free(weather.icon);
                free(weather.sunrise);
                free(weather.sunset);
            }
        }
        else
        {
            fprintf(stderr, "Failed to get geolocation data for IP %s\n", public_ip);
            return 1;
        }
    }
    else if (argc >= 2)
    {
        // If user passed exactly one arg and it's "locate" it was handled earlier.
        // Otherwise, join all argv[1]..argv[argc-1] into a single city name allowing spaces.
        size_t total_len = 0;
        for (int i = 1; i < argc; ++i)
            total_len += strlen(argv[i]) + 1; // space or null

        char *city = malloc(total_len);
        if (!city)
        {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }

        city[0] = '\0';
        for (int i = 1; i < argc; ++i)
        {
            strcat(city, argv[i]);
            if (i < argc - 1)
                strcat(city, " ");
        }

        WeatherData weather;

        if (get_weather_data(city, &weather) == 0)
        {
            printf("Weather in %s:\n", city);
            printf("Overall Weather: %s\n", weather.main);
            printf("Description: %s\n", weather.description);
            printf("Temperature: %.2f°C\n", weather.temperature);
            printf("Humidity: %d%%\n", weather.humidity);
            printf("Pressure: %d hPa\n", weather.pressure);
            printf("Min Temp: %.2f°C\n", weather.temp_min);
            printf("Max Temp: %.2f°C\n", weather.temp_max);
            printf("Feels Like: %.2f°C\n", weather.feels_like_temp);
            printf("Wind Speed: %.2f m/s\n", weather.wind_speed);
            printf("Wind Direction: %.2f°\n", weather.wind_deg);
            printf("Wind Gust: %.2f m/s\n", weather.wind_gust);
            printf("Cloudiness: %.2f%%\n", weather.cloudiness);
            printf("Sunrise: %s\n", weather.sunrise);
            printf("Sunset: %s\n", weather.sunset);

            free(weather.main);
            free(weather.description);
            free(weather.icon);
            free(weather.sunrise);
            free(weather.sunset);
        }
        else
        {
            fprintf(stderr, "Failed to get weather data for %s\n", city);
            free(city);
            return 1;
        }

        free(city);
    }
    else
    {
        fprintf(stderr, "Usage: %s <city> or %s locate\n", argv[0], argv[0]);
        return 1;
    }

    return 0;
}
