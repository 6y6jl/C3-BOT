#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Irisoled.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
#define TOUCH_PIN 2

// Имя робота
#define ROBOT_NAME "ROBOT"
#define ROBOT_VERSION "v1.0"

// Настроения для лица
enum RobotMood {
    MOOD_NORMAL,        // обычное (базовое)
    MOOD_HAPPY,         // счастливое (для поглаживаний)
    MOOD_SAD,           // грустное
    MOOD_ANGRY,         // злое
    MOOD_SURPRISED,     // удивленное
    MOOD_EXCITED,       // взволнованное
    MOOD_ALERT,         // тревога/внимание (для сообщений)
    MOOD_SLEEPY,        // сонное
    MOOD_WINK,          // подмигивание
    MOOD_SCARED,        // испуганное
    MOOD_BORED,         // скучающее
    MOOD_LOVE,          // влюбленное
    MOOD_COUNT
};

class MochiRobot {
private:
    Adafruit_SSD1306 display;
    RobotMood currentMood;
    
    // Для автоматического поведения
    unsigned long lastLookChange;
    unsigned long lastInteractionTime;
    unsigned long moodStartTime;
    int currentLookDirection;
    int targetLookDirection;
    bool isSleeping;
    
    // Константы
    const int SLEEP_TIMEOUT = 1800000;      // 30 минут
    const int TEMP_MOOD_TIMEOUT = 3000;     // 3 секунды
    const int LOOK_MIN = 5000;               // 5 секунд
    const int LOOK_MAX = 12000;              // 12 секунд
    const int MOVE_STEPS = 8;                 // Количество шагов для плавного движения
    const int STEP_DELAY = 40;                 // Задержка между шагами (мс)
    
    // Массив всех выражений
    const unsigned char* expressions[MOOD_COUNT] = {
        Irisoled::normal,        // MOOD_NORMAL
        Irisoled::happy,         // MOOD_HAPPY
        Irisoled::sad,           // MOOD_SAD
        Irisoled::angry,         // MOOD_ANGRY
        Irisoled::surprised,     // MOOD_SURPRISED
        Irisoled::excited,       // MOOD_EXCITED
        Irisoled::alert,         // MOOD_ALERT
        Irisoled::sleepy,        // MOOD_SLEEPY
        Irisoled::wink_left,     // MOOD_WINK
        Irisoled::scared,        // MOOD_SCARED
        Irisoled::bored,         // MOOD_BORED
        Irisoled::happy          // MOOD_LOVE (пока happy)
    };
    
    const unsigned char* getLookExpression() {
        switch(currentLookDirection) {
            case 1: return Irisoled::look_up;
            case 2: return Irisoled::look_down;
            case 3: return Irisoled::look_left;
            case 4: return Irisoled::look_right;
            default: return Irisoled::normal;
        }
    }
    
    // Плавное движение глаз
    void smoothMoveToTarget() {
        if (currentLookDirection == targetLookDirection) return;
        
        int step = (targetLookDirection > currentLookDirection) ? 1 : -1;
        
        for (int i = 0; i < MOVE_STEPS; i++) {
            // Плавно меняем направление
            float progress = (float)(i + 1) / MOVE_STEPS;
            int tempDirection = currentLookDirection + (int)(step * progress + 0.5);
            
            // Ограничиваем диапазон
            if (tempDirection < 0) tempDirection = 0;
            if (tempDirection > 4) tempDirection = 4;
            
            display.clearDisplay();
            display.drawBitmap(0, 0, getLookExpressionForDirection(tempDirection), 128, 64, WHITE);
            display.display();
            delay(STEP_DELAY);
        }
        
        currentLookDirection = targetLookDirection;
    }
    
    // Получить выражение для направления (без моргания)
    const unsigned char* getLookExpressionForDirection(int direction) {
        switch(direction) {
            case 1: return Irisoled::look_up;
            case 2: return Irisoled::look_down;
            case 3: return Irisoled::look_left;
            case 4: return Irisoled::look_right;
            default: return Irisoled::normal;
        }
    }
    
    void drawFace() {
        display.clearDisplay();
        
        if (currentMood == MOOD_NORMAL) {
            display.drawBitmap(0, 0, getLookExpression(), 128, 64, WHITE);
        } else {
            display.drawBitmap(0, 0, expressions[currentMood], 128, 64, WHITE);
        }
        
        display.display();
    }
    
public:
    MochiRobot() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
        currentMood = MOOD_NORMAL;
        lastLookChange = 0;
        lastInteractionTime = 0;
        moodStartTime = 0;
        currentLookDirection = 0;
        targetLookDirection = 0;
        isSleeping = false;
    }
    
    bool begin() {
        if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
            return false;
        }
        
        display.clearDisplay();
        display.display();
        display.setTextColor(WHITE);
        
        pinMode(TOUCH_PIN, INPUT);
        randomSeed(analogRead(0));
        
        return true;
    }
    
    void setMood(RobotMood mood) {
        currentMood = mood;
        moodStartTime = millis();
        
        if (mood != MOOD_SLEEPY) {
            isSleeping = false;
            lastInteractionTime = millis();
        }
        
        drawFace();
    }
    
    void update() {
        unsigned long now = millis();
        
        // Проверяем таймаут для временных эмоций
        if (currentMood != MOOD_NORMAL && currentMood != MOOD_SLEEPY) {
            if (now - moodStartTime > TEMP_MOOD_TIMEOUT) {
                setMood(MOOD_NORMAL);
                return;
            }
        }
        
        // Проверка сна
        if (currentMood == MOOD_NORMAL && !isSleeping && now - lastInteractionTime > SLEEP_TIMEOUT) {
            isSleeping = true;
            setMood(MOOD_SLEEPY);
            return;
        }
        
        if (isSleeping && now - lastInteractionTime < SLEEP_TIMEOUT) {
            isSleeping = false;
            setMood(MOOD_NORMAL);
            return;
        }
        
        // Плавная смена взгляда в NORMAL режиме
        if (currentMood == MOOD_NORMAL) {
            
            // Время сменить направление взгляда?
            if (now - lastLookChange > (unsigned long)random(LOOK_MIN, LOOK_MAX)) {
                // Выбираем новое направление (может быть таким же)
                targetLookDirection = random(0, 5);
                
                // Если направление изменилось - двигаемся плавно
                if (targetLookDirection != currentLookDirection) {
                    smoothMoveToTarget();
                }
                
                lastLookChange = now;
            }
        }
    }
    
    void handleTouch(unsigned long duration) {
        lastInteractionTime = millis();
        
        if (duration < 3000) {
            // Короткое нажатие - случайная эмоция
            int randomMood;
            do {
                randomMood = random(MOOD_HAPPY, MOOD_COUNT - 1);
            } while (randomMood == MOOD_SLEEPY);
            
            setMood((RobotMood)randomMood);
            
        } else {
            // Длинное нажатие - HAPPY
            setMood(MOOD_HAPPY);
        }
    }
    
    void onTelegramMessage() {
        lastInteractionTime = millis();
        setMood(MOOD_ALERT);
    }
    
    void showMessage(const char* line1, const char* line2 = "") {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(10, 20);
        display.println(line1);
        display.setCursor(10, 35);
        display.println(line2);
        display.display();
    }
    
    void showWiFiStatus(const char* ssid, const char* ip) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 5);
        display.println("WiFi Connected!");
        display.setCursor(0, 25);
        display.print("SSID: ");
        display.println(ssid);
        display.setCursor(0, 40);
        display.print("IP: ");
        display.println(ip);
        display.display();
        delay(2000);
        drawFace();
    }
    
    void showWelcome() {
        display.clearDisplay();
        
        // Анимация появления имени
        display.setTextSize(2);
        display.setCursor(20, 15);
        display.println(ROBOT_NAME);
        display.display();
        delay(1000);
        
        // Мигание имени
        for (int i = 0; i < 3; i++) {
            display.clearDisplay();
            display.display();
            delay(200);
            display.setCursor(20, 15);
            display.println(ROBOT_NAME);
            display.display();
            delay(200);
        }
        
        // Версия
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(45, 20);
        display.println(ROBOT_VERSION);
        display.setCursor(30, 35);
        display.println("ready");
        display.display();
        delay(1500);
        
        // Осматривается при запуске
        targetLookDirection = 3; // влево
        smoothMoveToTarget();
        delay(300);
        
        targetLookDirection = 4; // вправо
        smoothMoveToTarget();
        delay(300);
        
        targetLookDirection = 1; // вверх
        smoothMoveToTarget();
        delay(300);
        
        targetLookDirection = 2; // вниз
        smoothMoveToTarget();
        delay(300);
        
        targetLookDirection = 0; // центр
        smoothMoveToTarget();
        
        setMood(MOOD_HAPPY);
        delay(800);
        setMood(MOOD_NORMAL);
    }
    
    void onWiFiSuccess() {
        lastInteractionTime = millis();
        targetLookDirection = 1; // смотрит вверх (радостно)
        smoothMoveToTarget();
        display.clearDisplay();
        display.drawBitmap(0, 0, Irisoled::happy, 128, 64, WHITE);
        display.display();
        delay(500);
        drawFace();
    }
    
    void onWiFiFail() {
        lastInteractionTime = millis();
        targetLookDirection = 2; // смотрит вниз (грустно)
        smoothMoveToTarget();
        display.clearDisplay();
        display.drawBitmap(0, 0, Irisoled::sad, 128, 64, WHITE);
        display.display();
        delay(1000);
        drawFace();
    }
    
    void onPCOn() {
        lastInteractionTime = millis();
        display.clearDisplay();
        display.drawBitmap(0, 0, Irisoled::surprised, 128, 64, WHITE);
        display.display();
        delay(500);
        setMood(MOOD_HAPPY);
        delay(800);
        setMood(MOOD_NORMAL);
    }
};