#include "screen_ink.h"

#include <weather.h>
#include <API.hpp>
#include "holiday.h"
#include "nongli.h"

#include <_preference.h>

#include <U8g2_for_Adafruit_GFX.h>
#include <GxEPD2_3C.h>
#include "GxEPD2_display_selection_new_style.h"
#include "esp32c3_pins.h"  // ESP32-C3引脚配置

#ifdef ESP32C3_BUILD
#include <SPI.h>  // ESP32-C3需要手动SPI配置
#endif

#include "font.h"
#define ROTATION 0

// ESP32-C3 电子墨水屏显示器配置
// 使用预定义的引脚配置，可在esp32c3_pins.h中切换不同方案
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, GxEPD2_DRIVER_CLASS::HEIGHT> display(GxEPD2_DRIVER_CLASS(
    EPD_CS_PIN,    // SPI片选
    EPD_DC_PIN,    // 数据/命令选择  
    EPD_RST_PIN,   // 复位
    EPD_BUSY_PIN   // 忙信号检测
));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;


#define FONT_TEXT u8g2_font_wqy16_t_gb2312 // 224825bytes，最大字库（天气描述中"霾"，只有此字库中有）
#define FONT_SUB u8g2_font_wqy12_t_gb2312 // 次要字体，u8g2最小字体
const String week_str[] = { "日", "一", "二", "三", "四", "五", "六" };
// const String tg_str[] = { "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸" };            // 天干
// const String dz_str[] = { "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥" }; // 地支
// const String sx_str[] = { "鼠", "牛", "虎", "兔", "龙", "蛇", "马", "羊", "猴", "鸡", "狗", "猪" }; // 生肖
const String nl10_str[] = { "初", "十", "廿", "卅" }; // 农历十位
const String nl_str[] = { "十", "一", "二", "三", "四", "五", "六", "七", "八", "九", "十" }; // 农历个位
const String nl_mon_str[] = { "", "正", "二", "三", "四", "五", "六", "七", "八", "九", "十", "冬", "腊" }; // 农历首位

int _screen_status = -1;
int _calendar_status = -1;
String _cd_day_label;
String _cd_day_date;
String _tag_days_str;
int _week_1st;
int lunarDates[31];
int jqAccDate[24]; // 节气积累日

const int jrLength = 11;
const int jrDate[] = { 101, 214, 308, 312, 501, 504, 601, 701, 801, 910, 1001, 1224, 1225 };
const String jrText[] = { "元旦", "情人节", "妇女节", "植树节", "劳动节", "青年节", "儿童节", "建党节", "建军节", "教师节", "国庆节", "平安夜", "圣诞节" };

struct tm tmInfo = { 0 }; // 日历显示用的时间

struct
{
    int16_t topX;
    int16_t topY;
    int16_t topW;
    int16_t topH;

    int16_t tX;
    int16_t tY;
    int16_t tW;
    int16_t tH;

    int16_t yearX;
    int16_t yearY;
    int16_t weekX;
    int16_t weekY;

    int16_t lunarYearX;
    int16_t lunarYearY;
    int16_t lunarDayX;
    int16_t lunarDayY;
    int16_t cdDayX;
    int16_t cdDayY;

    int16_t weatherX;
    int16_t weatherY;
    int16_t weatherW;
    int16_t weatherH;

    int16_t headerX;
    int16_t headerY;
    int16_t headerW;
    int16_t headerH;

    int16_t daysX;
    int16_t daysY;
    int16_t daysW;
    int16_t daysH;
    int16_t dayW;
    int16_t dayH;
} static calLayout;

TaskHandle_t SCREEN_HANDLER;

void init_cal_layout_size() {
    calLayout.topX = 0;
    calLayout.topY = 0;
    calLayout.topW = 180;
    calLayout.topH = 60;

    calLayout.yearX = 10;
    calLayout.yearY = calLayout.topH - 28;
    calLayout.weekX = 10;
    calLayout.weekY = calLayout.topH - 5;

    calLayout.lunarYearX = calLayout.topX + calLayout.topW;
    calLayout.lunarYearY = calLayout.yearY / 2;
    calLayout.lunarDayX = calLayout.topX + calLayout.topW;
    calLayout.lunarDayY = calLayout.yearY;

    calLayout.cdDayX = 0;
    calLayout.cdDayY = calLayout.topH - 5;

    calLayout.tX = calLayout.topX + calLayout.topW;
    calLayout.tY = calLayout.topY;
    calLayout.tW = 60;
    calLayout.tH = calLayout.topH / 2;

    calLayout.weatherX = 300;
    calLayout.weatherY = calLayout.topY;
    calLayout.weatherW = display.width() - calLayout.weatherX;
    calLayout.weatherH = calLayout.topH;

    calLayout.headerX = calLayout.topX;
    calLayout.headerY = calLayout.topY + calLayout.topH;
    calLayout.headerW = display.width();
    calLayout.headerH = 20;

    calLayout.daysX = calLayout.topX;
    calLayout.daysY = calLayout.headerY + calLayout.headerH;
    calLayout.daysW = calLayout.headerW;
    calLayout.daysH = display.height() - calLayout.daysX;
    calLayout.dayW = 56;
    calLayout.dayH = 44;
}

void draw_cal_layout() {
    uint16_t color;

    u8g2Fonts.setFont(FONT_TEXT);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    int16_t daysMagin = 4;

    for (int i = 0; i < 7; i++) {
        if((i + _week_1st) % 7 == 0 || (i + _week_1st) % 7 == 6) {
            color = GxEPD_RED;
        } else {
            color = GxEPD_BLACK;
        }
        // header background
        if(i == 0) {
            display.fillRect(0, calLayout.headerY, (display.width() - 7 * calLayout.dayW)/2, calLayout.headerH, color);
        } else if(i == 6) {
            display.fillRect((display.width() + 7 * calLayout.dayW)/2, calLayout.headerY, (display.width() - 7 * calLayout.dayW)/2, calLayout.headerH, color);
        }
        display.fillRect((display.width() - 7 * calLayout.dayW)/2 + i * calLayout.dayW, calLayout.headerY, calLayout.dayW, calLayout.headerH, color);
        
        // header text
        u8g2Fonts.drawUTF8(calLayout.headerX + daysMagin + (calLayout.dayW - u8g2Fonts.getUTF8Width(week_str[i].c_str())) / 2 + i * calLayout.dayW, calLayout.headerY + calLayout.headerH - 3, week_str[(i + _week_1st) % 7].c_str());
    }
}

uint16_t todayColor = GxEPD_BLACK;
String todayLunarYear;
String todayLunarDay;

// 更新年份
void draw_cal_year(bool partial) {
    if (partial) {
        display.setPartialWindow(calLayout.topX, calLayout.topY, calLayout.topW + calLayout.tW, calLayout.topH);
        display.firstPage();
        display.fillScreen(GxEPD_WHITE);
    }

    // 日期
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(todayColor);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.setCursor(calLayout.yearX, calLayout.yearY);
    u8g2Fonts.setFont(u8g2_font_fub25_tn);
    u8g2Fonts.print(String(tmInfo.tm_year + 1900).c_str());
    u8g2Fonts.setFont(FONT_TEXT);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.print("年");
    u8g2Fonts.setFont(u8g2_font_fub25_tn);
    u8g2Fonts.setForegroundColor(todayColor);
    u8g2Fonts.print(String(tmInfo.tm_mon + 1).c_str());
    u8g2Fonts.setFont(FONT_TEXT);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.print("月");
    u8g2Fonts.setFont(u8g2_font_fub25_tn);
    u8g2Fonts.setForegroundColor(todayColor);
    u8g2Fonts.printf(String(tmInfo.tm_mday).c_str());
    u8g2Fonts.setFont(FONT_TEXT);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.print("日");

    calLayout.lunarYearX = u8g2Fonts.getCursorX() + 15;
    calLayout.lunarDayX = u8g2Fonts.getCursorX() + 15;

    // 星期几
    u8g2Fonts.setFont(FONT_TEXT);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(todayColor);
    u8g2Fonts.setCursor(calLayout.weekX, calLayout.weekY);
    u8g2Fonts.print("星期" + week_str[tmInfo.tm_wday]);

    calLayout.cdDayX = u8g2Fonts.getCursorX(); // update cd day X;

    // 今日农历年份，e.g. 乙巳年 蛇
    // 如果农历月份小于公历月份，那么说明是上一年
    u8g2Fonts.setCursor(calLayout.lunarYearX, calLayout.lunarYearY);
    u8g2Fonts.setFont(FONT_SUB);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.print(todayLunarYear);

    // 今日农历日期
    u8g2Fonts.setCursor(calLayout.lunarDayX, calLayout.lunarDayY);
    u8g2Fonts.setFont(FONT_SUB);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.print(todayLunarDay);

    // 特殊日期
    // draw_special_day();

    if (partial) {
        display.nextPage();
    }
}

void draw_cal_days(bool partial) {
    if (partial) {
        display.setPartialWindow(calLayout.daysX, calLayout.daysY, calLayout.daysW, calLayout.daysH);
        display.firstPage();
        display.fillScreen(GxEPD_WHITE);
    }

    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);

    size_t totalDays = 30; // 小月
    int monthNum = tmInfo.tm_mon + 1;
    if (monthNum == 1 || monthNum == 3 || monthNum == 5 || monthNum == 7 || monthNum == 8 || monthNum == 10 || monthNum == 12) { // 大月
        totalDays = 31;
    }
    if (monthNum == 2) {
        if ((tmInfo.tm_year + 1900) == 0 && (tmInfo.tm_year + 1900) % 100 != 0) {
            totalDays = 29; // 闰二月
        } else {
            totalDays = 28; // 二月
        }
    }

    // 计算本月第一天星期几
    int wday1 = (36 - tmInfo.tm_mday + tmInfo.tm_wday) % 7;
    // 计算本月第一天是全年的第几天（0～365）
    int yday1 = tmInfo.tm_yday - tmInfo.tm_mday + 1;

    // 确认哪些日期需要打tag
    char tags[31] = { 0 };
    int indexBegin = 0;
    while (_tag_days_str.length() >= (indexBegin + 9)) {
        String y = _tag_days_str.substring(indexBegin, indexBegin + 4);
        String m = _tag_days_str.substring(indexBegin + 4, indexBegin + 6);
        String d = _tag_days_str.substring(indexBegin + 6, indexBegin + 8);
        char t = _tag_days_str.charAt(indexBegin + 8);

        if ((y.equals(String(tmInfo.tm_year + 1900)) || y.equals("0000")) && (m.equals(String(tmInfo.tm_mon + 1)) || m.equals("00"))) {
            tags[d.toInt()] = t;
        }

        // Serial.printf("Format: %s, %s, %s, %c\n", y.c_str(), m.c_str(), d.c_str(), t);

        indexBegin = indexBegin + 9;
        while (indexBegin < _tag_days_str.length() && (_tag_days_str.charAt(indexBegin) < '0' || _tag_days_str.charAt(indexBegin) > '9')) { // 搜索字符串直到下个字符是0-9之间的
            indexBegin++;
        }
    }

    Holiday _holiday;
    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    size_t holiday_size = pref.getBytesLength(PREF_HOLIDAY);
    if (holiday_size > 0) {
        pref.getBytes(PREF_HOLIDAY, &_holiday, holiday_size);
    }
    pref.end();

    if(_holiday.year != tmInfo.tm_year + 1900 || _holiday.month != tmInfo.tm_mon + 1) {
        _holiday = {};
    }

    int jqIndex = 0;
    int jrIndex = 0;
    int shiftDay = (wday1 - _week_1st) >= 0 ? 0 : 7;
    for (size_t iDay = 0; iDay < totalDays; iDay++) {
        uint8_t num = wday1 + iDay - _week_1st + shiftDay; // 根据每周首日星期做偏移
        uint8_t column = num % 7; //(0~6)
        uint8_t row = num / 7;    //(0~4)
        if (row == 5) row = 0;
        int16_t x = calLayout.daysX + 4 + column * 56;
        int16_t y = calLayout.daysY + row * 44;

        // 周六、日，字体红色
        uint16_t color;
        if ((wday1 + iDay) % 7 == 0 || (wday1 + iDay) % 7 == 6) {
            color = GxEPD_RED;
        } else {
            color = GxEPD_BLACK;
        }
        
        if (tmInfo.tm_year + 1900 == _holiday.year && tmInfo.tm_mon + 1 == _holiday.month) {
            uint8_t holidayIndex = 0;
            for (; holidayIndex < _holiday.length; holidayIndex++) {
                if (abs(_holiday.holidays[holidayIndex]) == (iDay + 1)) {
                    // 显示公休、调班logo和颜色
                    u8g2Fonts.setFont(u8g2_font_open_iconic_all_1x_t);
                    if (_holiday.holidays[holidayIndex] > 0) { // 公休
                        color = GxEPD_RED;
                        u8g2Fonts.setForegroundColor(color);
                        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
                        u8g2Fonts.drawUTF8(x + 44, y + 11, "\u006c");
                    } else if (_holiday.holidays[holidayIndex] < 0) { // 调班
                        color = GxEPD_BLACK;
                        u8g2Fonts.setForegroundColor(color);
                        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
                        u8g2Fonts.drawUTF8(x + 44, y + 11, "\u0064");
                    }
                    break;
                }
            }
        }
        u8g2Fonts.setForegroundColor(color); // 设置整体颜色
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);

        // 画日历日期数字
        u8g2Fonts.setFont(u8g2_font_fub17_tn); // u8g2_font_fub17_tn，u8g2_font_logisoso18_tn
        int16_t numX = x + (56 - u8g2Fonts.getUTF8Width(String(iDay + 1).c_str())) / 2;
        int16_t numY = y + 22;
        u8g2Fonts.drawUTF8(numX, numY, String(iDay + 1).c_str()); // 画日历日期

        // 画节气&节日&农历       
        String lunarStr = "";
        int lunarDate = lunarDates[iDay];
        int isLeapMon = lunarDate < 0 ? 1 : 0; // 闰月
        lunarDate = abs(lunarDate);
        int lunarMon = lunarDate / 100;
        int lunarDay = lunarDate % 100;

        bool isJq = false; // 是否节气
        int accDays0 = tmInfo.tm_yday + 1 - tmInfo.tm_mday; // 本月0日的积累日（tm_yday 从0开始，tm_mday从1开始, i从0开始）
        for (; jqIndex < 24; jqIndex++) {
            if (accDays0 + iDay + 1 < jqAccDate[jqIndex]) {
                break;
            }
            if (accDays0 + iDay + 1 == jqAccDate[jqIndex]) {
                lunarStr = String(nl_jq_text[jqIndex]);
                isJq = true;
                break;
            }
        }
        bool isJr = false; // 是否节日
        for (; jrIndex < jrLength; jrIndex++) {
            if (tmInfo.tm_mon * 100 + iDay + 1 < jrDate[jrIndex]) {
                break;
            }
            if (tmInfo.tm_mon * 100 + iDay + 1 < jrDate[jrIndex]) {
                lunarStr = jrText[jrIndex];
                isJr == true;
                break;
            }
        }
        if (!isJq && !isJr) { // 农历
            if (lunarDay == 1) {
                // 初一，显示月份
                lunarStr = (isLeapMon == 0 ? "" : "闰") + nl_mon_str[lunarMon] + "月";
            } else {
                if (lunarDay == 10) {
                    lunarStr = "初十";
                } else if (lunarDay == 20) {
                    lunarStr = "二十";
                } else if (lunarDay == 30) {
                    lunarStr = "三十";
                } else {
                    // 其他日期
                    lunarStr = nl10_str[lunarDay / 10] + nl_str[lunarDay % 10];
                }
            }
            if (lunarMon == 1 && lunarDay == 1) {
                lunarStr = "春节";
            } else if (lunarMon == 1 && lunarDay == 15) {
                lunarStr = "元宵节";
            } else if (lunarMon == 5 && lunarDay == 5) {
                lunarStr = "端午节";
            } else if (lunarMon == 7 && lunarDay == 7) {
                lunarStr = "七夕节";
            } else if (lunarMon == 8 && lunarDay == 15) {
                lunarStr = "中秋节";
            } else if (lunarMon == 9 && lunarDay == 9) {
                lunarStr = "重阳节";
            }
        }
        // 画节气/节日/农历文字
        u8g2Fonts.setFont(FONT_TEXT);
        u8g2Fonts.drawUTF8(x + (56 - u8g2Fonts.getUTF8Width(lunarStr.c_str())) / 2, y + 44 - 4, lunarStr.c_str());

        // 今日日期
        if ((iDay + 1) == tmInfo.tm_mday) { // 双线加粗
            todayColor = color;

            // 加框线
            display.drawRoundRect(x, y + 1, 56, 44, 4, GxEPD_RED);
            display.drawRoundRect(x + 1, y + 2, 54, 42, 3, GxEPD_RED);

            // 今日农历年份，e.g. 乙巳年 蛇
            // 如果农历月份小于公历月份，那么说明是上一年
            int tg = nl_tg(tmInfo.tm_year + 1900 - (lunarMon > (tmInfo.tm_mon + 1)? 1:0));
            int dz = nl_dz(tmInfo.tm_year + 1900 - (lunarMon > (tmInfo.tm_mon + 1)? 1:0));;
            todayLunarYear = String(nl_tg_text[tg]) + String(nl_dz_text[dz]) + "年 " + String(nl_sx_text[dz]);

            // 今日农历日期
            if (lunarDay == 10) {
                lunarStr = "初十";
            } else if (lunarDay == 20) {
                lunarStr = "二十";
            } else if (lunarDay == 30) {
                lunarStr = "三十";
            } else {
                // 其他日期
                lunarStr = nl10_str[lunarDay / 10] + nl_str[lunarDay % 10];
            }
            todayLunarDay = (isLeapMon == 0 ? "" : "闰") + nl_mon_str[lunarMon] + "月" + lunarStr;
        }

        // 画日期Tag
        const char* tagChar = NULL;
        if (tags[iDay + 1] == 'a') { //tag
            tagChar = "\u0042";
        } else if (tags[iDay + 1] == 'b') { // dollar
            tagChar = "\u0024";
        } else if (tags[iDay + 1] == 'c') { // smile
            tagChar = "\u0053";
        } else if (tags[iDay + 1] == 'd') { // warning
            tagChar = "\u0021";
        }
        if (tagChar != NULL) {
            u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
            u8g2Fonts.setForegroundColor(GxEPD_RED);
            u8g2Fonts.setFont(u8g2_font_twelvedings_t_all);
            int iconX = numX - u8g2Fonts.getUTF8Width(tagChar) - 1; // 数字与tag间间隔1像素
            iconX = iconX <= (x + 3) ? (iconX + 1) : iconX; // 防止icon与今日框线产生干涉。
            int iconY = y + 15;
            u8g2Fonts.drawUTF8(iconX, iconY, tagChar);
        }

        // 画Calendar提示点
        /*
        Calendar* cal = weather_cal();
        for(int calIndex = 0; calIndex < cal->length; calIndex ++) {
            CalEvent event = cal->events[calIndex];
            if(event.dt_begin.substring(0, 4).toInt() == (tmInfo.tm_year + 1900)
            && event.dt_begin.substring(4, 6).toInt() == (tmInfo.tm_mon + 1)
            && event.dt_begin.substring(6, 8).toInt() == (iDay + 1)) {
                u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
                u8g2Fonts.setForegroundColor(GxEPD_RED);
                u8g2Fonts.setFont(u8g2_font_siji_t_6x10);
                u8g2Fonts.drawUTF8(x + 42, y + 12, "\ue015");
            }
        }
        */
    }
}

// draw countdown-day info
void draw_cd_day(String label, String date) {
    if (label == NULL || date == NULL || label.length() == 0 || date.length() != 8) {
        Serial.print("Invalid countdown-day parameters.\n");
        return;
    }

    long d = atol(date.c_str());

    struct tm today = { 0 }; // 今日0秒
    today.tm_year = tmInfo.tm_year;
    today.tm_mon = tmInfo.tm_mon;
    today.tm_mday = tmInfo.tm_mday;
    today.tm_hour = 0;
    today.tm_min = 0;
    today.tm_sec = 0;
    time_t todayT = mktime(&today);

    struct tm someday = { 0 }; // 倒计日0秒
    someday.tm_year = d / 10000 - 1900;
    someday.tm_mon = d % 10000 / 100 - 1;
    someday.tm_mday = d % 100;
    someday.tm_hour = 0;
    someday.tm_min = 0;
    someday.tm_sec = 0;
    time_t somedayT = mktime(&someday);

    /*
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &someday);
    Serial.printf("CD day: %s\n", buffer);
    */

    long diff = somedayT - todayT;
    if (diff < 0) return; // 倒计日已过

    int16_t beginX = calLayout.cdDayX;
    int16_t endX = calLayout.weatherX;
    int16_t y = calLayout.cdDayY;

    if (diff == 0) {
        String prefix = "今日 ";
        String suffix = " ！！！";
        u8g2Fonts.setFont(FONT_SUB);
        int16_t preWidth = u8g2Fonts.getUTF8Width(prefix.c_str());
        int16_t suffixWidth = u8g2Fonts.getUTF8Width(suffix.c_str());
        u8g2Fonts.setFont(FONT_SUB);
        int16_t labelWidth = u8g2Fonts.getUTF8Width(label.c_str());
        int16_t margin = (endX - beginX - preWidth - labelWidth - suffixWidth) / 2;

        u8g2Fonts.setCursor((margin > 0 ? margin : 0) + beginX, y); // 居中显示
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.print(prefix.c_str()); // 今天
        u8g2Fonts.setForegroundColor(GxEPD_RED);
        u8g2Fonts.setFont(FONT_TEXT);
        u8g2Fonts.print(label.c_str()); // ****
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.print(suffix.c_str()); // ！
    } else if (diff > 0) {
        String prefix = "距 ";
        String middle = " 还有 ";
        int iDiff = diff / (60 * 60 * 24);
        char days[11] = "";
        itoa(iDiff, days, 10);
        String suffix = " 天";

        u8g2Fonts.setFont(FONT_SUB);
        int16_t preWidth = u8g2Fonts.getUTF8Width(prefix.c_str());
        int16_t midWidth = u8g2Fonts.getUTF8Width(middle.c_str());
        int16_t suffixWidth = u8g2Fonts.getUTF8Width(suffix.c_str());
        u8g2Fonts.setFont(FONT_SUB);
        int16_t labelWidth = u8g2Fonts.getUTF8Width(label.c_str());
        u8g2Fonts.setFont(u8g2_font_fub14_tn);
        int16_t daysWidth = u8g2Fonts.getUTF8Width(days);
        int16_t margin = (endX - beginX - preWidth - labelWidth - midWidth - daysWidth - suffixWidth) / 2;

        u8g2Fonts.setCursor((margin > 0 ? margin : 0) + beginX, y); // 居中显示
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.print(prefix.c_str()); // 距
        u8g2Fonts.setFont(FONT_TEXT);
        u8g2Fonts.print(label.c_str()); // ****
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.print(middle.c_str()); // 还有
        u8g2Fonts.setForegroundColor(GxEPD_RED);
        u8g2Fonts.setFont(u8g2_font_fub14_tn);
        u8g2Fonts.print(days); // 0000
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.print(suffix.c_str()); // 天
    }
}

void draw_special_day() {
    String str = "Special Days!!!";

    u8g2Fonts.setCursor(u8g2Fonts.getCursorX() + 12, u8g2Fonts.getCursorY());
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2Fonts.print("\u00b7"); // 爱心
    u8g2Fonts.setFont(FONT_TEXT);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.print(str.c_str());
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2Fonts.print("\u00b7"); // 爱心
}

bool isNight(String time) {
    uint8_t hour = time.substring(11, 13).toInt();
    return hour < 6 || hour >= 18;
}

const char* getWeatherIcon(uint16_t id, bool fill) {
    switch (id) {
    case 100: return !fill ? "\uf101" : "\uf1ac";
    case 101: return !fill ? "\uf102" : "\uf1ad";
    case 102: return !fill ? "\uf103" : "\uf1ae";
    case 103: return !fill ? "\uf104" : "\uf1af";
    case 104: return !fill ? "\uf105" : "\uf1b0";
    case 150: return !fill ? "\uf106" : "\uf1b1";
    case 151: return !fill ? "\uf107" : "\uf1b2";
    case 152: return !fill ? "\uf108" : "\uf1b3";
    case 153: return !fill ? "\uf109" : "\uf1b4";
    case 300: return !fill ? "\uf10a" : "\uf1b5";
    case 301: return !fill ? "\uf10b" : "\uf1b6";
    case 302: return !fill ? "\uf10c" : "\uf1b7";
    case 303: return !fill ? "\uf10d" : "\uf1b8";
    case 304: return !fill ? "\uf10e" : "\uf1b9";
    case 305: return !fill ? "\uf10f" : "\uf1ba";
    case 306: return !fill ? "\uf110" : "\uf1bb";
    case 307: return !fill ? "\uf111" : "\uf1bc";
    case 308: return !fill ? "\uf112" : "\uf1bd";
    case 309: return !fill ? "\uf113" : "\uf1be";
    case 310: return !fill ? "\uf114" : "\uf1bf";
    case 311: return !fill ? "\uf115" : "\uf1c0";
    case 312: return !fill ? "\uf116" : "\uf1c1";
    case 313: return !fill ? "\uf117" : "\uf1c2";
    case 314: return !fill ? "\uf118" : "\uf1c3";
    case 315: return !fill ? "\uf119" : "\uf1c4";
    case 316: return !fill ? "\uf11a" : "\uf1c5";
    case 317: return !fill ? "\uf11b" : "\uf1c6";
    case 318: return !fill ? "\uf11c" : "\uf1c7";
    case 350: return !fill ? "\uf11d" : "\uf1c8";
    case 351: return !fill ? "\uf11e" : "\uf1c9";
    case 399: return !fill ? "\uf11f" : "\uf1ca";
    case 400: return !fill ? "\uf120" : "\uf1cb";
    case 401: return !fill ? "\uf121" : "\uf1cc";
    case 402: return !fill ? "\uf122" : "\uf1cd";
    case 403: return !fill ? "\uf123" : "\uf1ce";
    case 404: return !fill ? "\uf124" : "\uf1cf";
    case 405: return !fill ? "\uf125" : "\uf1d0";
    case 406: return !fill ? "\uf126" : "\uf1d1";
    case 407: return !fill ? "\uf127" : "\uf1d2";
    case 408: return !fill ? "\uf128" : "\uf1d3";
    case 409: return !fill ? "\uf129" : "\uf1d4";
    case 410: return !fill ? "\uf12a" : "\uf1d5";
    case 456: return !fill ? "\uf12b" : "\uf1d6";
    case 457: return !fill ? "\uf12c" : "\uf1d7";
    case 499: return !fill ? "\uf12d" : "\uf1d8";
    case 500: return !fill ? "\uf12e" : "\uf1d9";
    case 501: return !fill ? "\uf12f" : "\uf1da";
    case 502: return !fill ? "\uf130" : "\uf1db";
    case 503: return !fill ? "\uf131" : "\uf1dc";
    case 504: return !fill ? "\uf132" : "\uf1dd";
    case 507: return !fill ? "\uf133" : "\uf1de";
    case 508: return !fill ? "\uf134" : "\uf1df";
    case 509: return !fill ? "\uf135" : "\uf1e0";
    case 510: return !fill ? "\uf136" : "\uf1e1";
    case 511: return !fill ? "\uf137" : "\uf1e2";
    case 512: return !fill ? "\uf138" : "\uf1e3";
    case 513: return !fill ? "\uf139" : "\uf1e4";
    case 514: return !fill ? "\uf13a" : "\uf1e5";
    case 515: return !fill ? "\uf13b" : "\uf1e6";
    case 800: return "\uf13c";
    case 801: return "\uf13d";
    case 802: return "\uf13e";
    case 803: return "\uf13f";
    case 804: return "\uf140";
    case 805: return "\uf141";
    case 806: return "\uf142";
    case 807: return "\uf143";
    case 900: return !fill ? "\uf144" : "\uf1e7";
    case 901: return !fill ? "\uf145" : "\uf1e8";
    case 999:
    default: return !fill ? "\uf146" : "\uf1e9";
    }
}

// 画天气信息
#include "API.hpp"
void draw_weather(bool partial) {
    if (partial) {
        display.setPartialWindow(calLayout.weatherX, calLayout.weatherY, calLayout.weatherW, calLayout.weatherH - 1); // 高度减1，防止干扰到其他区域颜色
        display.firstPage();
        display.fillScreen(GxEPD_WHITE);
    }

    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    if (weather_type() == 1) {
        // 实时天气
        Weather* wNow = weather_data_now();
        /* 更新排版，将字体缩小，与每日天气风格一致
        // 天气图标
        u8g2Fonts.setFont(u8g2_font_qweather_icon_16);
        u8g2Fonts.setCursor(calLayout.weatherX + 2, calLayout.weatherY + 30);
        u8g2Fonts.print(getWeatherIcon(wNow->icon, isNight(wNow->time)));

        // 天气文字
        u8g2Fonts.setFont(FONT_TEXT);
        uint16_t w1 = u8g2Fonts.getUTF8Width(wNow->text.c_str());
        u8g2Fonts.setCursor(calLayout.weatherX + 30, calLayout.weatherY + 18);
        u8g2Fonts.print((wNow->text).c_str());

        // 温湿度
        u8g2Fonts.setFont(u8g2_font_tenthinnerguys_tf);
        u8g2Fonts.setCursor(calLayout.weatherX + 30, calLayout.weatherY + 35);
        u8g2Fonts.printf("%d°C | %d%%", wNow->temp, wNow->humidity);

        // 风向级别
        u8g2Fonts.setCursor(calLayout.weatherX, calLayout.weatherY + calLayout.weatherH - 6);
        u8g2Fonts.setFont(FONT_TEXT);
        u8g2Fonts.printf("%s", wNow->windDir.c_str());
        u8g2Fonts.setFont(u8g2_font_fub14_tn);
        u8g2Fonts.printf(" %d ", wNow->windScale);
        u8g2Fonts.setFont(FONT_TEXT);
        u8g2Fonts.printf("级");
        */
        // 天气图标
        u8g2Fonts.setFont(u8g2_font_qweather_icon_16);
        u8g2Fonts.setCursor(calLayout.weatherX, calLayout.weatherY + 44);
        u8g2Fonts.print(getWeatherIcon(wNow->icon, isNight(wNow->time)));

        // 天气文字
        u8g2Fonts.setFont(FONT_SUB);
        uint16_t w1 = u8g2Fonts.getUTF8Width(wNow->text.c_str());
        u8g2Fonts.setCursor(calLayout.weatherX + 28, calLayout.weatherY + 22);
        u8g2Fonts.print(wNow->text.c_str());

        u8g2Fonts.setFont(u8g2_font_tenthinnerguys_tf);
        u8g2Fonts.setCursor(calLayout.weatherX + 28, calLayout.weatherY + 37);
        u8g2Fonts.printf("%d°C | %d%%", wNow->temp, wNow->humidity);

        // 风向级别
        u8g2Fonts.setCursor(calLayout.weatherX + 28, calLayout.weatherY + calLayout.weatherH - 6);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.printf("%s", wNow->windDir.c_str());
        u8g2Fonts.setCursor(u8g2Fonts.getCursorX() + 5, u8g2Fonts.getCursorY());
        u8g2Fonts.setFont(u8g2_font_tenthinnerguys_tf);
        u8g2Fonts.printf("%d", wNow->windScale);
        u8g2Fonts.setCursor(u8g2Fonts.getCursorX() + 5, u8g2Fonts.getCursorY());
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.printf("级");
    } else {
        // 每日天气
        DailyForecast* wFc = weather_data_daily();
        DailyWeather* dw = wFc->weather;
        if (wFc->length == 0) {
            return;
        }
        DailyWeather wToday = dw[0];

        // 天气图标
        u8g2Fonts.setFont(u8g2_font_qweather_icon_16);
        u8g2Fonts.setCursor(calLayout.weatherX, calLayout.weatherY + 44);
        u8g2Fonts.print(getWeatherIcon(wToday.iconDay, false));

        // 天气文字
        u8g2Fonts.setFont(FONT_SUB);
        uint16_t w1 = u8g2Fonts.getUTF8Width(wToday.textDay.c_str());
        u8g2Fonts.setCursor(calLayout.weatherX + 28, calLayout.weatherY + 22);
        u8g2Fonts.print((wToday.textDay).c_str());

        u8g2Fonts.setFont(u8g2_font_tenthinnerguys_tf);
        u8g2Fonts.setCursor(u8g2Fonts.getCursorX() + 5, calLayout.weatherY + 22);
        u8g2Fonts.printf("%d%%", wToday.humidity);

        // 温度
        u8g2Fonts.setFont(u8g2_font_tenthinnerguys_tf);
        u8g2Fonts.setCursor(calLayout.weatherX + 28, calLayout.weatherY + 37);
        u8g2Fonts.printf("%d - %d°C", wToday.tempMin, wToday.tempMax);

        // 风向级别
        u8g2Fonts.setCursor(calLayout.weatherX + 28, calLayout.weatherY + calLayout.weatherH - 6);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.printf("%s", wToday.windDirDay.c_str());
        u8g2Fonts.setFont(u8g2_font_tenthinnerguys_tf);
        u8g2Fonts.printf(" %d ", wToday.windScaleDay);
        u8g2Fonts.setFont(FONT_SUB);
        u8g2Fonts.printf("级");
    }

    if (partial) {
        display.nextPage();
    }
}

// Draw err
void draw_err(bool partial) {
    if (partial) {
        display.setPartialWindow(380, 0, 20, 20);
        display.firstPage();
        display.fillScreen(GxEPD_WHITE);
    }
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setFont(u8g2_font_open_iconic_all_2x_t);
    u8g2Fonts.setCursor(382, 18);
    u8g2Fonts.print("\u0118");

    if (partial) {
        display.nextPage();
    }
}

///////////// Calendar //////////////
/**
 * 处理日历信息
 */
void si_calendar() {
    _calendar_status = 0;

    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    int32_t _calendar_date = pref.getInt(PREF_SI_CAL_DATE, 0);
    _cd_day_label = pref.getString(PREF_CD_DAY_LABLE, "");
    _cd_day_date = pref.getString(PREF_CD_DAY_DATE, "");
    _tag_days_str = pref.getString(PREF_TAG_DAYS, "");
    _week_1st = pref.getString(PREF_SI_WEEK_1ST, "0").toInt();
    pref.end();

    time_t now = 0;
    time(&now);
    localtime_r(&now, &tmInfo); // 时间戳转化为本地时间结构
    Serial.printf("System Time: %d-%02d-%02d %02d:%02d:%02d\n", (tmInfo.tm_year + 1900), tmInfo.tm_mon + 1, tmInfo.tm_mday, tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);

    // 如果当前时间无效
    if (tmInfo.tm_year + 1900 < 2025) {
        bool isSetOK = false;
        if (weather_status() == 1) {
            // 尝试使用api获取的时间
            String apiTime;
            Weather* weatherNow = weather_data_now();
            if (weatherNow->updateTime == NULL) {
                // TODO 处理每日天气
                DailyForecast* wFc = weather_data_daily();
                apiTime = wFc->updateTime;
            } else {
                apiTime = weatherNow->updateTime;
            }
            Serial.printf("API Time: %s\n", apiTime.c_str());
            tmInfo = { 0 }; // 重置为0
            if (strptime(apiTime.c_str(), "%Y-%m-%dT%H:%M", &tmInfo) != NULL) {  // 将时间字符串转成tm时间 e.g. 2024-11-14T17:36+08:00
                time_t set = mktime(&tmInfo);
                timeval tv;
                tv.tv_sec = set;
                settimeofday(&tv, nullptr);
                isSetOK = true;
                Serial.println("WARN: Set system time by api time.");
            } else {
                Serial.println("ERR: Fail to format api time.");
            }
        } else {
            // 如果天气也未获取成功，那么返回
            Serial.println("ERR: invalid time & not weather info got.");
        }
        if (!isSetOK) {
            _calendar_status = 2;
            return;
        }
    }

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmInfo);
    Serial.printf("Calendar Show Time: %s\n", buffer);

    nl_month_days(tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, lunarDates);
    nl_year_jq(tmInfo.tm_year + 1900, jqAccDate);

    _calendar_status = 1;
    return;
}

int si_calendar_status() {
    return _calendar_status;
}

///////////// Screen //////////////
/**
 * 屏幕刷新
 */
void task_screen(void* param) {
    Serial.println("[Task] screen update begin...");

#ifdef ESP32C3_BUILD
    // ESP32-C3需要手动配置SPI引脚
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
    Serial.printf("ESP32-C3 SPI初始化: CLK=%d, MOSI=%d, CS=%d\n", 
                  SPI_CLK_PIN, SPI_MOSI_PIN, EPD_CS_PIN);
#endif

    display.init(115200);          // 串口使能 初始化完全刷新使能 复位时间 ret上拉使能
    display.setRotation(ROTATION); // 设置屏幕旋转1和3是横向  0和2是纵向
    u8g2Fonts.begin(display);

    init_cal_layout_size();
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    draw_cal_layout();
    draw_cal_days(false);
    draw_cal_year(false);

    // 倒计日
    draw_cd_day(_cd_day_label, _cd_day_date);

    if (weather_status() == 1) {
        draw_weather(false);
    } else if (weather_status() == 2) {
        draw_err(false);
    }

    display.display();

    int32_t _calendar_date = (tmInfo.tm_year + 1900) * 10000 + (tmInfo.tm_mon + 1) * 100 + tmInfo.tm_mday;

    Preferences pref;
    pref.begin(PREF_NAMESPACE);
    pref.putInt(PREF_SI_CAL_DATE, _calendar_date);
    pref.end();

    display.powerOff(); // !!!important!!!, 关闭屏幕，否则会多0.5ma的空载电流（全屏刷新的话会自动关闭，局部刷新必须手动关闭）
    Serial.println("[Task] screen update end...");

    _screen_status = 1;
    SCREEN_HANDLER = NULL;
    vTaskDelete(NULL);
}

void si_screen() {
    _screen_status = 0;
    si_calendar(); // 准备日历数据

    if (si_calendar_status() == 2) {
        Serial.println("ERR: System time prepare failed.");
        _screen_status = 2;
        return;
    }

    if (SCREEN_HANDLER != NULL) {
        vTaskDelete(SCREEN_HANDLER);
    }
    xTaskCreate(task_screen, "Screen", 4096, NULL, 2, &SCREEN_HANDLER);
}

int si_screen_status() {
    return _screen_status;
}

void print_status() {
    Serial.printf("Weather: %d\n", weather_status());
    Serial.printf("Calendar: %d\n", si_calendar_status());
    Serial.printf("Screen: %d\n", si_screen_status());
}

///////////// 开机屏幕测试 //////////////
static bool _screen_test_passed = false;

/**
 * 开机屏幕测试 - 显示简单内容确认屏幕工作正常
 */
void si_screen_test() {
    Serial.println("=== 最基础屏幕驱动测试 ===");
    
#ifdef ESP32C3_BUILD
    // ESP32-C3需要手动配置SPI引脚
    Serial.println("配置ESP32-C3 SPI引脚...");
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
    Serial.printf("ESP32-C3 SPI初始化: CLK=%d, MOSI=%d, CS=%d\n", 
                  SPI_CLK_PIN, SPI_MOSI_PIN, EPD_CS_PIN);
    delay(1000);
#endif
    
    // 最基础的显示器初始化
    Serial.println("初始化显示器...");
    display.init(115200);  // 使用默认参数
    Serial.println("显示器初始化完成");
    delay(1000);
    
    // 设置旋转
    display.setRotation(0);  // 使用默认旋转
    Serial.printf("屏幕尺寸: %d x %d\n", display.width(), display.height());
    
    // 最简单的测试 - 全屏填充
    Serial.println("测试1: 全屏白色");
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    display.display();
    delay(3000);
    
    Serial.println("测试2: 全屏黑色");
    display.fillScreen(GxEPD_BLACK);
    display.display();
    delay(3000);
    
    Serial.println("测试3: 全屏红色");
    display.fillScreen(GxEPD_RED);
    display.display();
    delay(3000);
    
    Serial.println("测试4: 简单图形");
    display.fillScreen(GxEPD_WHITE);
    
    // 绘制简单的黑色矩形
    display.fillRect(50, 50, 100, 100, GxEPD_BLACK);
    display.fillRect(200, 50, 100, 100, GxEPD_RED);
    
    // 绘制边框
    display.drawRect(0, 0, display.width(), display.height(), GxEPD_BLACK);
    
    display.display();
    delay(3000);
    
    Serial.println("测试5: 连续刷新测试");
    for(int i = 0; i < 5; i++) {
        Serial.printf("刷新 %d/5\n", i+1);
        
        display.fillScreen(GxEPD_WHITE);
        
        // 绘制移动的矩形
        int x = 50 + i * 50;
        display.fillRect(x, 100, 50, 50, GxEPD_BLACK);
        display.fillRect(x, 200, 50, 50, GxEPD_RED);
        
        display.display();
        delay(2000);
    }
    
    // 最终测试 - 清屏
    Serial.println("最终测试: 清屏");
    display.fillScreen(GxEPD_WHITE);
    display.display();
    
    // 关闭电源
    display.powerOff();
    
    _screen_test_passed = true;
    Serial.println("=== 基础屏幕驱动测试完成 ===");
}

/**
 * 检查屏幕测试是否通过
 */
bool si_screen_test_passed() {
    return _screen_test_passed;
}

///////////// WiFi配网引导屏幕 //////////////

/**
 * 显示WiFi配网引导界面
 */
void si_show_wifi_config_guide() {
    Serial.println("=== 显示WiFi配网引导界面 ===");
    
#ifdef ESP32C3_BUILD
    // ESP32-C3需要手动配置SPI引脚
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    // 初始化显示器
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    // 清屏并设置全屏刷新
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    
    // 设置字体和颜色
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 绘制边框
    display.drawRect(10, 10, display.width()-20, display.height()-20, GxEPD_BLACK);
    
    // 显示标题
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(80, 80);
    u8g2Fonts.print("J-Calendar");
    
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(90, 110);
    u8g2Fonts.print("电子日历");
    
    // 显示配网提示
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(50, 160);
    u8g2Fonts.print("需要配置WiFi网络");
    
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(30, 190);
    u8g2Fonts.print("双击按钮启动配网模式");
    
    u8g2Fonts.setCursor(30, 210);
    u8g2Fonts.print("或连接已有WiFi网络");
    
    // 刷新显示
    display.display();
    display.powerOff();
    
    Serial.println("WiFi配网引导界面显示完成");
}

/**
 * 显示WiFi连接状态界面
 */
void si_show_wifi_connecting(const char* ssid) {
    Serial.printf("=== 显示WiFi连接状态: %s ===\n", ssid);
    
#ifdef ESP32C3_BUILD
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    // 局部刷新 - 只更新状态区域
    display.setPartialWindow(0, 0, display.width(), 100);
    display.fillScreen(GxEPD_WHITE);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 连接状态
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(30, 30);
    u8g2Fonts.print("📶 正在连接WiFi...");
    
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(30, 55);
    u8g2Fonts.printf("网络: %s", ssid);
    
    // 动态指示器
    static int indicator = 0;
    u8g2Fonts.setCursor(30, 75);
    for(int i = 0; i < 3; i++) {
        if(i == indicator % 3) {
            u8g2Fonts.print("●");
        } else {
            u8g2Fonts.print("○");
        }
        u8g2Fonts.print(" ");
    }
    indicator++;
    
    display.display(true); // 局部刷新
    // 不关闭电源，保持连接状态显示
}

/**
 * 显示WiFi连接失败界面
 */
void si_show_wifi_failed() {
    Serial.println("=== 显示WiFi连接失败界面 ===");
    
    // 局部刷新状态区域
    display.setPartialWindow(0, 0, display.width(), 100);
    display.fillScreen(GxEPD_WHITE);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(30, 30);
    u8g2Fonts.print("❌ WiFi连接失败");
    
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(30, 55);
    u8g2Fonts.print("双击按钮重新配置网络");
    
    u8g2Fonts.setCursor(30, 75);
    u8g2Fonts.print("10秒后进入休眠模式");
    
    display.display(true); // 局部刷新
    display.powerOff();
}

/**
 * 显示配网模式启动界面
 */
void si_show_config_mode() {
    Serial.println("=== 显示配网模式启动界面 ===");
    
#ifdef ESP32C3_BUILD
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    
    // 绘制边框
    display.drawRect(10, 10, display.width()-20, display.height()-20, GxEPD_BLACK);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 显示WiFi图标和标题
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(70, 70);
    u8g2Fonts.print("配网模式");
    
    // 显示状态信息
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(60, 110);
    u8g2Fonts.print("WiFi热点已启动");
    
    // 显示连接信息
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(30, 150);
    u8g2Fonts.print("热点名称: J-Calendar");
    
    u8g2Fonts.setCursor(30, 175);
    u8g2Fonts.print("热点密码: password");
    
    u8g2Fonts.setCursor(30, 200);
    u8g2Fonts.print("配置地址: 192.168.4.1");
    
    // 显示操作提示
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(20, 230);
    u8g2Fonts.print("请连接热点并访问配置页面");
    
    display.display();
    display.powerOff();
    
    Serial.println("配网模式启动界面显示完成");
}

/**
 * 显示配网超时界面
 */
void si_show_config_timeout() {
    Serial.println("=== 显示配网超时界面 ===");
    
#ifdef ESP32C3_BUILD
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    
    // 绘制边框
    display.drawRect(10, 10, display.width()-20, display.height()-20, GxEPD_BLACK);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 显示超时标题
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(80, 70);
    u8g2Fonts.print("配网超时");
    
    // 显示超时信息
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(50, 110);
    u8g2Fonts.print("配网模式已超时");
    
    u8g2Fonts.setCursor(60, 140);
    u8g2Fonts.print("设备即将休眠");
    
    // 显示操作提示
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(20, 180);
    u8g2Fonts.print("唤醒后可重新双击按钮配网");
    
    u8g2Fonts.setCursor(30, 200);
    u8g2Fonts.print("或连接已有WiFi网络");
    
    u8g2Fonts.setCursor(40, 220);
    u8g2Fonts.print("按GPIO0唤醒设备");
    
    display.display();
    display.powerOff();
    
    Serial.println("配网超时界面显示完成");
}

/**
 * 显示配网倒计时界面
 */
void si_show_config_countdown(int remainingMinutes) {
    Serial.printf("=== 显示配网倒计时界面: %d分钟 ===\n", remainingMinutes);
    
#ifdef ESP32C3_BUILD
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    // 局部刷新 - 只更新倒计时区域
    display.setPartialWindow(0, 200, display.width(), 50);
    display.fillScreen(GxEPD_WHITE);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 显示倒计时
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(30, 230);
    u8g2Fonts.printf("配网剩余时间: %d分钟", remainingMinutes);
    
    display.display(true); // 局部刷新
    // 不关闭电源，保持显示
    
    Serial.println("配网倒计时界面显示完成");
}

/**
 * 显示启动等待界面
 */
void si_show_startup_waiting(int remainingSeconds) {
    Serial.printf("=== 显示启动等待界面: %d秒 ===\n", remainingSeconds);
    
#ifdef ESP32C3_BUILD
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    
    // 绘制边框
    display.drawRect(10, 10, display.width()-20, display.height()-20, GxEPD_BLACK);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 显示标题
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(80, 70);
    u8g2Fonts.print("设备启动");
    
    // 显示等待信息
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(50, 110);
    u8g2Fonts.print("等待用户操作中");
    
    u8g2Fonts.setCursor(60, 140);
    u8g2Fonts.printf("倒计时: %d秒", remainingSeconds);
    
    // 显示操作提示
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(20, 180);
    u8g2Fonts.print("双击按钮可进入配网模式");
    
    u8g2Fonts.setCursor(30, 200);
    u8g2Fonts.print("或等待自动进入休眠");
    
    u8g2Fonts.setCursor(40, 220);
    u8g2Fonts.print("按GPIO0可唤醒设备");
    
    display.display();
    display.powerOff();
    
    Serial.println("启动等待界面显示完成");
}

/**
 * 显示WiFi连接失败重试界面
 */
void si_show_wifi_retry() {
    Serial.println("=== 显示WiFi连接失败重试界面 ===");
    
#ifdef ESP32C3_BUILD
    SPI.begin(SPI_CLK_PIN, -1, SPI_MOSI_PIN, EPD_CS_PIN);
#endif
    
    display.init(115200, true, 2, false);
    display.setRotation(ROTATION);
    u8g2Fonts.begin(display);
    
    display.setFullWindow();
    display.fillScreen(GxEPD_WHITE);
    
    // 绘制边框
    display.drawRect(10, 10, display.width()-20, display.height()-20, GxEPD_BLACK);
    
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    // 显示标题
    u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2Fonts.setCursor(70, 70);
    u8g2Fonts.print("WiFi连接失败");
    
    // 显示失败信息
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2Fonts.setCursor(50, 110);
    u8g2Fonts.print("网络连接失败");
    
    u8g2Fonts.setCursor(60, 140);
    u8g2Fonts.print("60秒后重试");
    
    // 显示操作提示
    u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2Fonts.setCursor(20, 180);
    u8g2Fonts.print("可能原因：密码错误、信号弱");
    
    u8g2Fonts.setCursor(30, 200);
    u8g2Fonts.print("双击按钮重新配置网络");
    
    u8g2Fonts.setCursor(40, 220);
    u8g2Fonts.print("或等待自动重试");
    
    display.display();
    display.powerOff();
    
    Serial.println("WiFi连接失败重试界面显示完成");
}

