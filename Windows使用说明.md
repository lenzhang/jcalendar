# Windows环境使用说明

## 概述

这个电子墨水屏日历项目现在完全支持Windows环境！我们提供了Windows PowerShell版本的构建脚本，让您能够在Windows系统上轻松编译和部署固件。

## 系统要求

- Windows 10 或更高版本
- PowerShell 5.1 或更高版本
- Python 3.7 或更高版本
- Git

## 安装依赖

### 1. 安装Python
如果还没有安装Python，请从 [python.org](https://www.python.org/downloads/) 下载并安装。

### 2. 安装PlatformIO
在PowerShell中运行：
```powershell
pip install platformio
```

### 3. 验证安装
```powershell
pio --version
```

## 使用方法

### 构建脚本

项目提供了两个构建脚本：

1. **`bin\build.ps1`** - Windows PowerShell版本（推荐Windows用户使用）
2. **`bin\build.sh`** - Linux/macOS Bash版本

### Windows PowerShell脚本使用

打开PowerShell，导航到项目目录，然后运行：

#### 编译标准版本
```powershell
cd bin
.\build.ps1 1.0.5
```

#### ESP32-C3交互式编译
```powershell
cd bin
.\build.ps1 esp32c3
```

#### 直接编译ESP32-C3
```powershell
cd bin
.\build.ps1 esp32c3 compile
```

#### 编译并上传ESP32-C3
```powershell
cd bin
.\build.ps1 esp32c3 upload
```

### 如果遇到执行策略问题

Windows可能阻止运行PowerShell脚本。解决方法：

1. 以管理员身份运行PowerShell
2. 执行以下命令：
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

## 支持的开发板

### 标准ESP32开发板
- **z98**: 适用于某些特定型号
- **z21**: 4.2寸三色电子墨水屏
- **z15**: 适用于某些特定型号

### ESP32-C3开发板
- **合宙CORE-ESP32-C3**: 专门优化的配置
- 使用z21驱动
- 包含详细的引脚配置

## 引脚配置 (ESP32-C3)

| 功能 | 引脚编号 | GPIO |
|------|----------|------|
| VCC  | 18       | 3.3V |
| GND  | 17       | GND  |
| CLK  | 19       | GPIO2|
| MOSI | 20       | GPIO3|
| RST  | 21       | GPIO10|
| DC   | 22       | GPIO6|
| CS   | 23       | GPIO7|
| BUSY | 24       | GPIO11|

## 编译输出

编译成功后，固件文件将保存在 `dist` 目录中：

### 标准版本
- `bootloader.bin`
- `partitions.bin`
- `jcalendar_[版本号]_z98.bin`
- `jcalendar_[版本号]_z21.bin`
- `jcalendar_[版本号]_z15.bin`

### ESP32-C3版本
- `jcalendar_esp32c3.bin`

## 故障排除

### 常见问题

1. **PlatformIO未找到**
   - 确保Python和pip正确安装
   - 重新安装PlatformIO: `pip install --upgrade platformio`

2. **串口权限问题**
   - 确保ESP32开发板正确连接
   - 检查设备管理器中的串口设备
   - 可能需要安装USB转串口驱动

3. **编译错误**
   - 确保所有依赖库都正确安装
   - 尝试清理构建缓存：`pio run -t clean`

4. **PowerShell执行策略**
   - 参考上面的执行策略设置方法

### 获取帮助

如果遇到问题，可以：

1. 查看错误日志
2. 检查PlatformIO配置
3. 确认开发板连接状态
4. 查看项目文档

## 功能特性

### 交互式菜单
ESP32-C3构建脚本提供了友好的交互式菜单：

1. 编译固件
2. 上传固件
3. 监控串口
4. 运行引脚测试
5. 清理编译文件
6. 完整编译并上传

### 自动化功能
- 自动检测PlatformIO安装
- 自动创建输出目录
- 自动复制固件文件
- 彩色输出提示

## 兼容性

✅ **完全支持Windows 10/11**
✅ **PowerShell 5.1+**
✅ **所有ESP32开发板**
✅ **所有屏幕型号**

这个项目现在是完全跨平台的，无论您使用Windows、Linux还是macOS，都能获得相同的开发体验！ 