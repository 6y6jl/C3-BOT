#pragma once
#include <FastBot.h>
#include "pc_control.h"
#include "DisplayService.h"

extern char chat_id[];
extern DisplayService display;

FastBot bot;

String getBoardInfo() {
    String info = "ROBOT ONLINE\n";
    info += "═══════════════════════\n\n";
    
    info += "📟 *ESP32-C3*\n";
    info += "▸ Частота: " + String(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "▸ Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB\n";
    info += "▸ SDK: " + String(ESP.getSdkVersion()) + "\n\n";
    
    info += "💾 *Память*\n";
    info += "▸ Свободно RAM: " + String(ESP.getFreeHeap() / 1024) + " KB\n\n";
    
    info += "📶 *Wi-Fi*\n";
    if (WiFi.status() == WL_CONNECTED) {
        info += "▸ Статус: ✅ Подключен\n";
        info += "▸ Сеть: " + WiFi.SSID() + "\n";
        info += "▸ IP: " + WiFi.localIP().toString() + "\n";
    } else {
        info += "▸ Статус: ❌ Не подключен\n";
    }
    
    return info;
}

void handleMessages(FB_msg& msg) {
    if (String(msg.userID) != String(chat_id)) return;
    
    display.onRobotTelegramMessage();
    
    if (msg.text == "/on") {
        pressPowerButton();
        bot.sendMessage("⚡ Сигнал на включение подан!", msg.userID);
        display.onRobotPCOn();
    }
    
    if (msg.text == "/status") {
        bot.sendMessage(getBoardInfo(), msg.userID);
    }
    
    if (msg.text == "/happy") {
        display.setRobotMood(1);
        bot.sendMessage("😊 ROBOT счастлив!", msg.userID);
    }
}