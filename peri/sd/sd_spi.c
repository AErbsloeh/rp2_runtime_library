#include "peri/sd/sd_spi.h"
#include "ff.h"
#include "f_util.h"
#include "sd_card.h"
#include "SPI/sd_card_spi.h"
#include "hardware/spi.h"


// ==================================== SD CARD INSTANCE =============================================
static spi_t sd_spi = {
    .miso_gpio = PICO_DEFAULT_SPI_RX_PIN,
    .mosi_gpio = PICO_DEFAULT_SPI_TX_PIN,
    .sck_gpio = PICO_DEFAULT_SPI_SCK_PIN,
    .baud_rate = 1 * 1000 * 1000, // 1 MHz
    .spi_mode = 0,
    .hw_inst = PICO_DEFAULT_SPI, 
    .no_miso_gpio_pull_up = true,
    .set_drive_strength = false,
    .use_static_dma_channels = false
};
static sd_spi_if_t spi_if = {
    .spi = &sd_spi,
    .set_drive_strength = false,
    .ss_gpio = 0
};
static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if, 
    .use_card_detect = false
};

// =================================== LOCAL FUNCS AND VARS =========================================
static FATFS fs;
static FIL fil;
static FRESULT fr;
static UINT bw;

size_t sd_get_num(){
     return 1; 
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num == 0) {
        return &sd_card;
    } else {
        return NULL;
    }
}

// ==================================== CALLABLE FUNCS ==============================================
bool sd_detected(sd_t *config){
    return sd_card.sd_test_com;
}

bool sd_init(sd_t *config){
    uint baudrate_originial = config->spi->fspi_khz * 1000;

    sd_spi.hw_inst = config->spi->spi_mod;
    sd_spi.sck_gpio = config->spi->pin_sclk;
    sd_spi.mosi_gpio = config->spi->pin_mosi;
    sd_spi.miso_gpio = config->spi->pin_miso;
    sd_spi.baud_rate = 1000000;
    sd_spi.spi_mode = config->spi->mode;
    spi_if.ss_gpio = config->gpio_cs;

    // Set as input for testing if card is present
    gpio_init(config->gpio_cs);
    gpio_pull_up(config->gpio_cs);
    gpio_set_dir(config->gpio_cs, GPIO_OUT);
    gpio_put(config->gpio_cs, true);
    sleep_ms(1);

    if(!config->spi->init_done){
        configure_spi_module(config->spi, false);
    }
    printf("SD Status: %d\n", sd_card.state.m_Status);
    config->init_done = sd_init_driver();
    if(config->init_done){
        config->card_available = true; //sd_card.state.card_type != SDCARD_NONE && sd_card.state.card_type != CARD_UNKNOWN;
    } else {
        config->card_available = false;
    }
    printf("SD Card Type: %d\n", sd_card.state.card_type);
    printf("SD Status: %d\n", sd_card.state.m_Status);

    sd_spi.baud_rate = baudrate_originial;
    return config->init_done;
}


bool sd_get_state(sd_t *config){
    sd_card_state_t state = sd_card.state;
    return false;
}


bool sd_mount(sd_t *config){
    if(!config->init_done){
        sd_init(config);
    }

    if(config->card_available){
        fr = f_mount(&fs, "", 1);
        if(fr == FR_OK){
            config->mount_done = true;
        } else {
            config->mount_done = false;
        }
    } else {
        config->mount_done = false;
    }
    return config->mount_done;
}


bool sd_create_file(sd_t *config, const char *filename){
    if(!config->mount_done){
        return false;
    } else {
        fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(fr == FR_OK){
            f_close(&fil);
            return true;
        } else {
            return false;
        }
    }
}

bool sd_open_file(sd_t *config, const char *filename){
    if(!config->mount_done){
        return false;
    } else {
        fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
        if(fr == FR_OK && fr == FR_EXIST){
            f_close(&fil);
            return true;
        } else {
            return false;
        }
    }
}


bool sd_write_content(sd_t *config, const char *content){
    if(!config->mount_done){
        return false;
    } else {
        int num = f_printf(&fil, content);
        return num > 0;
    }
}


bool sd_read_content(sd_t *config, char *buffer, size_t buf_size){
    if(!config->mount_done){
        return false;
    } else {
        UINT br;
        fr = f_read(&fil, buffer, buf_size-1, &br);
        if(fr == FR_OK){
            buffer[br] = '\0'; // Null-terminate the string
            return true;
        } else {
            return false;
        }
    }
}


bool sd_close_file(sd_t *config){
    if(!config->mount_done){
        return false;
    } else {
        fr = f_close(&fil);
        return fr == FR_OK;
    }
}


bool sd_unmount(sd_t *config){
    f_unmount("");
    config->mount_done = false;
    return true;
}
