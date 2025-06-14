#ifndef ___SCREEN_INK_H__
#define ___SCREEN_INK_H__

#include <Arduino.h>

int si_calendar_status();
void si_calendar();

int si_wifi_status();
void si_wifi();

int si_weather_status();
void si_weather();

int si_screen_status();
void si_screen();

// 开机屏幕测试功能
void si_screen_test();
bool si_screen_test_passed();

// WiFi配网引导界面
void si_show_wifi_config_guide();
void si_show_wifi_connecting(const char* ssid);
void si_show_wifi_failed();
void si_show_config_mode();
void si_show_config_timeout();
void si_show_config_countdown(int remainingMinutes);
void si_show_startup_waiting(int remainingSeconds);
void si_show_wifi_retry();

// 屏幕硬件测试
void si_screen_hardware_test();

// 基础SPI和显示器测试
void si_basic_spi_test();

void print_status();

#endif