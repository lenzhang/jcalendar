# PowerShell版本的构建脚本
# 适用于Windows环境

param(
    [string]$Target = "",
    [string]$Action = ""
)

# 切换到项目根目录
Set-Location -Path "$PSScriptRoot\.."
$basedir = Get-Location

# 显示使用方法
function Show-Usage {
    Write-Host "用法："
    Write-Host "  .\build.ps1 [VERSION]           - 编译所有固件版本 (z98, z21, z15)"
    Write-Host "  .\build.ps1 esp32c3            - ESP32-C3交互式编译"
    Write-Host "  .\build.ps1 esp32c3 compile    - 直接编译ESP32-C3固件"
    Write-Host "  .\build.ps1 esp32c3 upload     - 编译并上传ESP32-C3固件"
    Write-Host ""
    Write-Host "示例："
    Write-Host "  .\build.ps1 1.0.5"
    Write-Host "  .\build.ps1 esp32c3"
    Write-Host "  .\build.ps1 esp32c3 compile"
}

# 检查PlatformIO是否安装
function Test-PlatformIO {
    try {
        pio --version | Out-Null
        return $true
    }
    catch {
        return $false
    }
}

# ESP32-C3 构建功能
function Build-ESP32C3 {
    param(
        [string]$Action = ""
    )
    
    Write-Host "=== ESP32-C3 电子墨水屏日历编译脚本 ===" -ForegroundColor Green
    Write-Host ""

    # 检查PlatformIO是否安装
    if (-not (Test-PlatformIO)) {
        Write-Host "❌ PlatformIO未安装，请先安装PlatformIO" -ForegroundColor Red
        Write-Host "安装方法: pip install platformio"
        exit 1
    }

    Write-Host "✅ PlatformIO已安装" -ForegroundColor Green

    # 显示当前配置
    Write-Host ""
    Write-Host "📋 当前配置："
    Write-Host "- 开发板: 合宙CORE-ESP32-C3"
    Write-Host "- 屏幕: 4.2寸三色电子墨水屏 (使用z21驱动)"
    Write-Host "- 接线方案: 引脚19-24连续接线"
    Write-Host ""

    # 显示引脚配置
    Write-Host "🔌 引脚配置："
    Write-Host "- VCC    -> 引脚18 (3.3V)"
    Write-Host "- GND    -> 引脚17 (GND)"
    Write-Host "- CLK    -> 引脚19 (GPIO2)"
    Write-Host "- MOSI   -> 引脚20 (GPIO3)"
    Write-Host "- RST    -> 引脚21 (GPIO10)"
    Write-Host "- DC     -> 引脚22 (GPIO6)"
    Write-Host "- CS     -> 引脚23 (GPIO7)"
    Write-Host "- BUSY   -> 引脚24 (GPIO11)"
    Write-Host ""

    # 根据参数决定操作
    switch ($Action) {
        "compile" { Compile-ESP32C3 }
        "upload" { Compile-And-Upload-ESP32C3 }
        default { Show-ESP32C3Menu }
    }
}

# ESP32-C3 交互式菜单
function Show-ESP32C3Menu {
    Write-Host "请选择操作："
    Write-Host "1. 编译固件"
    Write-Host "2. 上传固件"
    Write-Host "3. 监控串口"
    Write-Host "4. 运行引脚测试"
    Write-Host "5. 清理编译文件"
    Write-Host "6. 完整编译并上传"
    Write-Host ""

    $choice = Read-Host "请输入选择 (1-6)"

    switch ($choice) {
        "1" { Compile-ESP32C3 }
        "2" { Upload-ESP32C3 }
        "3" { Monitor-ESP32C3 }
        "4" { Test-ESP32C3 }
        "5" { Clean-ESP32C3 }
        "6" { Full-Build-ESP32C3 }
        default {
            Write-Host "❌ 无效选择" -ForegroundColor Red
            exit 1
        }
    }
}

# ESP32-C3 编译功能
function Compile-ESP32C3 {
    Write-Host "🔨 开始编译ESP32-C3固件..." -ForegroundColor Yellow
    
    $result = & pio run -e esp32c3
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ 编译成功！" -ForegroundColor Green
        
        # 创建dist目录
        if (-not (Test-Path "$basedir\dist")) {
            New-Item -ItemType Directory -Path "$basedir\dist" | Out-Null
        }
        
        # 复制固件到dist目录
        $firmwarePath = "$basedir\.pio\build\esp32c3\firmware.bin"
        if (Test-Path $firmwarePath) {
            Copy-Item $firmwarePath "$basedir\dist\jcalendar_esp32c3.bin"
            Write-Host "📦 固件已保存到: dist\jcalendar_esp32c3.bin" -ForegroundColor Green
        }
    }
    else {
        Write-Host "❌ 编译失败，请检查错误信息" -ForegroundColor Red
        exit 1
    }
}

# ESP32-C3 上传功能
function Upload-ESP32C3 {
    Write-Host "📤 开始上传固件..." -ForegroundColor Yellow
    Write-Host "请确保ESP32-C3开发板已连接到电脑"
    Read-Host "按Enter继续..."
    
    $result = & pio run -e esp32c3 -t upload
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ 上传成功！" -ForegroundColor Green
    }
    else {
        Write-Host "❌ 上传失败，请检查连接和端口" -ForegroundColor Red
        exit 1
    }
}

# ESP32-C3 串口监控
function Monitor-ESP32C3 {
    Write-Host "📺 开始监控串口..." -ForegroundColor Yellow
    Write-Host "波特率: 115200"
    Write-Host "按 Ctrl+C 退出监控"
    & pio device monitor -b 115200
}

# ESP32-C3 测试
function Test-ESP32C3 {
    Write-Host "🧪 运行引脚测试..." -ForegroundColor Yellow
    & pio test -e esp32c3
}

# ESP32-C3 清理
function Clean-ESP32C3 {
    Write-Host "🧹 清理编译文件..." -ForegroundColor Yellow
    & pio run -t clean
    Write-Host "✅ 清理完成" -ForegroundColor Green
}

# ESP32-C3 完整构建
function Full-Build-ESP32C3 {
    Write-Host "🚀 完整编译并上传流程..." -ForegroundColor Yellow
    Write-Host ""
    
    Write-Host "步骤1: 清理旧文件"
    & pio run -t clean
    
    Write-Host "步骤2: 编译固件"
    & pio run -e esp32c3
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ 编译失败" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "步骤3: 上传固件"
    Write-Host "请确保ESP32-C3开发板已连接"
    Read-Host "按Enter继续上传..."
    & pio run -e esp32c3 -t upload
    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ 上传失败" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "步骤4: 监控串口输出"
    Write-Host "即将开始监控串口，观察开机屏幕测试..."
    Start-Sleep -Seconds 2
    & pio device monitor -b 115200
}

# ESP32-C3 编译并上传
function Compile-And-Upload-ESP32C3 {
    Compile-ESP32C3
    if ($LASTEXITCODE -eq 0) {
        Upload-ESP32C3
    }
}

# 标准构建功能（原有功能）
function Build-Standard {
    param(
        [string]$Version
    )
    
    if ([string]::IsNullOrEmpty($Version)) {
        Write-Host "错误: 需要版本参数 [VERSION]" -ForegroundColor Red
        Write-Host "用法: .\build.ps1 [VERSION]"
        Write-Host "   例如: .\build.ps1 1.0.5"
        exit 1
    }

    Write-Host "🔨 开始编译版本: $Version" -ForegroundColor Yellow

    # 清理dist目录
    if (Test-Path "$basedir\dist") {
        Remove-Item "$basedir\dist\*" -Force
    }
    else {
        New-Item -ItemType Directory -Path "$basedir\dist" | Out-Null
    }

    & pio run -t clean

    $env:PLATFORMIO_BUILD_FLAGS = "-DJ_VERSION=\`"$Version\`""
    & pio run -e z98 -e z21 -e z15

    if ($LASTEXITCODE -ne 0) {
        Write-Host "❌ 编译Z98固件时出错" -ForegroundColor Red
        exit 1
    }
    else {
        # 复制文件
        Copy-Item "$basedir\.pio\build\z98\bootloader.bin" "$basedir\dist\bootloader.bin"
        Copy-Item "$basedir\.pio\build\z98\partitions.bin" "$basedir\dist\partitions.bin"
        Copy-Item "$basedir\.pio\build\z98\firmware.bin" "$basedir\dist\jcalendar_${Version}_z98.bin"
        Copy-Item "$basedir\.pio\build\z15\firmware.bin" "$basedir\dist\jcalendar_${Version}_z15.bin"
        Copy-Item "$basedir\.pio\build\z21\firmware.bin" "$basedir\dist\jcalendar_${Version}_z21.bin"
        
        Write-Host "✅ 编译完成！" -ForegroundColor Green
        Write-Host "📦 固件文件："
        Write-Host "  - jcalendar_${Version}_z98.bin"
        Write-Host "  - jcalendar_${Version}_z15.bin"
        Write-Host "  - jcalendar_${Version}_z21.bin"
    }

    Write-Host ""
    Write-Host "=== 构建完成 ===" -ForegroundColor Green
    Write-Host "请提交并推送到Github"
}

### 主程序逻辑 ###

# 检查参数
if ([string]::IsNullOrEmpty($Target)) {
    Show-Usage
    exit 1
}

# 根据第一个参数选择功能
switch ($Target) {
    "esp32c3" {
        Build-ESP32C3 -Action $Action
    }
    { $_ -in @("-h", "--help", "help") } {
        Show-Usage
    }
    default {
        # 标准构建（版本号）
        Build-Standard -Version $Target
    }
}

Write-Host ""
Write-Host "💡 提示："
Write-Host "- 标准版本开机后显示静态界面"
Write-Host "- ESP32-C3版本开机后会自动执行屏幕测试"
Write-Host "- LED状态指示系统运行状态"
Write-Host "- 双击按钮进入配置模式"
Write-Host "- 详细说明请查看相关文档" 