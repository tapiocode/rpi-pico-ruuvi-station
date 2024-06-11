/**
 * Copyright (c) 2024 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#include "led.h"

static uint8_t pin_led;

static int64_t turn_off_led() {
    gpio_put(pin_led, 0);
    return 0;
}

void led_init(uint8_t pin_led_in) {
    pin_led = pin_led_in;
    gpio_init(pin_led);
    gpio_set_dir(pin_led, GPIO_OUT);
    gpio_put(pin_led, 0);
}

void led_flash() {
    gpio_put(pin_led, 1);
    uint32_t alarm = add_alarm_in_ms(50, turn_off_led, NULL, 0);
}
