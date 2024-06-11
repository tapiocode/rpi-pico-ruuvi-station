/**
 * Copyright (c) 2024 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#include "pico/stdlib.h"
#include "ruuvi_endpoint_5.h"
#include "pico-ssd1306/ssd1306.h"
#include "sparkline/sparkline.h"
#include "ruuvitag_reader.h"
#include "btstack.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define RUUVITAGS_MAX_AMOUNT 9

static ssd1306_t display;
static rt_tag_t ruuvitags[RUUVITAGS_MAX_AMOUNT];
static rt_tag_t *selected_tag;
static sparkline_t sparkline = {
    .x = 0,
    .y = 24,
    .area_width = 50,
    .area_height = 16,
    .stepsize = 3,
};

static uint32_t get_sec_since_boot() {
    return to_ms_since_boot(get_absolute_time()) / 1000;
}

static void draw_sparkline(rt_tag_t *tag) {
    sparkline_clear(&display, &sparkline);
    for (int i = 0; i < RUUVITAG_LATEST_READINGS_COUNT; i++) {
        if (tag->latest_readings[i] > INT16_MIN) {
            sparkline_add_datapoint(&sparkline, tag->latest_readings[i]);
        }
    }
    sparkline_draw(&display, &sparkline);
    ssd1306_show(&display);
}

static void display_write_temperature(rt_tag_t *tag, uint8_t tag_amount) {
    char row1[7] = {0};
    char row2[21] = {0};
    char row3[21] = {0};
    char row4[21] = {0};
    long rounded_t = floor(tag->data.temperature_c);
    long decimals_t = lrintf(tag->data.temperature_c * 100) % 100;
    long max_r_t = tag->temperature_max / 100;
    long max_d_t = tag->temperature_max % 100;
    long min_r_t = tag->temperature_min / 100;
    long min_d_t = tag->temperature_min % 100;

    uint32_t diff = get_sec_since_boot() - tag->at_time_since_boot_sec;
    uint8_t hr = diff / 3600;
    uint8_t min = (diff % 3600) / 60;
    uint8_t sec = diff % 60;

    sprintf(row1, "%3d,%02dC", rounded_t, decimals_t);
    sprintf(row2, "         %02d:%02d:%02d ago", hr, min, sec);
    sprintf(row3, "MAX %d,%02d  MIN %d,%02d", max_r_t, max_d_t, min_r_t, min_d_t);
    sprintf(row4, "Tag %d / %d  %s", tag->tag_number, tag_amount, tag->name);

#ifdef PRINTF_DEBUG
    printf("\n| %s | %s | %s | %s |", row1, row2, row3, row4);
#endif

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0,  3, row1);
    ssd1306_draw_string(&display, 0, 32, 1, row2);
    ssd1306_draw_string(&display, 0, 43, 1, row3);
    ssd1306_draw_string(&display, 0, 54, 1, row4);
    ssd1306_show(&display);
}

static void bluetooth_get_complete_local_name(char *name, const uint8_t * adv_data, uint8_t adv_size) {
    ad_context_t context;
    for (   ad_iterator_init(&context, adv_size, (uint8_t *) adv_data);
            ad_iterator_has_more(&context);
            ad_iterator_next(&context)) {
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t size = ad_iterator_get_data_len(&context);
        const uint8_t *data = ad_iterator_get_data(&context);
        if (data_type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
            for (uint8_t i = 0; i < size && i < 10; i++) {
                name[i] = data[i];
            }
        }
    }
}

static void rt_push_temperature(rt_tag_t *tag, int32_t temperature) {
    // Shift latest readings to make space for newest at index 0
    for (uint8_t i = RUUVITAG_LATEST_READINGS_COUNT; i > 0; i--) {
        tag->latest_readings[i] = tag->latest_readings[i - 1];
    }
    tag->latest_readings[0] = temperature;
}

void rt_init(uint8_t pin_sda, uint8_t pin_scl) {
    i2c_init(i2c1, 400000);
    gpio_set_function(pin_sda, GPIO_FUNC_I2C);
    gpio_set_function(pin_scl, GPIO_FUNC_I2C);
    gpio_pull_up(pin_sda);
    gpio_pull_up(pin_scl);

    ssd1306_init(&display, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 10, 10,  2, "RuuviTag");
    ssd1306_draw_string(&display, 10, 30,  2, "Reader");
    ssd1306_draw_string(&display, 10, 50,  2, "v1.0");
    ssd1306_show(&display);
}

// Finds ruuvi tag by mac address
rt_tag_t *rt_find_tag(bd_addr_t mac_address) {
    for (uint8_t i = 0; i < RUUVITAGS_MAX_AMOUNT; i++) {
        if (bd_addr_cmp(mac_address, ruuvitags[i].mac_address) == 0) {
            return &ruuvitags[i];
        }
    }
    return NULL;
}

// Returns amount of ruuvi tags found
uint8_t rt_get_tag_count() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < RUUVITAGS_MAX_AMOUNT; i++) {
        if (ruuvitags[i].mac_address[0] != 0) {
            count++;
        }
    }
    return count;
}

// Finds ruuvi tag by mac address and updates name
void rt_update_name(bd_addr_t mac_address, char *name) {
    if (name[0] == 0) return;
    rt_tag_t *tag = rt_find_tag(mac_address);
    if (tag == NULL) return;
    strcpy(tag->name, name);
}

// Adds ruuvi tag to array unless it's there already, then updates data
void rt_update_data(bd_addr_t mac_address, const uint8_t *data) {
    rt_tag_t *tag = rt_find_tag(mac_address);
    if (tag == NULL) {
        for (uint8_t i = 0; i < RUUVITAGS_MAX_AMOUNT; i++) {
            if (ruuvitags[i].mac_address[0] == 0) {
                bd_addr_copy(ruuvitags[i].mac_address, mac_address);
                tag = &ruuvitags[i];
                tag->tag_number = i + 1;
                tag->temperature_max = INT16_MIN;
                tag->temperature_min = INT16_MAX;
                tag->at_time_since_boot_sec = get_sec_since_boot();
                for (uint8_t i = 0; i < RUUVITAG_LATEST_READINGS_COUNT; i++) {
                    tag->latest_readings[i] = INT16_MIN;
                }
#ifdef PRINTF_DEBUG
                printf("New tag found: %s\n", tag->mac_address);
                printf("Time since boot sec: %d\n", tag->at_time_since_boot_sec);
#endif
                break;
            }
        }
    }
    if (tag == NULL) return;

    re_status_t result = re_5_decode(data, &tag->data);
    if (result != RE_SUCCESS) {
#ifdef PRINTF_DEBUG
        printf("Failed to decode data: %d\n", result);
#endif
        return;
    }

    int16_t temperature = (int16_t) floor(tag->data.temperature_c * 100.0f);
    tag->temperature_max = MAX(tag->temperature_max, temperature);
    tag->temperature_min = MIN(tag->temperature_min, temperature);
    tag->at_time_since_boot_sec = get_sec_since_boot();
    rt_push_temperature(tag, temperature);

    if (selected_tag == NULL) {
        selected_tag = tag;
    }
}

void rt_refresh_display() {
    if (selected_tag->mac_address[0] == 0) return;
    display_write_temperature(selected_tag, rt_get_tag_count());
    draw_sparkline(selected_tag);
}

/**
 * Returns 0 if RuuviTag data was updated, otherwise 1
 */
int rt_process_packet(uint8_t packet_type, uint8_t *packet) {
    uint8_t length = gap_event_advertising_report_get_data_length(packet);
    const uint8_t *data = gap_event_advertising_report_get_data(packet);

    bd_addr_t address;
    char device_local_name[11] = {0};

    if (packet_type != HCI_EVENT_PACKET) return 1;

    if (hci_event_packet_get_type(packet) != GAP_EVENT_ADVERTISING_REPORT) return 1;

    gap_event_advertising_report_get_address(packet, address);

    uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
    if (event_type == 0x04) {
        bluetooth_get_complete_local_name(device_local_name, data, length);
        rt_update_name(address, device_local_name);
        return 1;
    } else if (event_type == 0x00) {
        uint16_t manufacturer_id = little_endian_read_16(packet, 17);
        if (manufacturer_id != BLUETOOTH_COMPANY_ID_RUUVI_INNOVATIONS_LTD) return 1;
        rt_update_data(address, data);
        return 0;
    } else {
        return 1;
    }
}

// Cycle the selected tag to the next, or the first
void rt_cycle_tag() {
    uint8_t count = rt_get_tag_count();
    if (count == 0) return;
    uint8_t next = selected_tag->tag_number;
    next = next < count ? next : 0;
    selected_tag = &ruuvitags[next];
}
