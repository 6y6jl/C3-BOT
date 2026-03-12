#pragma once
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

// Добавляем inline, чтобы не было ошибок при сборке проекта
inline char bot_token[64] = ""; // Токен обычно ~46 символов, берем с запасом
inline char chat_id[20] = "";
inline bool shouldSaveConfig = false;

void saveConfigCallback() { 
    shouldSaveConfig = true; 
}

void loadConfig() {
    // true в begin(true) автоматически форматирует память при первом запуске
    if (LittleFS.begin(true)) {
        if (LittleFS.exists("/config.json")) {
            File f = LittleFS.open("/config.json", "r");
            if (f) {
                StaticJsonDocument<256> doc;
                DeserializationError error = deserializeJson(doc, f);
                if (!error) {
                    // Используем безопасное копирование strncpy
                    strncpy(bot_token, doc["token"] | "", sizeof(bot_token));
                    strncpy(chat_id, doc["id"] | "", sizeof(chat_id));
                }
                f.close();
                Serial.println("Конфигурация загружена из памяти.");
            }
        }
    } else {
        Serial.println("Ошибка монтирования LittleFS!");
    }
}

void saveConfig() {
    File f = LittleFS.open("/config.json", "w");
    if (f) {
        StaticJsonDocument<256> doc;
        doc["token"] = bot_token;
        doc["id"] = chat_id;
        if (serializeJson(doc, f) == 0) {
            Serial.println("Ошибка записи в файл!");
        }
        f.close();
        Serial.println("Настройки сохранены в LittleFS.");
    }
}
