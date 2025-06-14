# ESP32-C3 电子墨水屏日历移植指南

## 用户最终接线方案 ⭐

**开发板**: 合宙CORE-ESP32-C3开发板  
**电子墨水屏**: 4.2" 三色电子墨水屏 GDEY042Z98 (400x300分辨率)

### 接线表

| 电子墨水屏 | 开发板引脚 | GPIO | 功能说明 |
|-----------|----------|------|----------|
| VCC       | 引脚18   | 3.3V | 电源正极 |
| GND       | 引脚17   | GND  | 电源负极 |
| CLK       | 引脚19   | GPIO2 | SPI时钟 |
| MOSI(DIN) | 引脚20   | GPIO3 | SPI数据输入 |
| RST       | 引脚21   | GPIO10 | 复位信号 |
| DC        | 引脚22   | GPIO6 | 数据/命令选择 |
| CS        | 引脚23   | GPIO7 | SPI片选 |
| BUSY      | 引脚24   | GPIO11 | 忙信号检测 (已解锁) |

### 接线优势

1. **连续引脚接线**: 所有信号线都使用引脚17-24，包括电源线，接线整齐
2. **GPIO11已解锁**: 充分利用已解锁的GPIO11作为BUSY检测引脚
3. **稳定可靠**: 避开所有特殊功能引脚，确保系统稳定性
4. **便于调试**: 所有连接集中在一个区域，便于检查和调试

## 概述

本指南将帮助你将电子墨水屏日历项目移植到合宙CORE-ESP32-C3开发板上。ESP32-C3是一款低功耗的RISC-V单核处理器，具有WiFi和蓝牙功能。

## 硬件准备

### 开发板规格
- **芯片**: ESP32-C3FH4 (RISC-V 160MHz)
- **Flash**: 4MB SPI Flash
- **GPIO**: 22个可用GPIO (GPIO0-GPIO21)
- **电源**: 3.3V工作电压
- **通信**: WiFi 802.11 b/g/n, Bluetooth 5.0

### 电子墨水屏支持
- **支持型号**: 4.2寸三色电子墨水屏 (GDEY042Z98)
- **分辨率**: 400x300像素
- **颜色**: 黑/白/红三色
- **接口**: SPI

## 引脚连接

### 推荐接线方案 (使用开发板引脚17-24)

为了接线方便，我们使用开发板上连续的引脚17-24进行连接：

| 电子墨水屏引脚 | 开发板引脚 | GPIO | 功能说明 |
|----------------|------------|------|----------|
| VCC            | 引脚18     | 3.3V | 电源正极 |
| GND            | 引脚17     | GND  | 电源负极 |
| DIN (MOSI)     | 引脚23     | GPIO7 | SPI数据输入 |
| CLK (SCK)      | 引脚22     | GPIO6 | SPI时钟 |
| CS             | 引脚24     | GPIO11 | SPI片选 (已解锁) |
| DC             | 引脚21     | GPIO10 | 数据/命令选择 |
| RST            | 引脚20     | GPIO3 | 复位信号 |
| BUSY           | 引脚19     | GPIO2 | 忙信号反馈 |

### 接线优势

1. **整齐美观**: 所有连线都在连续的引脚17-24上
2. **电源方便**: 包含3.3V电源和GND，无需额外电源线
3. **性能最佳**: 使用ESP32-C3的硬件SPI引脚
4. **GPIO11解锁**: 充分利用已解锁的GPIO11作为片选
5. **避开限制**: 完全避开特殊功能引脚

### 状态LED (可选)

如果需要状态指示，可以使用开发板的LED：

| 功能 | 开发板引脚 | GPIO | 说明 |
|------|------------|------|------|
| WiFi状态 | GPIO12 (D4) | 12 | 高电平有效 |
| 系统状态 | GPIO13 (D5) | 13 | 高电平有效 |

## 接线图

```
电子墨水屏                    合宙ESP32-C3开发板
┌─────────┐                 ┌─────────────────┐
│   VCC   │ ──────────────> │ 引脚18 (3.3V)   │
│   GND   │ ──────────────> │ 引脚17 (GND)    │
│   DIN   │ ──────────────> │ 引脚23 (GPIO7)  │
│   CLK   │ ──────────────> │ 引脚22 (GPIO6)  │
│   CS    │ ──────────────> │ 引脚24 (GPIO11) │
│   DC    │ ──────────────> │ 引脚21 (GPIO10) │
│   RST   │ ──────────────> │ 引脚20 (GPIO3)  │
│   BUSY  │ ──────────────> │ 引脚19 (GPIO2)  │
└─────────┘                 └─────────────────┘
```

## 软件配置

### 1. PlatformIO 环境设置

在 `platformio.ini` 中使用ESP32-C3配置：

```ini
[env:esp32-c3-luatos]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

monitor_speed = 115200
build_flags = 
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DSI_DRIVER=98
    -DLUATOS_CORE_ESP32C3

lib_deps = 
    adafruit/Adafruit GFX Library
    olikraus/U8g2_for_Adafruit_GFX
    zinggjm/GxEPD2
    olikraus/U8g2
    ArduinoJson
    WiFi
    HTTPClient
    Preferences
```

### 2. 引脚配置

代码中已自动使用 `esp32c3_pins.h` 中定义的引脚配置：

```cpp
// 使用引脚17-24的配置
#define EPD_CS_PIN      11    // 引脚24 - GPIO11 (已解锁)
#define EPD_DC_PIN      10    // 引脚21 - GPIO10
#define EPD_RST_PIN     3     // 引脚20 - GPIO3
#define EPD_BUSY_PIN    2     // 引脚19 - GPIO2

// SPI硬件引脚
#define SPI_MOSI_PIN    7     // 引脚23 - GPIO7
#define SPI_CLK_PIN     6     // 引脚22 - GPIO6
```

### 3. GPIO11解锁

如果你还没有解锁GPIO11，可以使用以下步骤：

```bash
# 安装esptool
pip install esptool

# 连接开发板到电脑，检查端口号 (例如 COM3 或 /dev/ttyUSB0)
# 解锁GPIO11 (注意：这是一次性操作，不可逆)
esptool.py -p COM3 burn_efuse VDD_SPI_AS_GPIO 1

# 确认操作时输入: BURN
```

## 编译和上传

### 使用PlatformIO

```bash
# 编译
pio run -e esp32-c3-luatos

# 上传
pio run -e esp32-c3-luatos -t upload

# 监控串口
pio device monitor -p COM3 -b 115200
```

### 使用Arduino IDE

1. 安装ESP32开发板包：
   - 文件 → 首选项 → 附加开发板管理器网址
   - 添加：`https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - 工具 → 开发板 → 开发板管理器 → 搜索"ESP32" → 安装

2. 选择开发板：
   - 工具 → 开发板 → ESP32 Arduino → ESP32C3 Dev Module

3. 配置参数：
   - Upload Speed: 921600
   - CPU Frequency: 160MHz
   - Flash Size: 4MB
   - Partition Scheme: Default 4MB

## 注意事项

### 电源管理
- ESP32-C3工作电压为3.3V，与电子墨水屏完全兼容
- 建议使用Type-C接口供电进行调试
- 生产环境可考虑电池供电方案

### 引脚特性
- **GPIO11**: 已解锁，可安全用作SPI片选
- **GPIO2**: 既用作BUSY检测又是SPI_MISO，但不冲突（MISO在墨水屏中不使用）
- **引脚17-24**: 连续排列，接线美观整齐
- **硬件SPI**: 使用ESP32-C3的HSPI，性能最佳

### SPI配置
- ESP32-C3使用硬件SPI2 (HSPI)
- SPI频率建议设置为1-4MHz，确保稳定性
- 自动使用正确的SPI模式和位序

### 内存优化
- ESP32-C3内存相对较小，注意优化字体和图像资源
- 可考虑使用PSRAM版本的ESP32-C3 (如果需要)

## 故障排除

### 常见问题

1. **编译错误**
   - 检查库依赖是否正确安装
   - 确认PlatformIO配置正确

2. **上传失败**
   - 检查USB连接和驱动
   - 按住BOOT键再上传
   - 检查串口端口号

3. **显示不正常**
   - 检查接线是否正确
   - 确认电子墨水屏型号匹配
   - 检查电源供电稳定性

4. **GPIO11不可用**
   - 确认已正确解锁GPIO11
   - 检查熔丝位设置

### 调试方法

```cpp
// 在代码中添加调试信息
Serial.begin(115200);
Serial.println("ESP32-C3 E-ink Calendar Starting...");

// 检查引脚状态
pinMode(EPD_CS_PIN, OUTPUT);
digitalWrite(EPD_CS_PIN, HIGH);
Serial.printf("CS Pin GPIO%d (引脚24) initialized\n", EPD_CS_PIN);
```

### 接线检查清单

- [ ] VCC连接到引脚18 (3.3V)
- [ ] GND连接到引脚17 (GND)
- [ ] DIN连接到引脚23 (GPIO7)
- [ ] CLK连接到引脚22 (GPIO6)
- [ ] CS连接到引脚24 (GPIO11)
- [ ] DC连接到引脚21 (GPIO10)
- [ ] RST连接到引脚20 (GPIO3)
- [ ] BUSY连接到引脚19 (GPIO2)

## 性能优化建议

1. **降低功耗**
   - 使用深度睡眠模式
   - 优化WiFi连接时间
   - 合理设置屏幕刷新间隔

2. **提升稳定性**
   - 添加看门狗定时器
   - 异常处理和重启机制
   - 电源滤波优化

3. **代码优化**
   - 使用较小的字体库
   - 压缩图像资源
   - 优化内存使用

## 扩展功能

利用ESP32-C3的特性，可以添加以下功能：

- **蓝牙配置**: 使用蓝牙进行WiFi配置
- **OTA升级**: 无线固件更新
- **传感器接入**: 温湿度传感器等
- **状态指示**: 使用板载LED显示状态

通过以上配置，你就可以成功将电子墨水屏日历移植到ESP32-C3开发板上了！使用引脚17-24的接线方案让连接变得非常简单和美观。 