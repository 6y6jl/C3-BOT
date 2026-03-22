#include "DisplayService.h"
#include "images.h"
#include <Irisoled.h>
#include <ArduinoJson.h>

// Определение SCREEN_NAMES
const char* SCREEN_NAMES[NUM_SCREENS] = {
    "Robot Face",
    "Time & Date",
    "Weather",
    "Currency"
};

// Массив выражений робота
const unsigned char* robotExpressions[12] = {
    Irisoled::normal,     // 0 - NORMAL
    Irisoled::happy,      // 1 - HAPPY
    Irisoled::sad,        // 2 - SAD
    Irisoled::angry,      // 3 - ANGRY
    Irisoled::surprised,  // 4 - SURPRISED
    Irisoled::excited,    // 5 - EXCITED
    Irisoled::alert,      // 6 - ALERT
    Irisoled::sleepy,     // 7 - SLEEPY
    Irisoled::wink_left,  // 8 - WINK
    Irisoled::scared,     // 9 - SCARED
    Irisoled::bored,      // 10 - BORED
    Irisoled::happy       // 11 - LOVE
};

DisplayService::DisplayService(int width, int height, int reset_pin) : 
    display(width, height, &Wire, reset_pin) {
    currentMood = 0;
    currentLookDirection = 0;
    targetLookDirection = 0;
    lastLookChange = 0;
    lastInteractionTime = 0;
    moodStartTime = 0;
    isSleeping = false;
    isBlinking = false;
    lastBlink = 0;
    
    Serial.println("DisplayService constructor");
}

void DisplayService::begin() {
    Serial.println("DisplayService::begin() - trying to init display...");
    
    // Попытка инициализации с адресом 0x3C
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println("Failed with 0x3C, trying 0x3D...");
        // Попытка с адресом 0x3D
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
            Serial.println("Display allocation failed on both addresses!");
            return;
        }
    }
    
    Serial.println("Display initialized successfully!");
    display.clearDisplay();
    display.display();
}

void DisplayService::showOLEDStatus(std::initializer_list<String> lines, bool clear) {
    if (clear) display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    
    int cursorY = 0;
    for (const String& line : lines) {
        int xStart = (display.width() - line.length() * 6) / 2;
        display.setCursor(xStart, cursorY);
        display.print(line);
        cursorY += 10;
        if (cursorY >= display.height()) break;
    }
    display.display();
}

// ========== МЕТОДЫ РОБОТА ==========

const unsigned char* DisplayService::getLookExpressionForDirection(int direction) {
    switch(direction) {
        case 1: return Irisoled::look_up;
        case 2: return Irisoled::look_down;
        case 3: return Irisoled::look_left;
        case 4: return Irisoled::look_right;
        default: return Irisoled::normal;
    }
}

void DisplayService::smoothMoveToTarget() {
    if (currentLookDirection == targetLookDirection) return;
    
    int step = (targetLookDirection > currentLookDirection) ? 1 : -1;
    
    for (int i = 0; i < 8; i++) {
        float progress = (float)(i + 1) / 8;
        int tempDirection = currentLookDirection + (int)(step * progress + 0.5);
        if (tempDirection < 0) tempDirection = 0;
        if (tempDirection > 4) tempDirection = 4;
        
        display.clearDisplay();
        display.drawBitmap(0, 0, getLookExpressionForDirection(tempDirection), 128, 64, WHITE);
        display.display();
        delay(40);
    }
    currentLookDirection = targetLookDirection;
}

void DisplayService::drawRobotFace() {
    display.clearDisplay();
    
    if (currentMood == 0 && !isBlinking) {
        display.drawBitmap(0, 0, getLookExpressionForDirection(currentLookDirection), 128, 64, WHITE);
    } else if (isBlinking) {
        display.drawBitmap(0, 0, Irisoled::blink, 128, 64, WHITE);
    } else {
        display.drawBitmap(0, 0, robotExpressions[currentMood], 128, 64, WHITE);
    }
    
    display.display();
}

void DisplayService::updateRobotFace() {
    unsigned long now = millis();
    
    if (currentMood != 0 && currentMood != 7) {
        if (now - moodStartTime > TEMP_MOOD_TIMEOUT) {
            currentMood = 0;
            drawRobotFace();
            return;
        }
    }
    
    if (currentMood == 0 && !isSleeping && now - lastInteractionTime > SLEEP_TIMEOUT) {
        isSleeping = true;
        currentMood = 7;
        drawRobotFace();
        return;
    }
    
    if (isSleeping && now - lastInteractionTime < SLEEP_TIMEOUT) {
        isSleeping = false;
        currentMood = 0;
        drawRobotFace();
        return;
    }
    
    if (currentMood == 0 && !isBlinking && now - lastBlink > (unsigned long)random(BLINK_MIN, BLINK_MAX)) {
        isBlinking = true;
        drawRobotFace();
        lastBlink = now;
    }
    
    if (isBlinking && now - lastBlink > 150) {
        isBlinking = false;
        drawRobotFace();
    }
    
    if (currentMood == 0 && now - lastLookChange > (unsigned long)random(LOOK_MIN, LOOK_MAX)) {
        targetLookDirection = random(0, 5);
        if (targetLookDirection != currentLookDirection) {
            smoothMoveToTarget();
        }
        lastLookChange = now;
    }
}

void DisplayService::setRobotMood(int mood) {
    if (mood >= 0 && mood < 12) {
        currentMood = mood;
        moodStartTime = millis();
        if (mood != 7) {
            isSleeping = false;
            lastInteractionTime = millis();
        }
        drawRobotFace();
    }
}

void DisplayService::handleRobotTouch(unsigned long duration) {
    lastInteractionTime = millis();
    
    if (isSleeping) {
        isSleeping = false;
        currentMood = 0;
    }
    
    if (duration < 3000) {
        int randomMood = random(1, 12);
        if (randomMood == 7) randomMood = 1;
        setRobotMood(randomMood);
    } else {
        setRobotMood(1);
    }
}

void DisplayService::onRobotTelegramMessage() {
    lastInteractionTime = millis();
    setRobotMood(6);
}

void DisplayService::onRobotPCOn() {
    lastInteractionTime = millis();
    setRobotMood(4);
    delay(500);
    setRobotMood(1);
    delay(800);
    setRobotMood(0);
}

void DisplayService::onRobotWiFiSuccess() {
    lastInteractionTime = millis();
    setRobotMood(1);
    delay(500);
    setRobotMood(0);
}

void DisplayService::onRobotWiFiFail() {
    lastInteractionTime = millis();
    setRobotMood(2);
    delay(1000);
    setRobotMood(0);
}

// ========== ЭКРАННЫЕ МЕТОДЫ ==========

void DisplayService::drawRobotFaceScreen() {
    drawRobotFace();
}

void DisplayService::drawTimeScreen(const Config& config, String timeStr, String dateStr) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(3);
    display.setCursor((128 - timeStr.length() * 18) / 2, 15);
    display.print(timeStr);

    if (config.date_display) {
        display.setTextSize(1);
        display.setCursor((128 - dateStr.length() * 6) / 2, 50);
        display.print(dateStr);
    }
    
    display.display();
}

String DisplayService::getWeatherDescription(int wmo_code) {
    if (wmo_code == 0) return "Clear";
    if (wmo_code >= 1 && wmo_code <= 3) return "Cloudy";
    if (wmo_code >= 45 && wmo_code <= 48) return "Fog";
    if (wmo_code >= 51 && wmo_code <= 67) return "Rain";
    if (wmo_code >= 71 && wmo_code <= 77) return "Snow";
    if (wmo_code >= 95) return "Thunder";
    return "Unknown";
}

const unsigned char* DisplayService::getWeatherBitmap(int wmo_code, bool is_day) {
    if (wmo_code == 0) return is_day ? icon_sun : icon_moon;
    if (wmo_code >= 1 && wmo_code <= 3) return is_day ? icon_cloud : icon_cloud_moon;
    if (wmo_code >= 45 && wmo_code <= 48) return icon_fog;
    if (wmo_code >= 51 && wmo_code <= 67) return icon_rain;
    if (wmo_code >= 71 && wmo_code <= 77) return icon_snow;
    if (wmo_code >= 95) return icon_thunder;
    return icon_cloud;
}

void DisplayService::drawWeatherScreen(const Config& config, const WeatherData& data, const String& currentTime) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    bool valid = data.valid;
    
    // Город и время
    display.setTextSize(1);
    display.setCursor(2, 2);
    display.print(valid ? config.city : "No Location");
    
    display.setCursor(display.width() - currentTime.length() * 6 - 2, 2);
    display.print(currentTime);
    
    display.drawFastHLine(0, 12, display.width(), SSD1306_WHITE);
    
    // Температура
    display.setTextSize(3);
    String tempStr = valid ? (config.round_temps ? String((int)round(data.temp)) : String(data.temp, 1)) : "--";
    display.setCursor(5, 25);
    display.print(tempStr);
    display.cp437(true);
    display.write(248);
    display.print("C");
    
    // Иконка погоды
    const unsigned char* icon = valid ? getWeatherBitmap(data.weather_code, data.is_day) : icon_cloud;
    display.drawBitmap(display.width() - 28, 20, icon, 24, 24, SSD1306_WHITE);
    
    // Описание
    display.setTextSize(1);
    String desc = valid ? getWeatherDescription(data.weather_code) : "No Data";
    display.setCursor(display.width() - desc.length() * 6 - 2, 48);
    display.print(desc);
    
    // Влажность и ветер
    display.setCursor(5, 55);
    display.print("Hum: ");
    display.print(valid ? String(data.humidity) : "--");
    display.print("%  Wind: ");
    display.print(valid ? String((int)round(data.wind_speed)) : "--");
    display.print("km");
    
    display.display();
}

void DisplayService::drawCurrencyScreen(const Config& config, const CurrencyData& data) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Заголовок
    display.setTextSize(1);
    display.setCursor((128 - 40) / 2, 2);
    display.print("EXCHANGE RATE");
    
    display.drawFastHLine(0, 12, display.width(), SSD1306_WHITE);
    
    // Основной курс
    display.setTextSize(3);
    String rateStr = String(data.rate, 3);
    display.setCursor((128 - rateStr.length() * 18) / 2, 30);
    display.print(rateStr);
    
    // Дата обновления
    display.setTextSize(1);
    String dateStr = data.date;
    display.setCursor((128 - dateStr.length() * 6) / 2, 55);
    display.print(dateStr);
    
    display.display();
}

// ========== ОБЩИЕ МЕТОДЫ ==========

bool DisplayService::isScreenEnabled(const Config& config, int screenIndex) {
    switch (screenIndex) {
        case SCREEN_ROBOT_FACE: return config.show_face;
        case SCREEN_TIME: return config.show_time;
        case SCREEN_WEATHER: return config.show_weather;
        case SCREEN_CURRENCY: return config.show_currency;
        default: return false;
    }
}

void DisplayService::drawScreen(int screenIndex, const AppState& state, TimeService& timeService) {
    switch(screenIndex) {
        case SCREEN_ROBOT_FACE:
            drawRobotFaceScreen();
            break;
        case SCREEN_TIME:
            drawTimeScreen(state.config, timeService.getCurrentTimeShort(state.config.time_format), timeService.getFullDate());
            break;
        case SCREEN_WEATHER:
            drawWeatherScreen(state.config, state.weather, timeService.getCurrentTimeShort(state.config.time_format));
            break;
        case SCREEN_CURRENCY:
            drawCurrencyScreen(state.config, state.currency);
            break;
    }
}

int DisplayService::getNextAnimationEffect(uint16_t mask) {
    int enabledAnims[10];
    int count = 0;
    for (int i = 1; i <= 5; i++) {
        if (mask & (1 << i)) enabledAnims[count++] = i;
    }
    if (count == 0) return 0;
    return enabledAnims[random(0, count)];
}

// Анимации
void DisplayService::animateHorizontal(int prev, int next, const AppState& state, TimeService& t) {
    display.clearDisplay(); drawScreen(prev, state, t); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, t); memcpy(screenBufferNew, display.getBuffer(), 1024);
    for (int offset = 0; offset <= 128; offset += 8) {
        uint8_t* buf = display.getBuffer();
        for (int page = 0; page < 8; page++) {
            int start = page * 128;
            if (offset < 128) memcpy(&buf[start], &screenBufferOld[start + offset], 128 - offset);
            if (offset > 0) memcpy(&buf[start + (128 - offset)], &screenBufferNew[start], offset);
        }
        display.display();
    }
}

void DisplayService::animateVertical(int prev, int next, const AppState& state, TimeService& t) {
    display.clearDisplay(); drawScreen(prev, state, t); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, t); memcpy(screenBufferNew, display.getBuffer(), 1024);
    for (int step = 0; step <= 8; step++) {
        uint8_t* buf = display.getBuffer();
        for (int page = 0; page < 8; page++) {
            int oldIdx = page + step;
            int newIdx = page - (8 - step);
            if (oldIdx < 8) memcpy(&buf[page * 128], &screenBufferOld[oldIdx * 128], 128);
            else if (newIdx >= 0) memcpy(&buf[page * 128], &screenBufferNew[newIdx * 128], 128);
        }
        display.display();
        delay(10);
    }
}

void DisplayService::animateDissolve(int prev, int next, const AppState& state, TimeService& t) {
    display.clearDisplay(); drawScreen(prev, state, t); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, t); memcpy(screenBufferNew, display.getBuffer(), 1024);
    uint8_t masks[] = {0x80, 0xC0, 0xE0, 0xE4, 0xF4, 0xFC, 0xFE, 0xFF};
    for (int step = 0; step < 8; step++) {
        uint8_t* buf = display.getBuffer();
        for (int i = 0; i < 1024; i++) {
            buf[i] = (screenBufferNew[i] & masks[step]) | (screenBufferOld[i] & ~masks[step]);
        }
        display.display();
        delay(10);
    }
}

void DisplayService::animateCurtain(int prev, int next, const AppState& state, TimeService& t) {
    display.clearDisplay(); drawScreen(prev, state, t); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, t); memcpy(screenBufferNew, display.getBuffer(), 1024);
    for (int r = 0; r <= 80; r += 4) {
        int startX = 64 - r; if (startX < 0) startX = 0;
        int endX = 64 + r; if (endX > 128) endX = 128;
        uint8_t* buf = display.getBuffer();
        for (int x = 0; x < 128; x++) {
            bool inside = (x >= startX && x < endX);
            for (int page = 0; page < 8; page++) {
                int idx = x + (page * 128);
                buf[idx] = inside ? screenBufferNew[idx] : screenBufferOld[idx];
            }
        }
        display.display();
    }
}

void DisplayService::animateBlinds(int prev, int next, const AppState& state, TimeService& t) {
    display.clearDisplay(); drawScreen(prev, state, t); memcpy(screenBufferOld, display.getBuffer(), 1024);
    display.clearDisplay(); drawScreen(next, state, t); memcpy(screenBufferNew, display.getBuffer(), 1024);
    uint8_t* buf = display.getBuffer();
    memcpy(buf, screenBufferOld, 1024);
    display.display();
    int blindWidth = 16, numBlinds = 8;
    for (int progress = 0; progress < blindWidth; progress += 2) {
        for (int blind = 0; blind < numBlinds; blind++) {
            int startX = blind * blindWidth;
            for (int i = 0; i < 2; i++) {
                int x = startX + progress + i;
                if (x >= 128) continue;
                for (int page = 0; page < 8; page++) {
                    buf[x + (page * 128)] = screenBufferNew[x + (page * 128)];
                }
            }
        }
        display.display();
        delay(5);
    }
}

void DisplayService::animateTransition(int prevScreen, int nextScreen, const AppState& state, TimeService& timeService) {
    int effect = getNextAnimationEffect(state.config.anim_mask);
    switch(effect) {
        case 1: animateHorizontal(prevScreen, nextScreen, state, timeService); break;
        case 2: animateVertical(prevScreen, nextScreen, state, timeService); break;
        case 3: animateDissolve(prevScreen, nextScreen, state, timeService); break;
        case 4: animateCurtain(prevScreen, nextScreen, state, timeService); break;
        case 5: animateBlinds(prevScreen, nextScreen, state, timeService); break;
        default:
            display.clearDisplay();
            drawScreen(nextScreen, state, timeService);
            display.display();
            break;
    }
}