#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "hal/helper/crc.h"

int main(void){
    // Example data to test
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t c1 = crc16_ccitt(data1, sizeof(data1));
    printf("data1 CRC16: 0x%04X\n", c1);
    printf("low=0x%02X\n", (uint8_t)(c1 & 0xFF));
    printf("high=0x%02X\n", (uint8_t)(c1 >> 8));

    // ASCII test string (common CRC test vector)
    const uint8_t text[] = "123456789";
    uint16_t ct = crc16_ccitt(text, sizeof(text)-1);
    printf("\"123456789\" CRC16: 0x%04X\n", ct);
    printf("low=0x%02X\n", (uint8_t)(ct & 0xFF));
    printf("high=0x%02X\n", (uint8_t)(ct >> 8));

    return 0;
}
