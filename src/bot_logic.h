#pragma once
#include <FastBot.h>
#include "pc_control.h"
#include "mochi_robot.h"

extern char chat_id[];
extern MochiRobot mochi; // Добавляем доступ к роботу

FastBot bot;

String getBoardInfo() {
    String info = "🟢 *СИСТЕМА ОНЛАЙН*\n";
    info += "═══════════════════════\n\n";
    
    info += "📟 *ESP32-C3*\n";
    info += "▸ Частота: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "▸ Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n";
    info += "▸ SDK: " + String(ESP.getSdkVersion()) + "\n\n";
    
    info += "💾 *Память*\n";
    info += "▸ Свободно RAM: " + String(ESP.getFreeHeap() / 1024) + " KB\n";
    info += "▸ Минимум RAM: " + String(ESP.getMinFreeHeap() / 1024) + " KB\n\n";
    
    info += "📶 *Wi-Fi*\n";
    if (WiFi.status() == WL_CONNECTED) {
        info += "▸ Статус: ✅ Подключен\n";
        info += "▸ Сеть: " + WiFi.SSID() + "\n";
        info += "▸ IP: " + WiFi.localIP().toString() + "\n";
        info += "▸ Сигнал: " + String(WiFi.RSSI()) + " dBm\n\n";
    } else {
        info += "▸ Статус: ❌ Не подключен\n\n";
    }
    
    unsigned long uptime = millis() / 1000;
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;
    
    info += "⏱️ *Время работы*\n";
    info += "▸ ";
    if (days > 0) info += String(days) + "д ";
    if (hours > 0) info += String(hours) + "ч ";
    if (minutes > 0) info += String(minutes) + "м ";
    info += String(seconds) + "с\n\n";
    
    #ifdef ESP32
    float temp = temperatureRead();
    if (temp > 5 && temp < 100) {
        info += "🌡️ *Температура*\n";
        info += "▸ Чип: " + String(temp, 1) + "°C\n\n";
    }
    #endif
    
    info += "═══════════════════════\n";
    info += "📱 PC Controller v1.0";
    
    return info;
}

void handleMessages(FB_msg& msg) {
    if (String(msg.userID) != String(chat_id)) return;

    // Показываем ALERT при любом сообщении в Telegram
    mochi.onTelegramMessage();
    
    if (msg.text == "/on") {
        pressPowerButton();
        bot.sendMessage("⚡ Сигнал на включение подан!", msg.userID);
        mochi.onPCOn(); // Анимация включения ПК
    }
    
    if (msg.text == "/status") {
        bot.sendMessage(getBoardInfo(), msg.userID);
    }
}