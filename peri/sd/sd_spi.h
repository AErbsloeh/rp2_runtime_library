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


/*! \brief Function for mounting to the device using SPI interface
* \param config     Pointer to SD card configuration struct
* \param filename   Char array / string with filename
* \return           Return if file exists
*/
bool sd_file_exists(sd_t *config, char *filename);


/*! \brief Function for creating a file on SD card
* \param config     Pointer to SD card configuration struct
* \param filename   Char array / string with filename
* \return           Return if file has been builded
*/
bool sd_create_file(sd_t *config, char *filename);


/*! \brief Function for open an already existing file on SD card
* \param config     Pointer to SD card configuration struct
* \param filename   Char array / string with filename
* \return           Return if file has been opened
*/
bool sd_open_file(sd_t *config, char *filename);


/*! \brief Function for writing content into mounted file on SD card
* \param config     Pointer to SD card configuration struct
* \param content    Char array / string with data
* \return           Return if write process is done
*/
bool sd_write_content(sd_t *config, char *content);


/*! \brief Function for reading whole content from mounted file on SD card
* \param config     Pointer to SD card configuration struct
* \param buffer     Char array / string with data
* \param buf_size   Size of the buffer
* \return           Return if read process is done
*/
bool sd_read_complete(sd_t *config, char *buffer, size_t buf_size);


/*! \brief Function for reading one line from mounted file on SD card
* \param config         Pointer to SD card configuration struct
* \param buffer         Char array / string with data
* \param buf_size       Size of the buffer
* \param bytes_offset   Unsigned integer with bytes offset to read line from file
* \return               Return if read process is done
*/
bool sd_read_line(sd_t *config, char *buffer, size_t buf_size, size_t bytes_offset);


/*! \brief Function for closing file on SD card
* \param config     Pointer to SD card configuration struct
* \return           Return if close process is done successfully
*/
bool sd_close_file(sd_t *config);


/*! \brief Function for unmounting SD card from system
* \param config     Pointer to SD card configuration struct
* \return           Return if unmounting process is done successfully
*/
bool sd_unmount(sd_t *config);


#endif
