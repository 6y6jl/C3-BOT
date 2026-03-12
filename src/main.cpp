#include "Arduino.h"
#include "config_handler.h"
#include "pc_control.h"
#include "bot_logic.h"

#define RESET_PIN 2

void checkResetButton() {
    // Для TTP223 нажатие — это HIGH (1)
    if (digitalRead(RESET_PIN) == HIGH) {
        unsigned long startPress = millis();
        Serial.println("\n[ВНИМАНИЕ] Сенсор зажат! Держите 3 сек для сброса...");
        
        while (digitalRead(RESET_PIN) == HIGH) {
            if (millis() - startPress >= 3000) {
                Serial.println("[СБРОС] Очистка памяти и Wi-Fi...");
                WiFiManager wm;
                wm.resetSettings(); 
                LittleFS.format();   
                Serial.println("Готово! Перезагрузка...");
                delay(1000);
                ESP.restart();
            }
            delay(10); 
        }
        Serial.println("[ОТМЕНА] Сенсор отпущен слишком рано.");
    }
}

void setup() {
    Serial.begin(115200);
    setupHardware();
    loadConfig();

    pinMode(RESET_PIN, INPUT); // Важно для TTP223

    WiFiManager wm;
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(180); 

    WiFiManagerParameter p_token("t", "Telegram Token", bot_token, 50);
    WiFiManagerParameter p_id("i", "Your Chat ID", chat_id, 20);
    wm.addParameter(&p_token);
    wm.addParameter(&p_id);

    // Подключаемся к WiFi
    if (!wm.autoConnect("ESP32_PC_CONTROLLER")) {
        Serial.println("Таймаут настройки. Рестарт...");
        delay(3000);
        ESP.restart();
    }

    // Принудительно закрываем точку доступа
    WiFi.softAPdisconnect(true); 
    WiFi.mode(WIFI_STA); 

    // Сохранение данных из веб-интерфейса
    if (shouldSaveConfig) {
        strncpy(bot_token, p_token.getValue(), 50);
        strncpy(chat_id, p_id.getValue(), 20);
        saveConfig();
        Serial.println("Настройки сохранены.");
    }

    // Отладка: проверяем, что в памяти
    Serial.print("Текущий токен: "); Serial.println(bot_token);
    Serial.print("Текущий ID: "); Serial.println(chat_id);

    // Инициализация бота
    bot.setToken(bot_token);
    bot.attach(handleMessages);
    
    if (strlen(chat_id) > 1) {
        bot.sendMessage("⚡ Система управления запущена", chat_id);
    }
    
    Serial.println("Система онлайн!");
}

void loop() {
    bot.tick();
    checkResetButton();
}
