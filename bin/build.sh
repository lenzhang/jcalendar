#!/bin/bash

export PATH=/Users/jerry/.platformio/penv/bin:/Users/jerry/.platformio/penv:/Users/jerry/.platformio/python3/bin:$PATH

# 切换到项目根目录
cd ..
basedir=$PWD

# 显示使用方法
show_usage() {
    echo "用法："
    echo "  ./build.sh [VERSION]           - 编译所有固件版本 (z98, z21, z15)"
    echo "  ./build.sh esp32c3            - ESP32-C3交互式编译"
    echo "  ./build.sh esp32c3 compile    - 直接编译ESP32-C3固件"
    echo "  ./build.sh esp32c3 upload     - 编译并上传ESP32-C3固件"
    echo ""
    echo "示例："
    echo "  ./build.sh 1.0.5"
    echo "  ./build.sh esp32c3"
    echo "  ./build.sh esp32c3 compile"
}

# ESP32-C3 构建功能
build_esp32c3() {
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

    # 根据参数决定操作
    if [ "$2" = "compile" ]; then
        compile_esp32c3
    elif [ "$2" = "upload" ]; then
        compile_and_upload_esp32c3
    else
        # 显示交互式菜单
        show_esp32c3_menu
    fi
}

# ESP32-C3 交互式菜单
show_esp32c3_menu() {
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
            compile_esp32c3
            ;;
        2)
            upload_esp32c3
            ;;
        3)
            monitor_esp32c3
            ;;
        4)
            test_esp32c3
            ;;
        5)
            clean_esp32c3
            ;;
        6)
            full_build_esp32c3
            ;;
        *)
            echo "❌ 无效选择"
            exit 1
            ;;
    esac
}

# ESP32-C3 编译功能
compile_esp32c3() {
    echo "🔨 开始编译ESP32-C3固件..."
    pio run -e esp32c3
    if [ $? -eq 0 ]; then
        echo "✅ 编译成功！"
        # 复制固件到dist目录
        mkdir -p $basedir/dist
        if [ -f $basedir/.pio/build/esp32c3/firmware.bin ]; then
            cp $basedir/.pio/build/esp32c3/firmware.bin $basedir/dist/jcalendar_esp32c3.bin
            echo "📦 固件已保存到: dist/jcalendar_esp32c3.bin"
        fi
    else
        echo "❌ 编译失败，请检查错误信息"
        exit 1
    fi
}

# ESP32-C3 上传功能
upload_esp32c3() {
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
}

# ESP32-C3 串口监控
monitor_esp32c3() {
    echo "📺 开始监控串口..."
    echo "波特率: 115200"
    echo "按 Ctrl+C 退出监控"
    pio device monitor -b 115200
}

# ESP32-C3 测试
test_esp32c3() {
    echo "🧪 运行引脚测试..."
    pio test -e esp32c3
}

# ESP32-C3 清理
clean_esp32c3() {
    echo "🧹 清理编译文件..."
    pio run -t clean
    echo "✅ 清理完成"
}

# ESP32-C3 完整构建
full_build_esp32c3() {
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
}

# ESP32-C3 编译并上传
compile_and_upload_esp32c3() {
    compile_esp32c3
    if [ $? -eq 0 ]; then
        upload_esp32c3
    fi
}

# 标准构建功能（原有功能）
build_standard() {
    if [ $# -ne 1 ]; then
        echo "错误: 需要版本参数 [VERSION]"
        echo "用法: ./build.sh [VERSION]"
        echo "   例如: ./build.sh 1.0.5"
        exit 1
    fi

    version=$1
    echo "🔨 开始编译版本: $version"

    rm -f ./dist/*

    pio run -t clean

    export PLATFORMIO_BUILD_FLAGS=-DJ_VERSION=\\\"${version}\\\"
    pio run -e z98 -e z21 -e z15

    if [ $? -ne 0 ]; then
        echo "❌ 编译Z98固件时出错"
        exit 1
    else
        # 创建dist目录
        mkdir -p $basedir/dist
        
        # 复制文件
        cp $basedir/.pio/build/z98/bootloader.bin $basedir/dist/bootloader.bin
        cp $basedir/.pio/build/z98/partitions.bin $basedir/dist/partitions.bin
        cp $basedir/.pio/build/z98/firmware.bin $basedir/dist/jcalendar_${version}_z98.bin
        cp $basedir/.pio/build/z15/firmware.bin $basedir/dist/jcalendar_${version}_z15.bin
        cp $basedir/.pio/build/z21/firmware.bin $basedir/dist/jcalendar_${version}_z21.bin
        
        echo "✅ 编译完成！"
        echo "📦 固件文件："
        echo "  - jcalendar_${version}_z98.bin"
        echo "  - jcalendar_${version}_z15.bin"
        echo "  - jcalendar_${version}_z21.bin"
    fi

    echo ""
    echo "=== 构建完成 ==="
    echo "请提交并推送到Github"
}

### 主程序逻辑 ###
cd $basedir

# 检查参数
if [ $# -eq 0 ]; then
    show_usage
    exit 1
fi

# 根据第一个参数选择功能
case $1 in
    "esp32c3")
        build_esp32c3 "$@"
        ;;
    "-h"|"--help"|"help")
        show_usage
        ;;
    *)
        # 标准构建（版本号）
        build_standard "$@"
        ;;
esac

echo ""
echo "💡 提示："
echo "- 标准版本开机后显示静态界面"
echo "- ESP32-C3版本开机后会自动执行屏幕测试"
echo "- LED状态指示系统运行状态"
echo "- 双击按钮进入配置模式"
echo "- 详细说明请查看相关文档"