#ifndef ESP32C3_PINS_H
#define ESP32C3_PINS_H

/*
 * ESP32-C3 引脚配置文件
 * 适用于合宙CORE-ESP32-C3开发板
 * 
 * 使用开发板引脚19-24进行接线 (用户最终接线方案)
 * 
 * 引脚19-24对应关系:
 * - 引脚19: IO02 (GPIO2, SPI2_CLK)
 * - 引脚20: IO03 (GPIO3, SPI2_MOSI)
 * - 引脚21: IO10 (GPIO10, RST)
 * - 引脚22: IO06 (GPIO6, DC)
 * - 引脚23: IO07 (GPIO7, CS)
 * - 引脚24: PB_11 (GPIO11, BUSY - 已解锁)
 * 
 * 另外还需要电源连接:
 * - 引脚17: GND (接地)
 * - 引脚18: 3.3V (芯片电源)
 */

// 电子墨水屏引脚配置 - 用户最终接线方案
#define EPD_CS_PIN      GPIO_NUM_7     // 引脚23 - GPIO7 (SPI片选)
#define EPD_DC_PIN      GPIO_NUM_6     // 引脚22 - GPIO6 (数据/命令选择)
#define EPD_RST_PIN     GPIO_NUM_10    // 引脚21 - GPIO10 (复位信号)
#define EPD_BUSY_PIN    GPIO_NUM_11    // 引脚24 - GPIO11 (忙信号检测，已解锁)

// SPI硬件引脚配置
#define SPI_MOSI_PIN    GPIO_NUM_3     // 引脚20 - GPIO3 (SPI数据输出)
#define SPI_MISO_PIN    GPIO_NUM_2     // 引脚19 - GPIO2 (SPI数据输入，通常不用)
#define SPI_CLK_PIN     GPIO_NUM_2     // 引脚19 - GPIO2 (SPI时钟)

// 开发板LED引脚 (可用于状态指示)
#define LED_D4_PIN      12    // GPIO12 - 高电平有效
#define LED_D5_PIN      13    // GPIO13 - 高电平有效

// 按键引脚
#define BOOT_KEY_PIN    9     // GPIO9 - BOOT按键，低电平有效

// WiFi状态LED (可选)
#define WIFI_LED_PIN    LED_D4_PIN    // 使用D4 LED显示WiFi状态
#define STATUS_LED_PIN  LED_D5_PIN    // 使用D5 LED显示系统状态

// 其他可用GPIO (不在接线范围内)
#define GPIO_SPARE_1    0     // GPIO0
#define GPIO_SPARE_2    1     // GPIO1
#define GPIO_SPARE_3    4     // GPIO4
#define GPIO_SPARE_4    5     // GPIO5

/*
 * 电子墨水屏接线方案 (用户最终方案):
 * 
 * 电子墨水屏    开发板      GPIO       功能说明
 * ----------   --------   --------   ---------
 * VCC       <- 引脚18     3.3V       电源正极
 * GND       <- 引脚17     GND        电源负极
 * DIN(MOSI) <- 引脚20     GPIO3      SPI数据输入
 * CLK       <- 引脚19     GPIO2      SPI时钟
 * CS        <- 引脚23     GPIO7      SPI片选
 * DC        <- 引脚22     GPIO6      数据/命令选择
 * RST       <- 引脚21     GPIO10     复位信号
 * BUSY      <- 引脚24     GPIO11     忙信号反馈 (已解锁)
 * 
 * 接线优势:
 * 1. 使用连续引脚19-24进行信号连接，接线整齐
 * 2. 包含电源引脚17-18，方便统一供电
 * 3. 充分利用已解锁的GPIO11作为BUSY检测
 * 4. 避开所有特殊功能引脚，确保稳定性
 * 
 * 注意事项:
 * 1. GPIO2同时用作CLK和MISO，但在电子墨水屏中不冲突
 *    因为电子墨水屏是单向通信，不需要MISO数据返回
 * 2. 所有GPIO都支持3.3V电平，与电子墨水屏兼容
 * 3. SPI频率建议设置为1-4MHz，确保信号稳定
 * 4. GPIO11已解锁，可以安全用作BUSY检测
 */

// ESP32-C3 兼容的按钮和LED引脚配置
#ifdef ESP32C3_BUILD
    // ESP32-C3 按钮引脚 (支持RTC唤醒)
    #define PIN_BUTTON_ESP32C3  GPIO_NUM_9   // GPIO9 - BOOT按键，支持RTC唤醒
    
    // ESP32-C3 LED引脚 (使用开发板LED)
    #define PIN_LED_ESP32C3     GPIO_NUM_12  // GPIO12 - D4 LED，高电平有效
    
    // 兼容性定义
    #define PIN_BUTTON          PIN_BUTTON_ESP32C3
    #define PIN_LED             PIN_LED_ESP32C3
    
    // LED控制宏 (ESP32-C3的LED是高电平有效)
    #define LED_ON()            digitalWrite(PIN_LED, HIGH)
    #define LED_OFF()           digitalWrite(PIN_LED, LOW)
    
#else
    // 原始ESP32引脚配置
    #define PIN_BUTTON          GPIO_NUM_14  // 原始按钮引脚
    #define PIN_LED             GPIO_NUM_22  // 原始LED引脚
    
    // LED控制宏 (原始ESP32的LED是低电平有效)
    #define LED_ON()            digitalWrite(PIN_LED, LOW)
    #define LED_OFF()           digitalWrite(PIN_LED, HIGH)
    
#endif

#endif // ESP32C3_PINS_H 