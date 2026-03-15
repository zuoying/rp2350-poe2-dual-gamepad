#include "tusb.h"

// 设备描述符（Windows免驱）
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4002,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// 配置描述符：2个独立HID手柄接口
uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, 200, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), 0x81, 64, 1),
    TUD_HID_DESCRIPTOR(1, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), 0x82, 64, 1),
};

// HID报告描述符（标准Xbox手柄格式，兼容Windows）
uint8_t const desc_hid_report[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x10, 0x81, 0x02,
    0x05, 0x01, 0x19, 0x30, 0x29, 0x35,
    0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F,
    0x75, 0x10, 0x95, 0x06, 0x81, 0x02,
    0xC0
};

// 字符串描述符
char const* string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "RP2350-Gamepad",
    "POE2-Dual-Controller",
    "0001"
};

// TinyUSB回调函数
uint8_t const* tud_descriptor_device_cb(void) { return (uint8_t*)&desc_device; }
uint8_t const* tud_descriptor_configuration_cb(uint8_t idx) { (void)idx; return desc_configuration; }
uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) { (void)itf; return desc_hid_report; }

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t buf[64];
    if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) return NULL;
    
    const char* str = string_desc_arr[index];
    uint8_t len = (uint8_t)strlen(str);
    buf[0] = (uint16_t)(TUSB_DESC_STRING << 8) | (2 + len*2);
    for(uint8_t i=0; i<len; i++) buf[i+1] = str[i];
    return buf;
}
