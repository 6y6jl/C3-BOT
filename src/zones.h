#pragma once
#include <ArduinoJson.h>

const char POSIX_TIMEZONE_MAP[] PROGMEM = R"({
    "Europe/Minsk": "EET-2EEST",
    "Europe/Moscow": "MSK-3",
    "Europe/Kyiv": "EET-2EEST",
    "Europe/Warsaw": "CET-1CEST",
    "Europe/London": "GMT0BST",
    "America/New_York": "EST5EDT",
    "America/Chicago": "CST6CDT",
    "America/Denver": "MST7MDT",
    "America/Los_Angeles": "PST8PDT",
    "Asia/Tokyo": "JST-9",
    "Asia/Shanghai": "CST-8"
})";