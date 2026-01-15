#include "peri/sd/sd_spi.h"
#include <stdio.h>


int main(){   
    // Init Phase 
    stdio_init_all();

    // --- Add SD card support
    spi_rp2_t spi = {
        .pin_mosi = PICO_DEFAULT_SPI_TX_PIN,
        .pin_sclk = PICO_DEFAULT_SPI_SCK_PIN,
        .pin_miso = PICO_DEFAULT_SPI_RX_PIN,
        .spi_mod = PICO_DEFAULT_SPI,
        .fspi_khz = 16000,
        .mode = 0,
        .msb_first = true,
        .init_done = false
    };
    sd_t sd_config = {
        .spi = &spi,
        .gpio_cs = 6,
        .card_available = false,    
        .mount_done = false,
        .init_done = false
    };
    
    if(sd_init(&sd_config)){
        printf("SD driver init done\n");
        if(sd_mount(&sd_config)){
            printf("SD mount done\n");
            bool access_ready = false;
            if(sd_file_exists(&sd_config, "test.txt")){
                access_ready = sd_open_file(&sd_config, "test.txt");
                if(access_ready){
                    printf("File opened on sd card\n");
                } else {
                    printf("File not found (append)\n");
                }
            } else {
                access_ready = sd_create_file(&sd_config, "test.txt");
                if(access_ready){
                    printf("File generated on sd card\n");
                } else {
                    printf("File not found (create)\n");
                }
            }
            if(access_ready){
                if(sd_write_content(&sd_config, "123456\n")){
                    printf("Write content into file\n");
                    char sd_content[1000] = {0};
                    char sd_line[1000] = {0};

                    if(sd_read_complete(&sd_config, sd_content, sizeof(sd_content))){
                        printf("File read completely\n");
                    }
                    printf("%s", sd_content);
                    if(sd_read_line(&sd_config, sd_line, sizeof(sd_line), 0)){
                        printf("File read completely\n");
                    }
                    printf("%s", sd_line);
                }
            }
        } else {
            printf("No card available");
        }
        if(sd_close_file(&sd_config)){
            printf("Closed file\n");
        }
        if(sd_config.mount_done){
            if(sd_unmount(&sd_config)){
                printf("Unmounted sd card\n");
            }
            
        }
    }

    // Main Loop
    while (true) {  
        tight_content_loop();
    }
}
