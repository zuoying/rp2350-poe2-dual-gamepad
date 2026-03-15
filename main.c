#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/random.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pio_usb.h"
#include "tusb.h"
#include "ws2812.pio.h"

// ==================== 硬件定义 ====================
#define USB_VBUS_EN    18  // USB-A口5V供电控制
#define WS2812_PIN     16  // RGB灯引脚
#define PIO_INST       pio0
#define PIO_SM         0
#define PIO_USB_DP     12  // PIO USB D+
#define PIO_USB_DM     13  // PIO USB D-

// ==================== 模式定义 ====================
typedef enum {
    MODE_SYNC = 0,        // 同步模式（反作弊）
    MODE_MASTER_ONLY,     // 仅主角色
    MODE_SLAVE_ONLY       // 仅副角色
} control_mode_t;

// ==================== 手柄数据结构 ====================
typedef struct {
    uint8_t  report_id;
    uint16_t buttons;
    uint8_t  lt, rt;
    int16_t  lx, ly, rx, ry;
} __attribute__((packed)) gamepad_t;

// ==================== 全局变量 ====================
gamepad_t g_real_input = {0};
control_mode_t g_current_mode = MODE_SYNC;
bool g_last_toggle_press = false;
uint8_t g_rand_strength = 30;  // 反作弊随机化强度（0-100）

// 组合键：View + Menu
#define VIEW_BTN  0x0020
#define MENU_BTN  0x0100

// ==================== RGB灯驱动 ====================
static inline uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

static inline void set_rgb_color(uint32_t grb) {
    pio_sm_put_blocking(PIO_INST, PIO_SM, grb << 8u);
}

static void update_mode_led(void) {
    switch(g_current_mode) {
        case MODE_SYNC:         set_rgb_color(rgb_to_grb(0,255,0)); break;  // 绿
        case MODE_MASTER_ONLY:  set_rgb_color(rgb_to_grb(0,0,255)); break;  // 蓝
        case MODE_SLAVE_ONLY:   set_rgb_color(rgb_to_grb(255,0,0)); break;  // 红
    }
}

// ==================== 反作弊随机化算法 ====================
static int16_t rand_offset(int16_t base, uint8_t max_offset) {
    if (max_offset == 0) return base;
    int16_t offset = (int16_t)(get_random_32() % (max_offset * 2 + 1)) - max_offset;
    int16_t res = base + offset;
    // 限制在-32768~32767范围内
    return (res < -32768) ? -32768 : (res > 32767) ? 32767 : res;
}

static uint8_t rand_trigger_offset(uint8_t base) {
    uint8_t offset = (uint8_t)(get_random_32() % (g_rand_strength / 2 + 1));
    offset = (get_random_32() % 2) ? offset : -offset;
    uint16_t res = base + offset;
    return (res > 255) ? 255 : (res < 0) ? 0 : (uint8_t)res;
}

static void apply_anti_cheat(gamepad_t *master, gamepad_t *slave) {
    // 主手柄：随机化强度减半
    uint8_t master_strength = g_rand_strength / 2;
    master->lx = rand_offset(g_real_input.lx, master_strength);
    master->ly = rand_offset(g_real_input.ly, master_strength);
    master->rx = rand_offset(g_real_input.rx, master_strength);
    master->ry = rand_offset(g_real_input.ry, master_strength);
    master->lt = rand_trigger_offset(g_real_input.lt);
    master->rt = rand_trigger_offset(g_real_input.rt);

    // 副手柄：全强度随机化
    slave->lx = rand_offset(g_real_input.lx, g_rand_strength);
    slave->ly = rand_offset(g_real_input.ly, g_rand_strength);
    slave->rx = rand_offset(g_real_input.rx, g_rand_strength);
    slave->ry = rand_offset(g_real_input.ry, g_rand_strength);
    slave->lt = rand_trigger_offset(g_real_input.lt);
    slave->rt = rand_trigger_offset(g_real_input.rt);

    // 按键延迟（模拟真人操作）
    uint32_t btn_delay = get_random_32() % (g_rand_strength * 0.5);
    sleep_ms(btn_delay);
    slave->buttons = g_real_input.buttons;
}

// ==================== 模式切换逻辑 ====================
static bool is_toggle_key_pressed(void) {
    return (g_real_input.buttons & (VIEW_BTN | MENU_BTN)) == (VIEW_BTN | MENU_BTN);
}

static void switch_control_mode(void) {
    g_current_mode = (control_mode_t)((g_current_mode + 1) % 3);
    // 切换提示：白闪3次
    for(int i=0; i<3; i++) {
        set_rgb_color(rgb_to_grb(255,255,255));
        sleep_ms(80);
        set_rgb_color(rgb_to_grb(0,0,0));
        sleep_ms(80);
    }
    update_mode_led();
}

static void build_virtual_output(gamepad_t *master, gamepad_t *slave) {
    memset(master, 0, sizeof(gamepad_t));
    memset(slave, 0, sizeof(gamepad_t));
    master->report_id = 1;
    slave->report_id = 2;

    switch(g_current_mode) {
        case MODE_SYNC:
            memcpy(master, &g_real_input, sizeof(gamepad_t));
            memcpy(slave, &g_real_input, sizeof(gamepad_t));
            apply_anti_cheat(master, slave);  // 反作弊随机化
            break;
        case MODE_MASTER_ONLY:
            memcpy(master, &g_real_input, sizeof(gamepad_t));
            master->lx = rand_offset(master->lx, g_rand_strength/2);
            break;
        case MODE_SLAVE_ONLY:
            memcpy(slave, &g_real_input, sizeof(gamepad_t));
            slave->lx = rand_offset(slave->lx, g_rand_strength);
            break;
    }
}

// ==================== Core1：PIO USB Host（读取真实手柄） ====================
void core1_main() {
    sleep_ms(10);
    // 配置PIO USB
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = PIO_USB_DP;
    pio_cfg.pin_dm = PIO_USB_DM;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    // 初始化USB Host
    tuh_init(1);

    // 持续处理Host任务
    while (1) {
        tuh_task();
    }
}

// ==================== USB Host回调 ====================
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc, uint16_t len) {
    (void)desc; (void)len;
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr; (void)instance;
    memset(&g_real_input, 0, sizeof(g_real_input));
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    (void)dev_addr; (void)instance;
    if (len <= sizeof(g_real_input)) {
        memcpy(&g_real_input, report, len);
    }
    tuh_hid_receive_report(dev_addr, instance);
}

// ==================== USB Device回调 ====================
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t rid, hid_report_type_t type, uint8_t* buf, uint16_t reqlen) {
    (void)itf; (void)rid; (void)type; (void)buf; (void)reqlen;
    return 0;
}

void tuh_hid_set_report_cb(uint8_t itf, uint8_t rid, hid_report_type_t type, uint8_t const* buf, uint16_t len) {
    (void)itf; (void)rid; (void)type; (void)buf; (void)len;
}

// ==================== 主函数 ====================
int main(void) {
    // 1. 设置系统时钟（120MHz，适配PIO USB）
    set_sys_clock_khz(120000, true);
    stdio_init_all();

    // 2. 开启USB-A口5V供电（关键！）
    gpio_init(USB_VBUS_EN);
    gpio_set_dir(USB_VBUS_EN, GPIO_OUT);
    gpio_put(USB_VBUS_EN, 1);

    // 3. 初始化RGB灯
    uint pio_offset = pio_add_program(PIO_INST, &ws2812_program);
    ws2812_program_init(PIO_INST, PIO_SM, pio_offset, WS2812_PIN, 800000, false);
    update_mode_led();

    // 4. 初始化硬件随机数（反作弊）
    get_random_32();

    // 5. 启动Core1处理PIO USB Host
    multicore_reset_core1();
    multicore_launch_core1(core1_main);

    // 6. 初始化USB Device（Type-C口）
    sleep_ms(100);
    tud_init(0);

    // 主循环
    while (1) {
        tud_task();

        // 模式切换逻辑
        bool current_toggle = is_toggle_key_pressed();
        if (current_toggle && !g_last_toggle_press) {
            switch_control_mode();
        }
        g_last_toggle_press = current_toggle;

        // 发送虚拟手柄数据
        if (tud_hid_ready()) {
            gamepad_t out1, out2;
            build_virtual_output(&out1, &out2);
            tud_hid_n_report(0, out1.report_id, &out1, sizeof(out1));
            tud_hid_n_report(1, out2.report_id, &out2, sizeof(out2));
        }

        sleep_ms(1);
    }

    return 0;
}
