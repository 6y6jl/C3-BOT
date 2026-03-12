#pragma once
#include <FastBot.h>
#include "pc_control.h"

extern char chat_id[]; // Ссылка на переменную из конфига
FastBot bot;

void handleMessages(FB_msg& msg) {
    if (String(msg.userID) != String(chat_id)) return;

    if (msg.text == "/on") {
        pressPowerButton();
        bot.sendMessage("Сигнал на включение подан ⚡", msg.userID);
    }
    
    if (msg.text == "/status") {
        bot.sendMessage("Система онлайн 🟢", msg.userID);
    }
}
