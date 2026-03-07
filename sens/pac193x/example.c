#include <stdio.h>
#include "sens/pac193x/pac193x.h"
#include "hal/i2c/i2c.h"
#include "hal/led/led.h"


int main(){   
    stdio_init_all();
    sleep_ms(1000);
    init_default_led();

    // --- Init of the I2C---
    static i2c_rp2_t i2c0_inst = {
        .i2c_mod = i2c1,
        .pin_sda = 6,
        .pin_scl = 7,
        .fi2c_khz = 100,
        .avai_devices = 0,
        .init_done = false,
    };
    init_i2c_module(&i2c0_inst);
    scan_i2c_bus_for_device(&i2c0_inst);

    // --- Init of the PAC193x ---
    static pac193x_t pac193x_config = {
        .i2c = &i2c0_inst,
        .gpio_pwrdwn = 255,
        .gpio_alert = 255,
        .adr = 0,
        .num_channels = 0,
        .sample_rate = 3,
        .enable_channels = true,
        .enable_sleep_mode = false,
        .enable_single_shot_mode = false,
        .init_done = false
    };
    pac193x_config.adr = pac193x_get_i2c_address(499);

    set_state_default_led(true);
    if(pac193x_init(&pac193x_config)){
        printf("Init PAC193x sensor done\n");
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
        
        pac193x_update_data_register(&pac193x_config);
        accumulation_number = pac193x_read_accumulation_number(&pac193x_config);
        for (uint8_t i = 0; i < pac193x_config.num_channels; i++){
            voltage[i] = pac193x_read_voltage(&pac193x_config, i);
            current[i] = pac193x_read_current(&pac193x_config, i);
            power[i] = pac193x_read_power(&pac193x_config, i);
            voltage_avg[i] = pac193x_read_voltage_rolling(&pac193x_config, i);
            current_avg[i] = pac193x_read_current_rolling(&pac193x_config, i);
            power_acc[i] = pac193x_read_power_accumulated(&pac193x_config, i);
        }

        printf("===========Data =======\n");
        printf("Ite #: %d\n", accumulation_number);
        for (uint8_t i = 0; i < pac193x_config.num_channels; i++){
            printf("V-CH%d: %d\n", i, voltage[i]);
            printf("I-CH%d: %d\n", i, current[i]);
            printf("P-CH%d: %d\n", i, power[i]);
            printf("V-CH%d-AVG: %d\n", i, voltage_avg[i]);
            printf("I-CH%d-AVG: %d\n", i, current_avg[i]);
            printf("P-CH%d-ACC: %llu\n", i, (unsigned long long)power_acc[i]);
        }
        toggle_state_default_led();
    };
}

