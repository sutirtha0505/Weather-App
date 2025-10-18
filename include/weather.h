#ifndef WEATHER_H
#define WEATHER_H

typedef struct {
    float temperature;
    int humidity;
    int pressure;
    float temp_min;
    float temp_max;
    float feels_like_temp;
    float wind_speed;
    float wind_deg;
    float wind_gust;
    float cloudiness;
    char *main; // Weather condition
    char *description; // Detailed weather description

    int id; // New field for weather condition ID
    char *icon; // New field for weather icon code
    char *sunrise; // New field for sunrise time
    char *sunset; // New field for sunset time

} WeatherData;

int get_weather_data(const char *city, WeatherData *weather);
//Getting the Geolocation data
int get_geolocation(char *ip, char *city);

#endif // WEATHER_H
