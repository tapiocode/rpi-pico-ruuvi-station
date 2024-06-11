/**
 * Copyright (c) 2024 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#define BUTTON_PIN 16
#define LED_PIN 22
#define I2C_SDA 18
#define I2C_SCL 19

#define RE_5_ENABLED 1
#include "ruuvi_endpoint_5.h"
#include "ruuvi_endpoints.h"

#include "btstack_run_loop.h"
#include "btstack.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "button/button.h"
#include "led/led.h"
#include "ruuvitag_reader/ruuvitag_reader.h"

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    if (rt_process_packet(packet_type, packet) == 0) {
        led_flash();
        rt_refresh_display();
    }
}

static void gap_le_advertisements_setup(void) {
    // Active scanning, 100% (scan interval = scan window)
    gap_set_scan_parameters(1, 48, 48);
    gap_start_scan();

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
}

static int init_bluetooth() {
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("Failed to initialise cyw43_arch\n");
        return -1;
    }
    gap_le_advertisements_setup();
    hci_power_control(HCI_POWER_ON);
    return 0;
}

int main() {
    stdio_init_all();
    if (init_bluetooth() < 0) return -1;
    rt_init(I2C_SDA, I2C_SCL);
    led_init(LED_PIN);
    init_button(BUTTON_PIN, &rt_cycle_tag);

    btstack_run_loop_execute();
}
