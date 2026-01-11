#ifndef SD_SPI_H_
#define SD_SPI_H_


#include "hal/spi/spi.h"


/*! \brief Struct for configuring the module for SD card slot on board
* \param spi            Pointer to SPI interface
* \param gpio_cs        Chip Select GPIO pin
* \param card_available Boolean if card is available
* \param mount_done     Boolean if mount done flag
* \param init_done      Initialization flag
*/
typedef struct {
    spi_rp2_t *spi;
    uint8_t gpio_cs;
    bool card_available;    
    bool mount_done;
    bool init_done;
} sd_t;


/*! \brief Function for detecting if card is available
* \param config    Pointer to SD card configuration struct
* \return          Is SD card available?
*/
bool sd_detected(sd_t *config);

/*! \brief Initialization of SD card using SPI interface
* \param config    Pointer to SD card configuration struct
* \return          Initialization success flag
*/
bool sd_init(sd_t *config);


bool sd_get_state(sd_t *config);

/*! \brief Function for mounting to the device using SPI interface
* \param config    Pointer to SD card configuration struct
* \return          Mounting success flag
*/
bool sd_mount(sd_t *config);


bool sd_create_file(sd_t *config, const char *filename);


bool sd_open_file(sd_t *config, const char *filename);


bool sd_write_content(sd_t *config, const char *content);


bool sd_read_content(sd_t *config, char *buffer, size_t buf_size);


bool sd_close_file(sd_t *config);


bool sd_unmount(sd_t *config);


#endif
