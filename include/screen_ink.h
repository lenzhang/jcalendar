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

void print_status();

#endif