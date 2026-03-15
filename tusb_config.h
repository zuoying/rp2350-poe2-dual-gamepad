#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_MCU OPT_MCU_RP2350
#define CFG_TUSB_OS  OPT_OS_PICO
#define CFG_TUSB_DEBUG 0

// USB Device：模拟2个HID游戏手柄
#define CFG_TUD_ENABLED     1
#define CFG_TUD_HID         2
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_HID_EP_BUFSIZE 64

// USB Host：PIO USB读取真实手柄
#define CFG_TUH_ENABLED     1
#define CFG_TUH_HID         1
#define CFG_TUH_MAX_DEVICE  2
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HID_EP_BUFSIZE 64

// PIO USB配置（GPIO12=D+，GPIO13=D-）
#define PIO_USB_DP_PIN 12
#define PIO_USB_DM_PIN 13

#endif
