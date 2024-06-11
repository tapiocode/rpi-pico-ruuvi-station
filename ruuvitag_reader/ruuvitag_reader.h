
#include "ruuvi_endpoint_5.h"
#include "ruuvi_endpoints.h"
#include "bluetooth.h"
#define RUUVITAG_LATEST_READINGS_COUNT 16

typedef struct {
    uint8_t tag_number;
    bd_addr_t mac_address;
    re_5_data_t data;
    uint32_t at_time_since_boot_sec;
    char name[10];
    // min and max stored with double decimals (e.g. 23.45 -> 2345)
    int16_t temperature_min;
    int16_t temperature_max;
    // Newest reading at 0
    int16_t latest_readings[RUUVITAG_LATEST_READINGS_COUNT];
} rt_tag_t;

void rt_init(uint8_t pin_sda, uint8_t pin_scl);

rt_tag_t *rt_find_tag(bd_addr_t mac_address);

uint8_t rt_get_tag_count();

void rt_update_name(bd_addr_t mac_address, char *name);

void rt_update_data(bd_addr_t mac_address, const uint8_t *data);

void rt_cycle_tag();

int rt_process_packet(uint8_t packet_type, uint8_t *packet);

void rt_refresh_display();
