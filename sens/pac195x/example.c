#include <stdio.h>
#include "sens/pac1954/pac1954.h"
#include "hal/i2c/i2c.h"
#include "hal/led/led.h"


int main(){
    stdio_init_all();
    sleep_ms(1000);

    set_gpio_default_led(2);
    init_default_led();

    // --- Init of the I2C---
    static i2c_rp2_t i2c0_inst = {
        .i2c_mod = i2c1,
        .pin_sda = 10,
        .pin_scl = 11,
        .fi2c_khz = 100,
        .avai_devices = 0,
        .init_done = false,
    };
    init_i2c_module(&i2c0_inst);
    toggle_state_default_led();
    scan_i2c_bus_for_device(&i2c0_inst);

    // --- Init of the PAC1954 ---
    static pac1954_t sens = {
        .i2c = &i2c0_inst,
        .gpio_pwrdwn = 255,
        .gpio_alert = 255,
        .adr = 0x11,
        .num_channels = 4,
        .sample_mode = PAC1954_MODE_8SPS,
        .enable_bipolar_voltage = false,
        .enable_bipolar_current = false,
        .accum_config = PAC1954_ACCUM_POWER,
        .init_done = false
    };

    set_state_default_led(true);
    bool init_done = pac1954_init(&sens);
    if(init_done){
        printf("Init PAC1954 sensor done\n");
    } else {
        printf("Init failed");
    }

    uint32_t accumulation_number = 0;
    int16_t voltage[4] = {0};
    int16_t current[4] = {0};
    int32_t power[4] = {0};
    int16_t voltage_avg[4] = {0};
    int16_t current_avg[4] = {0};
    uint64_t power_acc[4] = {0};

    while (true) {
        sleep_ms(2000);
        if(init_done){
            pac1954_do_conversion(&sens);
            accumulation_number = pac1954_read_accumulation_number(&sens);
            for (uint8_t i = 0; i < sens.num_channels; i++){
                voltage[i] = pac1954_read_voltage_single(&sens, false, i);
                current[i] = pac1954_read_current_single(&sens, false, i);
                power[i] = pac1954_read_power_single(&sens, i);
                voltage_avg[i] = pac1954_read_voltage_single(&sens, true, i);
                current_avg[i] = pac1954_read_current_single(&sens, true, i);
                power_acc[i] = pac1954_read_power_accumulated_single(&sens, i);
            }

            printf("===========Data =======\n");
            printf("Ite #: %d\n", accumulation_number);
            for (uint8_t i = 0; i < sens.num_channels; i++){
                printf("V-CH%d: %d\n", i, voltage[i]);
                printf("I-CH%d: %d\n", i, current[i]);
                printf("P-CH%d: %d\n", i, power[i]);
                printf("V-CH%d-AVG: %d\n", i, voltage_avg[i]);
                printf("I-CH%d-AVG: %d\n", i, current_avg[i]);
                printf("P-CH%d-ACC: %llu\n", i, (unsigned long long)power_acc[i]);
            }
            toggle_state_default_led();
        };
    };
}