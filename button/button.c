/**
 * Copyright (c) 2024 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#include "hardware/gpio.h"
#include "button.h"

static uint8_t button_pin;
static uint32_t button_alarm_id = 0;
static void (*button_action)(void);

static int64_t button_enable() {
    if (gpio_get(button_pin) == 0) {
        gpio_set_irq_enabled_with_callback(button_pin, GPIO_IRQ_EDGE_RISE, true, &button_pressed);
    }
}

static void button_pressed() {
    button_action();
    if (button_alarm_id > 0) cancel_alarm(button_alarm_id);
    button_alarm_id = add_alarm_in_ms(200, &button_enable, NULL, 0);
    if (button_alarm_id > 0) {
        gpio_set_irq_enabled(button_pin, GPIO_IRQ_EDGE_RISE, false);
    }
}

/**
 * Calls given callback immediately when button is pressed. Will let callback to be run again
 * only when the button has been released and then pressed again.
 *
 * @param pin
 * @param callback_when_pressed
 */
void init_button(uint8_t pin, void (*callback_when_pressed)(void)) {
    button_action = callback_when_pressed;
    button_pin = pin;
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_OUT);
    gpio_pull_up(button_pin);
    button_enable();
}
