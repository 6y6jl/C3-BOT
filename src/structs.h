#pragma once
#include <Arduino.h>

// Имя робота
#define ROBOT_NAME "ROBOT"
#define ROBOT_VERSION "v1.0"

// Типы экранов
enum ScreenType {
    SCREEN_ROBOT_FACE,
    SCREEN_TIME,
    SCREEN_WEATHER,
    SCREEN_CURRENCY,
    NUM_SCREENS
};

// Константы - определяем в .cpp
extern const char* SCREEN_NAMES[NUM_SCREENS];

struct Config {
    // Локация
    String city = "Gomel";
    float latitude = 52.4345;
    float longitude = 30.9754;
    String timezone = "Europe/Minsk";
    bool auto_detect = false;
    
    // Настройки отображения
    String time_format = "24";
    bool date_display = true;
    String temp_unit = "C";
    bool round_temps = true;
    
    // Валюты
    String currency_base = "usd";
    String currency_target = "byn";
    int currency_multiplier = 1;
    
    // Режимы
    bool screen_auto_cycle = true;
    int screen_interval_sec = 10;
    int screen_order[NUM_SCREENS] = {0, 1, 2, 3};
    
    bool show_face = true;
    bool show_time = true;
    bool show_weather = true;
    bool show_currency = true;
    
    // Анимации
    uint16_t anim_mask = 62;
    
    // Ночной режим
    bool night_mode = true;
    String night_start = "22:00";
    String night_end = "06:00";
    int night_action = 1;
    
    // Служебные
    String device_id = "";
    String ip_address = "";
};

struct WeatherData {
    float temp = 0;
    float apparent_temperature = 0;
    float wind_speed = 0;
    int humidity = 0;
    int weather_code = 0;
    bool is_day = true;
    String update_time = "N/A";
    bool valid = false;
};

struct CurrencyData {
    String base = "USD";
    String target = "BYN";
    float rate = 0;
    String date = "";
    bool updated = false;
};

struct AppState {
    Config config;
    WeatherData weather;
    CurrencyData currency;
    int currentScreen = SCREEN_ROBOT_FACE;
    int previousScreen = SCREEN_ROBOT_FACE;
    unsigned long lastScreenChange = 0;
    unsigned long lastWeatherUpdate = 0;
    unsigned long lastCurrencyUpdate = 0;
};