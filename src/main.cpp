#include <Arduino.h>
#include <ArduinoJson.h>

#include <WiFiManager.h>

#include "OneButton.h"

#include "led.h"
#include "_sntp.h"
#include "weather.h"
#include "screen_ink.h"
#include "_preference.h"

#include "version.h"

#ifdef ESP32C3_BUILD
#include <SPI.h>  // ESP32-C3需要手动SPI配置
#endif

// ESP32-C3兼容性配置
#ifdef ESP32C3_BUILD
    #include "esp32c3_pins.h"
    #define PIN_BUTTON_USED PIN_BUTTON  // 使用ESP32-C3配置的按钮引脚
#else
    #define PIN_BUTTON_USED GPIO_NUM_14 // 原始ESP32按钮引脚
#endif

OneButton button(PIN_BUTTON_USED, true);

void IRAM_ATTR checkTicks() {
    button.tick();
}

WiFiManager wm;
WiFiManagerParameter para_qweather_host("qweather_host", "和风天气Host", "", 64); //     和风天气key
WiFiManagerParameter para_qweather_key("qweather_key", "和风天气API Key", "", 32); //     和风天气key
// const char* test_html = "<br/><label for='test'>天气模式</label><br/><input type='radio' name='test' value='0' checked> 每日天气test </input><input type='radio' name='test' value='1'> 实时天气test</input>";
// WiFiManagerParameter para_test(test_html);
WiFiManagerParameter para_qweather_type("qweather_type", "天气类型（0:每日天气，1:实时天气）", "0", 2, "pattern='\\[0-1]{1}'"); //     城市code
WiFiManagerParameter para_qweather_location("qweather_loc", "位置ID", "", 64); //     城市code
WiFiManagerParameter para_cd_day_label("cd_day_label", "倒数日（4字以内）", "", 10); //     倒数日
WiFiManagerParameter para_cd_day_date("cd_day_date", "日期（yyyyMMdd）", "", 8, "pattern='\\d{8}'"); //     城市code
WiFiManagerParameter para_tag_days("tag_days", "日期Tag（yyyyMMddx，详见README）", "", 30); //     日期Tag
WiFiManagerParameter para_si_week_1st("si_week_1st", "每周起始（0:周日，1:周一）", "0", 2, "pattern='\\[0-1]{1}'"); //     每周第一天

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        Serial.println("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.printf("Wakeup was not caused by deep sleep.\n");
    }
}

void buttonClick(void* oneButton);
void buttonDoubleClick(void* oneButton);
void buttonLongPressStop(void* oneButton);
void go_sleep();

unsigned long _idle_millis;
unsigned long TIME_TO_SLEEP = 600 * 1000;  // 10分钟配网超时

bool _wifi_flag = false;
unsigned long _wifi_failed_millis;
void setup() {
    delay(2000);  // ESP32-C3启动延迟
    
    Serial.begin(115200);
    delay(1000);
    
#ifdef ESP32C3_BUILD
    Serial.println("=== ESP32-C3 DEBUG START ===");
    Serial.println("Serial initialized at 115200 baud");
    Serial.flush();
    delay(500);
#endif
    
    Serial.println(".");
    Serial.flush();
    delay(100);
    
    print_wakeup_reason();
    Serial.println("\r\n\r\n\r\n");
    Serial.flush();
    delay(100);

    button.setClickMs(300);
    button.setPressMs(3000); // 设置长按的时长
    button.attachClick(buttonClick, &button);
    button.attachDoubleClick(buttonDoubleClick, &button);
    // button.attachMultiClick()
    button.attachLongPressStop(buttonLongPressStop, &button);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_USED), checkTicks, CHANGE);

    Serial.printf("***********************\r\n");
    Serial.printf("      J-Calendar\r\n");
    Serial.printf("    version: %s\r\n", J_VERSION);
    Serial.printf("***********************\r\n\r\n");
    Serial.printf("Copyright © 2022-2025 JADE Software Co., Ltd. All Rights Reserved.\r\n\r\n");

    // ESP32-C3 正常启动流程
    Serial.println("=== ESP32-C3 正常启动 ===");
    Serial.flush();

    Serial.println("=== Initializing LED ===");
    Serial.flush();
    delay(100);
    led_init();
    led_fast();
    
    Serial.println("=== 检查WiFi配置状态 ===");
    Serial.flush();
    delay(100);
    
    // 检查是否有WiFi配置
    wm.setHostname("J-Calendar");
    wm.setEnableConfigPortal(false);
    wm.setConnectTimeout(10);
    
    // 尝试自动连接WiFi，这会自动检测是否有保存的配置
    Serial.println("=== 尝试自动连接WiFi ===");
    
    // 调试：检查WiFiManager的getWiFiIsSaved()方法
    bool wmSaved = wm.getWiFiIsSaved();
    Serial.print("WiFiManager.getWiFiIsSaved(): ");
    Serial.println(wmSaved ? "true" : "false");
    Serial.flush();
    
    // 调试：检查WiFi.SSID()方法
    String currentSSID = WiFi.SSID();
    Serial.print("WiFi.SSID(): '");
    Serial.print(currentSSID);
    Serial.print("' (长度: ");
    Serial.print(currentSSID.length());
    Serial.println(")");
    Serial.flush();
    
    // 设置一个较短的连接超时时间来快速检测
    wm.setConnectTimeout(5);
    Serial.println("开始autoConnect()...");
    
    if (wm.autoConnect()) {
        // WiFi连接成功
        Serial.println("=== WiFi连接成功 ===");
        Serial.flush();
        led_on(); // 常亮表示连接成功
        _wifi_flag = true;
    } else {
        // WiFi连接失败，通过WiFi状态判断是否有配置
        Serial.println("=== WiFi连接失败 ===");
        Serial.flush();
        
        // 更可靠的方法：检查WiFi状态和SSID
        String savedSSID = WiFi.SSID();
        bool hasValidConfig = (savedSSID.length() > 0 && !savedSSID.equals(""));
        
        Serial.printf("保存的SSID: '%s' (长度: %d)\n", savedSSID.c_str(), savedSSID.length());
        
        if (!hasValidConfig) {
            // 没有有效的WiFi配置 - 进入配网等待模式
            Serial.println("=== 未检测到有效WiFi配置，显示配网引导 ===");
            Serial.println("这是一台新设备或WiFi配置已被清除");
            Serial.println("请双击按钮启动配网模式");
            
            _wifi_flag = false;
            _wifi_failed_millis = millis();
            led_slow(); // 慢闪表示需要配置
            
            // 显示配网引导界面
            si_show_wifi_config_guide();
            
            // 跳过网络相关任务，设置为完成状态避免在loop中执行
            _sntp_exec(2);  // 设置SNTP状态为跳过
            weather_exec(2); // 设置天气状态为跳过
            // 不关闭WiFi，保持STA模式以便后续启动AP配网
            Serial.println("=== 等待用户配网，不进入休眠 ===");
            
        } else {
            // 有WiFi配置但连接失败 - 可能是网络问题
            Serial.println("=== WiFi配置存在但连接失败 ===");
            Serial.printf("尝试连接到: %s\n", savedSSID.c_str());
            Serial.println("可能原因：密码错误、网络不可用、信号弱等");
            Serial.println("可以双击按钮重新配置，或等待重试");
            
            _wifi_flag = false;
            _wifi_failed_millis = millis();
            led_slow(); // 慢闪表示连接失败
            
            // 显示WiFi连接失败界面
            si_show_wifi_retry();
            
            // 跳过网络相关任务，准备休眠重试
            _sntp_exec(2);
            weather_exec(2);
            WiFi.mode(WIFI_OFF);
            Serial.println("=== WiFi连接失败，60秒后休眠重试 ===");
        }
    }
}

/**
 * 处理各个任务
 * 1. sntp同步
 *      前置条件：Wifi已连接
 * 2. 刷新日历
 *      前置条件：sntp同步完成（无论成功或失败）
 * 3. 刷新天气信息
 *      前置条件：wifi已连接
 * 4. 系统配置
 *      前置条件：无
 * 5. 休眠
 *      前置条件：所有任务都完成或失败，
 */
void loop() {
    button.tick(); // 单击，刷新页面；双击，打开配置；长按，重启
    wm.process();
    // 前置任务：wifi已连接
    // sntp同步
    if (_sntp_status() == -1) {
        _sntp_exec();
    }
    // 如果是定时器唤醒，并且接近午夜（23:50之后），则直接休眠
    if (_sntp_status() == SYNC_STATUS_TOO_LATE) {
        go_sleep();
    }
    // 前置任务：wifi已连接
    // 获取Weather信息
    if (weather_status() == -1) {
        weather_exec();
    }

    // 刷新日历
    // 前置任务：sntp、weather
    // 执行条件：屏幕状态为待处理
    // 注意：weather_status() == 3 表示配置无效，不应该触发屏幕刷新
    if (_sntp_status() > 0 && weather_status() > 0 && weather_status() != 3 && si_screen_status() == -1) {
        // 数据获取完毕后，关闭Wifi，省电
        if (!wm.getConfigPortalActive()) {
            WiFi.mode(WIFI_OFF);
        }
        Serial.println("Wifi closed after data fetch.");

        si_screen();
    } else if (_sntp_status() > 0 && weather_status() == 3 && si_screen_status() == -1) {
        // 天气配置无效，但时间同步成功，显示无天气信息的日历
        if (!wm.getConfigPortalActive()) {
            WiFi.mode(WIFI_OFF);
        }
        Serial.println("Wifi closed after data fetch. Weather config invalid, showing calendar without weather.");
        si_screen();
    }

    // 休眠
    // 前置条件：屏幕刷新完成（或成功）

    // 未在配置状态，且屏幕刷新完成，进入休眠
    if (!wm.getConfigPortalActive() && si_screen_status() > 0) {
        // 添加启动后的延迟，给用户30秒时间进行操作
        static unsigned long startupTime = millis();
        if (millis() - startupTime < 30 * 1000) {
            // 启动后30秒内，每5秒更新一次屏幕倒计时
            static unsigned long lastStartupPrompt = 0;
            if (millis() - lastStartupPrompt > 5 * 1000) {
                int remainingSeconds = 30 - (int)((millis() - startupTime) / 1000);
                Serial.printf("=== 启动后等待用户操作中 (%d/30秒)，双击按钮可进入配网模式 ===\n", 
                             30 - remainingSeconds);
                
                // 在屏幕上显示等待界面
                si_show_startup_waiting(remainingSeconds);
                
                lastStartupPrompt = millis();
            }
            return; // 30秒内不进入休眠
        }
        
        if(_wifi_flag) {
            go_sleep();
        }
        // 检查是否有WiFi配置
        String savedSSID = WiFi.SSID();
        bool hasValidConfig = (savedSSID.length() > 0 && !savedSSID.equals(""));
        
        if(!_wifi_flag && hasValidConfig && millis() - _wifi_failed_millis > 60 * 1000) {
            // 有WiFi配置但连接失败，60秒后休眠重试
            Serial.println("=== WiFi配置存在但连接失败，60秒后休眠重试 ===");
            go_sleep();
        } else if (!_wifi_flag && !hasValidConfig) {
            // 没有WiFi配置，保持等待状态，不休眠
            // 每60秒更新一次屏幕提示
            static unsigned long lastPrompt = 0;
            if (millis() - lastPrompt > 60 * 1000) {
                Serial.println("=== 等待用户配网中，请双击按钮启动配网模式 ===");
                
                // 在屏幕上显示配网引导界面
                si_show_wifi_config_guide();
                
                lastPrompt = millis();
            }
        }
    }
    // 配置状态下，延长超时时间并在休眠前刷新屏幕
    if (wm.getConfigPortalActive()) {
        unsigned long configTime = millis() - _idle_millis;
        unsigned long remainingTime = TIME_TO_SLEEP - configTime;
        
        // 每60秒更新一次屏幕倒计时
        static unsigned long lastConfigPrompt = 0;
        if (millis() - lastConfigPrompt > 60 * 1000) {
            int remainingMinutes = (int)(remainingTime / 60000);
            Serial.printf("=== 配网模式运行中，剩余时间: %d分钟 ===\n", remainingMinutes);
            
            // 在屏幕上显示配网倒计时
            si_show_config_countdown(remainingMinutes);
            
            lastConfigPrompt = millis();
        }
        
        if (configTime > TIME_TO_SLEEP) {
            Serial.println("=== 配网模式超时，准备进入休眠 ===");
            
            // 在休眠前显示超时提示界面
            si_show_config_timeout();
            
            Serial.println("=== 配网模式超时，进入休眠 ===");
            go_sleep();
        }
    }

    delay(10);
}


// 刷新页面
void buttonClick(void* oneButton) {
    Serial.println("Button click.");
    if (wm.getConfigPortalActive()) {
        Serial.println("In config status.");
    } else {
        Serial.println("Refresh screen manually.");
        si_screen();
    }
}

void saveParamsCallback() {
    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    pref.putString(PREF_QWEATHER_HOST, para_qweather_host.getValue());
    pref.putString(PREF_QWEATHER_KEY, para_qweather_key.getValue());
    pref.putString(PREF_QWEATHER_TYPE, strcmp(para_qweather_type.getValue(), "1") == 0 ? "1" : "0");
    pref.putString(PREF_QWEATHER_LOC, para_qweather_location.getValue());
    pref.putString(PREF_CD_DAY_LABLE, para_cd_day_label.getValue());
    pref.putString(PREF_CD_DAY_DATE, para_cd_day_date.getValue());
    pref.putString(PREF_TAG_DAYS, para_tag_days.getValue());
    pref.putString(PREF_SI_WEEK_1ST, strcmp(para_si_week_1st.getValue(), "1") == 0 ? "1" : "0");
    pref.end();

    Serial.println("Params saved.");

    _idle_millis = millis(); // 刷新无操作时间点

    ESP.restart();
}

void preSaveParamsCallback() {
}

// 双击打开配置页面
void buttonDoubleClick(void* oneButton) {
    Serial.println("Button double click.");
    
    if (wm.getConfigPortalActive()) {
        Serial.println("配置模式中，重启设备");
        ESP.restart();
        return;
    }

    Serial.println("=== 启动配网模式 ===");
    
    // 显示配网模式界面
    si_show_config_mode();

    if (weather_status == 0) {
        weather_stop();
    }

    // 确保WiFi处于正确状态
    WiFi.mode(WIFI_AP_STA);  // 设置为AP+STA模式
    delay(100);

    // 设置配置页面
    // 根据配置信息设置默认值
    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    String qHost = pref.getString(PREF_QWEATHER_HOST, "api.qweather.com");
    String qToken = pref.getString(PREF_QWEATHER_KEY, "");
    String qType = pref.getString(PREF_QWEATHER_TYPE, "0");
    String qLoc = pref.getString(PREF_QWEATHER_LOC, "");
    String cddLabel = pref.getString(PREF_CD_DAY_LABLE, "");
    String cddDate = pref.getString(PREF_CD_DAY_DATE, "");
    String tagDays = pref.getString(PREF_TAG_DAYS, "");
    String week1st = pref.getString(PREF_SI_WEEK_1ST, "0");
    pref.end();

    para_qweather_host.setValue(qHost.c_str(), 64);
    para_qweather_key.setValue(qToken.c_str(), 32);
    para_qweather_location.setValue(qLoc.c_str(), 64);
    para_qweather_type.setValue(qType.c_str(), 1);
    para_cd_day_label.setValue(cddLabel.c_str(), 16);
    para_cd_day_date.setValue(cddDate.c_str(), 8);
    para_tag_days.setValue(tagDays.c_str(), 30);
    para_si_week_1st.setValue(week1st.c_str(), 1);

    // 重新初始化WiFiManager以确保干净状态
    wm.setTitle("J-Calendar 电子日历配置");
    wm.addParameter(&para_si_week_1st);
    wm.addParameter(&para_qweather_host);
    wm.addParameter(&para_qweather_key);
    wm.addParameter(&para_qweather_type);
    wm.addParameter(&para_qweather_location);
    wm.addParameter(&para_cd_day_label);
    wm.addParameter(&para_cd_day_date);
    wm.addParameter(&para_tag_days);
    
    // 自定义菜单，优化用户体验
    std::vector<const char*> menu = { "wifi","param","update","sep","info","restart","exit" };
    wm.setMenu(menu);
    wm.setConfigPortalBlocking(false);
    wm.setBreakAfterConfig(true);
    wm.setPreSaveParamsCallback(preSaveParamsCallback);
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setSaveConnect(false); // 保存完wifi信息后是否自动连接，设置为否，以便于用户继续配置param。
    
    // 设置AP配置
    wm.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    
    Serial.println("=== 正在启动配置门户 ===");
    Serial.println("热点名称: J-Calendar");
    Serial.println("热点密码: password");
    Serial.println("配置地址: http://192.168.4.1");
    
    // 启动配置门户
    bool result = wm.startConfigPortal("J-Calendar", "password");
    
    if (result) {
        Serial.println("=== 配网模式启动成功 ===");
    } else {
        Serial.println("=== 配网模式启动失败 ===");
    }

    led_config(); // LED 进入三快闪状态

    // 控制配置超时180秒后休眠
    _idle_millis = millis();
}


// 重置系统，并重启
void buttonLongPressStop(void* oneButton) {
    Serial.println("Button long press.");
    Serial.println("=== 清除所有配置信息 ===");

    // 清除WiFiManager保存的WiFi配置
    wm.resetSettings();
    Serial.println("WiFi配置已清除");

    // 删除应用程序配置
    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    pref.clear();
    pref.end();
    Serial.println("应用配置已清除");

    Serial.println("=== 配置清除完成，重启设备 ===");
    delay(1000);
    ESP.restart();
}

#define uS_TO_S_FACTOR 1000000
#define TIMEOUT_TO_SLEEP  10 // seconds
time_t blankTime = 0;
void go_sleep() {
    // 设置唤醒时间为下个偶数整点。
    time_t now = time(NULL);
    struct tm tmNow = { 0 };
    // Serial.printf("Now: %ld -- %s\n", now, ctime(&now));
    localtime_r(&now, &tmNow); // 时间戳转化为本地时间结构

    uint64_t p;
    // 根据配置情况来刷新，如果未配置qweather信息，则24小时刷新，否则每2小时刷新
    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    String _qweather_key = pref.getString(PREF_QWEATHER_KEY, "");
    pref.end();
    if (_qweather_key.length() == 0 || weather_type() == 0) { // 没有配置天气或者使用按日天气，则第二天刷新。
        Serial.println("Sleep to next day.");
        now += 3600 * 24;
        localtime_r(&now, &tmNow); // 将新时间转成tm
        // Serial.printf("Set1: %ld -- %s\n", now, ctime(&now));

        struct tm tmNew = { 0 };
        tmNew.tm_year = tmNow.tm_year;
        tmNew.tm_mon = tmNow.tm_mon;        // 月份从0开始
        tmNew.tm_mday = tmNow.tm_mday;           // 日期
        tmNew.tm_hour = 0;           // 小时
        tmNew.tm_min = 0;            // 分钟
        tmNew.tm_sec = 10;            // 秒, 防止离线时出现时间误差，所以，延后10s
        time_t set = mktime(&tmNew);

        p = (uint64_t)(set - time(NULL));
        Serial.printf("Sleep time: %ld seconds\n", p);
    } else {
        if (tmNow.tm_hour % 2 == 0) { // 将时间推后两个小时，偶整点刷新。
            now += 7200;
        } else {
            now += 3600;
        }
        localtime_r(&now, &tmNow); // 将新时间转成tm
        // Serial.printf("Set1: %ld -- %s\n", now, ctime(&now));

        struct tm tmNew = { 0 };
        tmNew.tm_year = tmNow.tm_year;
        tmNew.tm_mon = tmNow.tm_mon;        // 月份从0开始
        tmNew.tm_mday = tmNow.tm_mday;           // 日期
        tmNew.tm_hour = tmNow.tm_hour;           // 小时
        tmNew.tm_min = 0;            // 分钟
        tmNew.tm_sec = 10;            // 秒, 防止离线时出现时间误差，所以，延后10s
        time_t set = mktime(&tmNew);

        p = (uint64_t)(set - time(NULL));
        Serial.printf("Sleep time: %ld seconds\n", p);
    }

    esp_sleep_enable_timer_wakeup(p * (uint64_t)uS_TO_S_FACTOR);
    
#ifdef ESP32C3_BUILD
    // ESP32-C3使用深度睡眠GPIO唤醒
    esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_BUTTON_USED, ESP_GPIO_WAKEUP_GPIO_LOW);
#else
    // 原始ESP32使用ext0唤醒
    esp_sleep_enable_ext0_wakeup(PIN_BUTTON_USED, 0);
#endif

    // 省电考虑，关闭RTC外设和存储器
    // esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF); // RTC IO, sensors and ULP, 注意：由于需要按键唤醒，所以不能关闭，否则会导致RTC_IO唤醒失败
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF); // 
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    // 省电考虑，重置gpio，平均每针脚能省8ua。
    gpio_reset_pin(PIN_LED); // 减小deep-sleep电流
    
#ifdef ESP32C3_BUILD
    // ESP32-C3 屏幕引脚重置
    gpio_reset_pin(EPD_CS_PIN);   // CS引脚
    gpio_reset_pin(EPD_DC_PIN);   // DC引脚  
    gpio_reset_pin(EPD_RST_PIN);  // RST引脚
    gpio_reset_pin(EPD_BUSY_PIN); // BUSY引脚
    gpio_reset_pin(SPI_CLK_PIN);  // CLK引脚
    gpio_reset_pin(SPI_MOSI_PIN); // MOSI引脚
#else
    // 原始ESP32引脚重置
    gpio_reset_pin(GPIO_NUM_5);   // 减小deep-sleep电流
    gpio_reset_pin(GPIO_NUM_17);  // 减小deep-sleep电流
    gpio_reset_pin(GPIO_NUM_16);  // 减小deep-sleep电流
    gpio_reset_pin(GPIO_NUM_4);   // 减小deep-sleep电流
#endif

    delay(10);
    Serial.println("Deep sleep...");
    Serial.flush();
    esp_deep_sleep_start();
}