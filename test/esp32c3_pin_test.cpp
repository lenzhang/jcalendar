/*
 * ESP32-C3 电子墨水屏引脚测试程序
 * 适用于合宙CORE-ESP32-C3开发板
 * 用于验证电子墨水屏的引脚连接和基本功能
 * 
 * 用户最终接线方案:
 * VCC    -> 引脚18 (3.3V)
 * GND    -> 引脚17 (GND)
 * CLK    -> 引脚19 (GPIO2)
 * MOSI   -> 引脚20 (GPIO3)
 * RST    -> 引脚21 (GPIO10)
 * DC     -> 引脚22 (GPIO6)
 * CS     -> 引脚23 (GPIO7)
 * BUSY   -> 引脚24 (GPIO11)
 */

#include <Arduino.h>
#include <SPI.h>
#include "esp32c3_pins.h"

// 引脚测试变量
volatile bool busy_state = false;
volatile unsigned long last_busy_change = 0;

// 中断处理函数
void IRAM_ATTR onBusyChange() {
    busy_state = digitalRead(EPD_BUSY_PIN);
    last_busy_change = millis();
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== ESP32-C3 电子墨水屏引脚测试程序 ===");
    Serial.println("测试开始...");
    
    // 初始化引脚
    pinMode(EPD_CS_PIN, OUTPUT);
    pinMode(EPD_DC_PIN, OUTPUT);
    pinMode(EPD_RST_PIN, OUTPUT);
    pinMode(EPD_BUSY_PIN, INPUT);
    
    // 初始化LED引脚用于状态指示
    pinMode(LED_D4_PIN, OUTPUT);
    pinMode(LED_D5_PIN, OUTPUT);
    
    // 设置初始状态
    digitalWrite(EPD_CS_PIN, HIGH);   // CS高电平 - 不选中
    digitalWrite(EPD_DC_PIN, LOW);    // DC低电平 - 命令模式
    digitalWrite(EPD_RST_PIN, HIGH);  // RST高电平 - 正常工作
    
    // 设置BUSY引脚中断
    attachInterrupt(digitalPinToInterrupt(EPD_BUSY_PIN), onBusyChange, CHANGE);
    
    // 初始化SPI
    Serial.println("初始化SPI...");
    SPIClass* hspi = new SPIClass(HSPI);
    
    // 使用用户的最终接线方案
    // CLK=GPIO2, MOSI=GPIO3, CS=GPIO7
    hspi->begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
    
    // 设置SPI参数
    hspi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    
    Serial.println("SPI初始化完成");
    
    // 打印当前引脚配置
    Serial.println("\n=== 当前引脚配置 (用户最终接线方案) ===");
    Serial.printf("CS引脚   (GPIO%d): %s\n", EPD_CS_PIN, digitalRead(EPD_CS_PIN) ? "高电平" : "低电平");
    Serial.printf("DC引脚   (GPIO%d): %s\n", EPD_DC_PIN, digitalRead(EPD_DC_PIN) ? "高电平" : "低电平");
    Serial.printf("RST引脚  (GPIO%d): %s\n", EPD_RST_PIN, digitalRead(EPD_RST_PIN) ? "高电平" : "低电平");
    Serial.printf("BUSY引脚 (GPIO%d): %s\n", EPD_BUSY_PIN, digitalRead(EPD_BUSY_PIN) ? "高电平" : "低电平");
    Serial.printf("CLK引脚  (GPIO%d): SPI时钟\n", SPI_CLK_PIN);
    Serial.printf("MOSI引脚 (GPIO%d): SPI数据\n", SPI_MOSI_PIN);
    
    Serial.println("\n=== 引脚接线检查 ===");
    Serial.println("引脚19 (GPIO2) -> CLK");
    Serial.println("引脚20 (GPIO3) -> MOSI");
    Serial.println("引脚21 (GPIO10) -> RST");
    Serial.println("引脚22 (GPIO6) -> DC");
    Serial.println("引脚23 (GPIO7) -> CS");
    Serial.println("引脚24 (GPIO11) -> BUSY");
    Serial.println("引脚17 (GND) -> GND");
    Serial.println("引脚18 (3.3V) -> VCC");
    
    Serial.println("\n开始引脚功能测试...");
}

void loop() {
    static unsigned long last_test = 0;
    static int test_step = 0;
    
    if (millis() - last_test > 3000) {  // 每3秒执行一次测试
        last_test = millis();
        
        // LED指示当前测试状态
        digitalWrite(LED_D4_PIN, test_step % 2);
        digitalWrite(LED_D5_PIN, (test_step + 1) % 2);
        
        switch (test_step % 4) {
            case 0:
                Serial.println("\n--- 测试CS引脚 (GPIO7) ---");
                Serial.println("设置CS为低电平...");
                digitalWrite(EPD_CS_PIN, LOW);
                delay(500);
                Serial.printf("CS引脚状态: %s\n", digitalRead(EPD_CS_PIN) ? "高电平" : "低电平");
                digitalWrite(EPD_CS_PIN, HIGH);
                break;
                
            case 1:
                Serial.println("\n--- 测试DC引脚 (GPIO6) ---");
                Serial.println("切换DC引脚状态...");
                digitalWrite(EPD_DC_PIN, HIGH);
                delay(500);
                Serial.printf("DC引脚状态: %s (数据模式)\n", digitalRead(EPD_DC_PIN) ? "高电平" : "低电平");
                digitalWrite(EPD_DC_PIN, LOW);
                Serial.printf("DC引脚状态: %s (命令模式)\n", digitalRead(EPD_DC_PIN) ? "高电平" : "低电平");
                break;
                
            case 2:
                Serial.println("\n--- 测试RST引脚 (GPIO10) ---");
                Serial.println("执行复位操作...");
                digitalWrite(EPD_RST_PIN, LOW);
                delay(100);
                Serial.printf("RST引脚状态: %s (复位中)\n", digitalRead(EPD_RST_PIN) ? "高电平" : "低电平");
                digitalWrite(EPD_RST_PIN, HIGH);
                delay(100);
                Serial.printf("RST引脚状态: %s (正常工作)\n", digitalRead(EPD_RST_PIN) ? "高电平" : "低电平");
                break;
                
            case 3:
                Serial.println("\n--- 测试BUSY引脚 (GPIO11) ---");
                Serial.printf("BUSY引脚状态: %s\n", digitalRead(EPD_BUSY_PIN) ? "忙碌" : "空闲");
                if (last_busy_change > 0) {
                    Serial.printf("上次BUSY状态变化: %lu ms前\n", millis() - last_busy_change);
                }
                
                // 发送测试SPI数据
                Serial.println("发送测试SPI数据...");
                digitalWrite(EPD_CS_PIN, LOW);
                SPI.transfer(0x12);  // 发送测试字节
                digitalWrite(EPD_CS_PIN, HIGH);
                break;
        }
        
        test_step++;
        
        // 显示系统状态
        Serial.printf("\n--- 系统状态 ---\n");
        Serial.printf("运行时间: %lu ms\n", millis());
        Serial.printf("可用内存: %u bytes\n", ESP.getFreeHeap());
        Serial.printf("芯片温度: %.1f°C\n", ESP.getChipTemperature());
        
        // 显示引脚电平状态
        Serial.printf("当前引脚状态: CS=%d, DC=%d, RST=%d, BUSY=%d\n",
                     digitalRead(EPD_CS_PIN),
                     digitalRead(EPD_DC_PIN), 
                     digitalRead(EPD_RST_PIN),
                     digitalRead(EPD_BUSY_PIN));
        
        Serial.println("==========================================");
    }
    
    delay(100);
} 