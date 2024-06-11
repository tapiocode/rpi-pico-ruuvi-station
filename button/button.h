
#include "pico/stdlib.h"

static void button_pressed();

void init_button(uint8_t pin, void (*callback_fn)(void));
