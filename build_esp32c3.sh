#!/bin/bash

# ESP32-C3 电子墨水屏日历编译脚本
# 适用于合宙CORE-ESP32-C3开发板

echo "=== ESP32-C3 电子墨水屏日历编译脚本 ==="
echo ""

# 检查PlatformIO是否安装
if ! command -v pio &> /dev/null; then
    echo "❌ PlatformIO未安装，请先安装PlatformIO"
    echo "安装方法: pip install platformio"
    exit 1
fi

echo "✅ PlatformIO已安装"

# 显示当前配置
echo ""
echo "📋 当前配置："
echo "- 开发板: 合宙CORE-ESP32-C3"
echo "- 屏幕: 4.2寸三色电子墨水屏 (使用z21驱动)"
echo "- 接线方案: 引脚19-24连续接线"
echo ""

# 显示引脚配置
echo "🔌 引脚配置："
echo "- VCC    -> 引脚18 (3.3V)"
echo "- GND    -> 引脚17 (GND)"
echo "- CLK    -> 引脚19 (GPIO2)"
echo "- MOSI   -> 引脚20 (GPIO3)"
echo "- RST    -> 引脚21 (GPIO10)"
echo "- DC     -> 引脚22 (GPIO6)"
echo "- CS     -> 引脚23 (GPIO7)"
echo "- BUSY   -> 引脚24 (GPIO11)"
echo ""

# 选择操作
echo "请选择操作："
echo "1. 编译固件"
echo "2. 上传固件"
echo "3. 监控串口"
echo "4. 运行引脚测试"
echo "5. 清理编译文件"
echo "6. 完整编译并上传"
echo ""

read -p "请输入选择 (1-6): " choice

case $choice in
    1)
        echo "🔨 开始编译ESP32-C3固件..."
        pio run -e esp32c3
        if [ $? -eq 0 ]; then
            echo "✅ 编译成功！"
        else
            echo "❌ 编译失败，请检查错误信息"
            exit 1
        fi
        ;;
    2)
        echo "📤 开始上传固件..."
        echo "请确保ESP32-C3开发板已连接到电脑"
        read -p "按Enter继续..."
        pio run -e esp32c3 -t upload
        if [ $? -eq 0 ]; then
            echo "✅ 上传成功！"
        else
            echo "❌ 上传失败，请检查连接和端口"
            exit 1
        fi
        ;;
    3)
        echo "📺 开始监控串口..."
        echo "波特率: 115200"
        echo "按 Ctrl+C 退出监控"
        pio device monitor -b 115200
        ;;
    4)
        echo "🧪 运行引脚测试..."
        pio test -e esp32c3
        ;;
    5)
        echo "🧹 清理编译文件..."
        pio run -t clean
        echo "✅ 清理完成"
        ;;
    6)
        echo "🚀 完整编译并上传流程..."
        echo ""
        echo "步骤1: 清理旧文件"
        pio run -t clean
        
        echo "步骤2: 编译固件"
        pio run -e esp32c3
        if [ $? -ne 0 ]; then
            echo "❌ 编译失败"
            exit 1
        fi
        
        echo "步骤3: 上传固件"
        echo "请确保ESP32-C3开发板已连接"
        read -p "按Enter继续上传..."
        pio run -e esp32c3 -t upload
        if [ $? -ne 0 ]; then
            echo "❌ 上传失败"
            exit 1
        fi
        
        echo "步骤4: 监控串口输出"
        echo "即将开始监控串口，观察开机屏幕测试..."
        sleep 2
        pio device monitor -b 115200
        ;;
    *)
        echo "❌ 无效选择"
        exit 1
        ;;
esac

echo ""
echo "=== 操作完成 ==="
echo ""
echo "💡 提示："
echo "- 开机后会自动执行屏幕测试"
echo "- LED状态指示系统运行状态"
echo "- 双击按钮进入配置模式"
echo "- 详细说明请查看 docs/ESP32-C3_使用说明.md" 