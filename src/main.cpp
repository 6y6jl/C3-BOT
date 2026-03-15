#include "Arduino.h"
#include "config_handler.h"
#include "bot_logic.h"
#include "mochi_robot.h"

MochiRobot mochi;
bool wifiConnected = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n=== MOCHI ROBOT PC CONTROLLER ===");
    
    // Инициализация робота
    if (!mochi.begin()) {
        Serial.println("OLED не найден!");
    } else {
        mochi.showWelcome();
    }
    
    // Загрузка конфигурации
    loadConfig();
    
    // Пытаемся подключиться к WiFi
    mochi.showMessage("Connecting...", "to WiFi");
    
    WiFiManager wm;
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(180);
    
    WiFiManagerParameter p_token("t", "Telegram Token", bot_token, 50);
    WiFiManagerParameter p_id("i", "Your Chat ID", chat_id, 20);
    wm.addParameter(&p_token);
    wm.addParameter(&p_id);
    
    if (wm.autoConnect("MOCHI_ROBOT")) {
        wifiConnected = true;
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        
        mochi.showWiFiStatus(WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        mochi.onWiFiSuccess();
        
        if (shouldSaveConfig) {
            strncpy(bot_token, p_token.getValue(), 50);
            strncpy(chat_id, p_id.getValue(), 20);
            saveConfig();
            mochi.showMessage("Config Saved!", "Token & ID");
            delay(1000);
        }
        
        bot.setToken(bot_token);
        bot.attach(handleMessages);
        
        if (strlen(chat_id) > 1) {
            bot.sendMessage("🤖 MOCHI ROBOT запущен!\nСтатус ПК контроллера онлайн", chat_id);
        }
        
        mochi.showMessage("MOCHI Ready!", "Online");
        
    } else {
        wifiConnected = false;
        
        mochi.showMessage("⚠️ WiFi Failed!", "AP Mode Active");
        mochi.onWiFiFail();
        delay(2000);
        
        mochi.showMessage("Connect to AP:", "MOCHI_ROBOT");
        delay(2000);
        
        mochi.showMessage("Configure at:", "192.168.4.1");
        // Заменяем MOOD_SURPRISED на MOOD_EXCITED
        mochi.setMood(MOOD_EXCITED);
        
        Serial.println("WiFi Failed! AP Mode: MOCHI_ROBOT");
        Serial.println("Connect and visit 192.168.4.1 to configure");
    }
    
    Serial.println("MOCHI ROBOT запущен!");
}

void loop() {
    // Проверка WiFi статуса
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        wifiConnected = false;
        mochi.onWiFiFail();
        mochi.showMessage("⚠️ WiFi Lost!", "Check connection");
    } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
        wifiConnected = true;
        mochi.onWiFiSuccess();
        mochi.showWiFiStatus(WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    }
    
    // Логика бота
    if (wifiConnected) {
        bot.tick();
    }
    
    // Обновление лица (автоматические движения)
    mochi.update();
    
    // Проверка сенсорной кнопки с измерением длительности
    static unsigned long pressStart = 0;
    static bool buttonPressed = false;
    
    if (digitalRead(TOUCH_PIN) == HIGH && !buttonPressed) {
        buttonPressed = true;
        pressStart = millis();
    }
    
    if (digitalRead(TOUCH_PIN) == LOW && buttonPressed) {
        buttonPressed = false;
        unsigned long pressDuration = millis() - pressStart;
        
        // Передаем длительность нажатия в робота (вместо nextMood)
        mochi.handleTouch(pressDuration);
        
        delay(200); // Антидребезг
    }
}