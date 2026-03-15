cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

# 强制指定硬件平台（关键！适配RP2350和微雪板子）
set(PICO_PLATFORM rp2350)
set(PICO_BOARD waveshare_rp2350_usb_a)

project(poe2_dual_gamepad C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 初始化Pico SDK
pico_sdk_init()

# 第一步：先定义可执行文件（必须在pico_generate_pio_header之前）
add_executable(${PROJECT_NAME}
    main.c
    usb_descriptors.c
)

# 第二步：再生成WS2812 PIO头文件（依赖已定义的目标）
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# 链接依赖库（含PIO USB、多核、GPIO、TinyUSB）
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_multicore
    pico_pio_usb
    tinyusb_device
    tinyusb_host
    hardware_gpio
    hardware_pio
    hardware_random  # 硬件随机数（反作弊）
)

# 编译选项：开启所有警告，打印内存占用
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)

# 启用USB串口输出（调试用）
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# 生成UF2/ELF/HEX文件（可直接烧录）
pico_add_extra_outputs(${PROJECT_NAME})
