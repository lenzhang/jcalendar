#include "led.h"

#ifdef ESP32C3_BUILD
    #include "esp32c3_pins.h"
#else
    #define PIN_LED GPIO_NUM_22
    #define LED_ON()  digitalWrite(PIN_LED, LOW)   // 原始ESP32 LED低电平有效
    #define LED_OFF() digitalWrite(PIN_LED, HIGH)
#endif

TaskHandle_t LED_HANDLER;
int8_t BLINK_TYPE;

void led_init()
{
    pinMode(PIN_LED, OUTPUT);
    LED_OFF(); // 初始化时关闭LED
    Serial.printf("LED初始化完成 - 使用引脚GPIO%d\n", PIN_LED);
}

void task_led(void *param)
{
    while(1)
    {
        switch(BLINK_TYPE)
        {
            case 0: // 关闭
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(1000));
            break;
            case 1: // 常亮
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(1000));
            break;
            case 2: // 慢闪 (每2秒闪一次)
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(1000));
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(1000));
            break;
            case 3: // 快闪 (每秒闪2次)
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(200));
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(200));
            break;
            case 4: // 配置模式 (三短闪一长灭)
                // 三次短闪
                for(int i = 0; i < 3; i++) {
                    LED_ON();
                    vTaskDelay(pdMS_TO_TICKS(200));
                    LED_OFF();
                    vTaskDelay(pdMS_TO_TICKS(200));
                }
                // 一次长灭
                vTaskDelay(pdMS_TO_TICKS(1000));
            break;
            default:
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void led_fast()
{
    BLINK_TYPE = 3;
    if (LED_HANDLER != NULL)
    {
        vTaskDelete(LED_HANDLER);
    }
    xTaskCreate(task_led, "TASK_LED", 2048, NULL, 5, &LED_HANDLER);
    Serial.println("LED设置为快闪模式 (系统启动中)");
}

void led_slow()
{
    BLINK_TYPE = 2;
    if (LED_HANDLER != NULL)
    {
        vTaskDelete(LED_HANDLER);
    }
    xTaskCreate(task_led, "TASK_LED", 2048, NULL, 5, &LED_HANDLER);
    Serial.println("LED设置为慢闪模式 (WiFi连接失败)");
}

void led_config()
{
    BLINK_TYPE = 4;
    if (LED_HANDLER != NULL)
    {
        vTaskDelete(LED_HANDLER);
    }
    xTaskCreate(task_led, "TASK_LED", 2048, NULL, 5, &LED_HANDLER);
    Serial.println("LED设置为配置模式 (三短闪一长灭)");
}

void led_on()
{
    BLINK_TYPE = 1;
    if (LED_HANDLER != NULL)
    {
        vTaskDelete(LED_HANDLER);
    }
    xTaskCreate(task_led, "TASK_LED", 2048, NULL, 5, &LED_HANDLER);
    Serial.println("LED设置为常亮模式 (WiFi连接成功)");
}

void led_off()
{
    BLINK_TYPE = 0;
    if (LED_HANDLER != NULL)
    {
        vTaskDelete(LED_HANDLER);
    }
    xTaskCreate(task_led, "TASK_LED", 2048, NULL, 5, &LED_HANDLER);
    Serial.println("LED设置为关闭模式 (系统休眠)");
}

