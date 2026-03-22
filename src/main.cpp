#include "Arduino.h"
#include "config_handler.h"
#include "bot_logic.h"
#include "DisplayService.h"
#include "TimeService.h"
#include "WeatherService.h"
#include "CurrencyService.h"
#include "structs.h"

#define TOUCH_PIN 2
#define CONTRAST_NORMAL 255
#define CONTRAST_DIM 30

// Глобальные объекты
DisplayService display(128, 64, -1);
TimeService timeService;
WeatherService weatherService;
CurrencyService currencyService;
AppState state;

bool wifiConnected = false;
unsigned long lastTempScreenTime = 0;
bool showingTempScreen = false;
const unsigned long TEMP_SCREEN_DURATION = 5000;
bool nightModeActive = false;
unsigned long lastNightCheck = 0;

void updateWeather() {
    String updateTime = timeService.getCurrentTimeShort(state.config.time_format);
    if (weatherService.fetchWeather(state.config, state.weather, updateTime)) {
        state.lastWeatherUpdate = millis();
    }
}

void updateCurrency() {
    if (currencyService.fetchRate(state.config.currency_base, state.config.currency_target, state.currency)) {
        state.lastCurrencyUpdate = millis();
    }
}

void updateAllData() {
    updateWeather();
    updateCurrency();
}

int getNextEnabledScreen(int current) {
    for (int i = 1; i <= NUM_SCREENS; i++) {
        int next = (current + i) % NUM_SCREENS;
        if (display.isScreenEnabled(state.config, next)) {
            return next;
        }
    }
    return current;
}

void showTempTime() {
    display.drawTimeScreen(state.config, 
        timeService.getCurrentTimeShort(state.config.time_format), 
        timeService.getFullDate());
    showingTempScreen = true;
    lastTempScreenTime = millis();
}

void hideTempScreen() {
    if (showingTempScreen) {
        showingTempScreen = false;
        display.drawScreen(state.currentScreen, state, timeService);
    }
}

// Проверка ночного режима
bool isNightModeActive() {
    if (!state.config.night_mode) return false;
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    int currentMinutes = timeinfo->tm_hour * 60 + timeinfo->tm_min;
    
    int startH = state.config.night_start.substring(0, 2).toInt();
    int startM = state.config.night_start.substring(3, 5).toInt();
    int startMinutes = startH * 60 + startM;
    
    int endH = state.config.night_end.substring(0, 2).toInt();
    int endM = state.config.night_end.substring(3, 5).toInt();
    int endMinutes = endH * 60 + endM;
    
    if (startMinutes < endMinutes) {
        return (currentMinutes >= startMinutes && currentMinutes < endMinutes);
    } else {
        return (currentMinutes >= startMinutes || currentMinutes < endMinutes);
    }
}

// Установка яркости
void setBrightness(int level) {
    display.display.ssd1306_command(SSD1306_SETCONTRAST);
    display.display.ssd1306_command(level);
    Serial.printf("Brightness set to: %d\n", level);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== ROBOT PC CONTROLLER ===");
    
    display.begin();
    delay(500);
    
    loadConfig();
    
    if (state.config.latitude == 0) {
        state.config.latitude = 52.4345;
        state.config.longitude = 30.9754;
        state.config.city = "Gomel";
        state.config.timezone = "Europe/Minsk";
    }
    
    display.showOLEDStatus({"Connecting...", "to WiFi"});
    
    WiFiManager wm;
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(180);
    
    WiFiManagerParameter p_token("t", "Telegram Token", bot_token, 50);
    WiFiManagerParameter p_id("i", "Your Chat ID", chat_id, 20);
    wm.addParameter(&p_token);
    wm.addParameter(&p_id);
    
    if (wm.autoConnect("ROBOT")) {
        wifiConnected = true;
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        
        timeService.syncNTP(state.config.timezone);
        updateAllData();
        
        if (shouldSaveConfig) {
            strncpy(bot_token, p_token.getValue(), 50);
            strncpy(chat_id, p_id.getValue(), 20);
            saveConfig();
        }
        
        bot.setToken(bot_token);
        bot.attach(handleMessages);
        
        if (strlen(chat_id) > 1) {
            bot.sendMessage("ROBOT запущен!\nСтатус ПК контроллера онлайн", chat_id);
        }
        
        display.onRobotWiFiSuccess();
        
    } else {
        wifiConnected = false;
        display.showOLEDStatus({"WiFi Failed!", "AP: ROBOT", "192.168.4.1"});
        display.onRobotWiFiFail();
    }
    
    state.currentScreen = SCREEN_ROBOT_FACE;
    display.drawScreen(state.currentScreen, state, timeService);
    
    // Применяем ночной режим при старте
    nightModeActive = isNightModeActive();
    if (nightModeActive) {
        setBrightness(CONTRAST_DIM);
        Serial.println("Night mode: Dim display");
    } else {
        setBrightness(CONTRAST_NORMAL);
    }
    
    Serial.println("ROBOT запущен!");
}

void loop() {
    unsigned long now = millis();
    
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        wifiConnected = false;
        display.onRobotWiFiFail();
    } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
        wifiConnected = true;
        display.onRobotWiFiSuccess();
        updateAllData();
    }
    
    if (wifiConnected) {
        bot.tick();
    }
    
    // Проверка ночного режима (каждую минуту)
    if (now - lastNightCheck > 60000) {
        bool nightNow = isNightModeActive();
        if (nightNow != nightModeActive) {
            nightModeActive = nightNow;
            if (nightModeActive) {
                setBrightness(CONTRAST_DIM);
                Serial.println("Night mode: Dim display");
            } else {
                setBrightness(CONTRAST_NORMAL);
                Serial.println("Day mode: Normal brightness");
                display.drawScreen(state.currentScreen, state, timeService);
            }
        }
        lastNightCheck = now;
    }
    
    if (!showingTempScreen && state.currentScreen == SCREEN_ROBOT_FACE) {
        display.updateRobotFace();
    }
    
    // Автоматический показ времени в 00 и 30 минут
    if (!showingTempScreen && wifiConnected) {
        time_t nowTime = time(nullptr);
        struct tm* timeinfo = localtime(&nowTime);
        int minutes = timeinfo->tm_min;
        
        static int lastShownMinute = -1;
        if ((minutes == 0 || minutes == 30) && lastShownMinute != minutes) {
            showTempTime();
            lastShownMinute = minutes;
        }
    }
    
    if (showingTempScreen && now - lastTempScreenTime > TEMP_SCREEN_DURATION) {
        hideTempScreen();
    }
    
    if (wifiConnected && now - state.lastWeatherUpdate > 1800000) {
        updateWeather();
        if (!showingTempScreen && state.currentScreen == SCREEN_WEATHER) {
            display.drawScreen(state.currentScreen, state, timeService);
        }
    }
    
    if (wifiConnected && now - state.lastCurrencyUpdate > 1800000) {
        updateCurrency();
        if (!showingTempScreen && state.currentScreen == SCREEN_CURRENCY) {
            display.drawScreen(state.currentScreen, state, timeService);
        }
    }
    
    static unsigned long lastClockUpdate = 0;
    if (now - lastClockUpdate > 1000) {
        if (showingTempScreen) {
            display.drawTimeScreen(state.config, 
                timeService.getCurrentTimeShort(state.config.time_format), 
                timeService.getFullDate());
        }
        lastClockUpdate = now;
    }
    
    // Обработка кнопки
    static unsigned long pressStart = 0;
    static bool buttonPressed = false;
    
    if (digitalRead(TOUCH_PIN) == HIGH && !buttonPressed) {
        buttonPressed = true;
        pressStart = millis();
    }
    
    if (digitalRead(TOUCH_PIN) == LOW && buttonPressed) {
        buttonPressed = false;
        unsigned long duration = millis() - pressStart;
        
        if (showingTempScreen) {
            hideTempScreen();
        } 
        else if (duration < 1500 && state.currentScreen == SCREEN_ROBOT_FACE) {
            display.handleRobotTouch(duration);
        }
        else if (duration >= 1500) {
            state.previousScreen = state.currentScreen;
            state.currentScreen = getNextEnabledScreen(state.currentScreen);
            display.animateTransition(state.previousScreen, state.currentScreen, state, timeService);
        }
        
        delay(200);
    }
}