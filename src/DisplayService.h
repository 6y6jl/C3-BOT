#pragma once
#include "structs.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "TimeService.h"

class DisplayService {
public:
    Adafruit_SSD1306 display;

    DisplayService(int width, int height, int reset_pin);
    void begin();
    
    void showOLEDStatus(std::initializer_list<String> lines, bool clear = true);
    
    // Экранные методы
    void drawRobotFaceScreen();
    void drawTimeScreen(const Config& config, String timeStr, String dateStr);
    void drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime);
    void drawCurrencyScreen(const Config& config, const CurrencyData& data);
    
    void drawScreen(int screenIndex, const AppState& state, TimeService& timeService);
    void animateTransition(int prevScreen, int nextScreen, const AppState& state, TimeService& timeService);
    
    // Анимация для прямого вызова
    void animateCurtain(int prev, int next, const AppState& state, TimeService& t);  // Переносим в public!

    bool isScreenEnabled(const Config& config, int screenIndex);
    
    // Методы для управления лицом робота
    void updateRobotFace();
    void setRobotMood(int mood);
    void handleRobotTouch(unsigned long duration);
    void onRobotTelegramMessage();
    void onRobotPCOn();
    void onRobotWiFiSuccess();
    void onRobotWiFiFail();
    
private:    
    uint8_t screenBufferOld[1024];
    uint8_t screenBufferNew[1024];

    int getNextAnimationEffect(uint16_t mask);
    void animateHorizontal(int prev, int next, const AppState& state, TimeService& t);
    void animateVertical(int prev, int next, const AppState& state, TimeService& t);
    void animateDissolve(int prev, int next, const AppState& state, TimeService& t);
    void animateBlinds(int prev, int next, const AppState& state, TimeService& t);

    String getWeatherDescription(int wmo_code);
    const unsigned char* getWeatherBitmap(int wmo_code, bool is_day);
    
    // Состояние робота
    int currentMood;
    int currentLookDirection;
    int targetLookDirection;
    unsigned long lastLookChange;
    unsigned long lastInteractionTime;
    unsigned long moodStartTime;
    bool isSleeping;
    bool isBlinking;
    unsigned long lastBlink;
    
    const int SLEEP_TIMEOUT = 1800000;
    const int TEMP_MOOD_TIMEOUT = 3000;
    const int LOOK_MIN = 5000;
    const int LOOK_MAX = 12000;
    const int BLINK_MIN = 3000;
    const int BLINK_MAX = 8000;
    
    const unsigned char* getLookExpressionForDirection(int direction);
    void smoothMoveToTarget();
    void drawRobotFace();
};