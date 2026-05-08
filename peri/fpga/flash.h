#ifndef FLASH_FPGA_H_
#define FLASH_FPGA_H_


#include "pico/stdlib.h"
#include "hal/spi/spi.h"


// More Informations about the flashing process
// Principle:           https://blog.classycode.com/sharing-an-spi-flash-memory-between-a-microcontroller-and-a-xilinx-7-series-fpga-with-multiboot-26dde7c05075
// AMD SPI and Flash:   https://docs.amd.com/v/u/en-US/xapp1233-spi-config-ultrascale
// Lattice SPI Flash:   https://www.latticesemi.com/en/Products/FPGAandCPLD/iCE40UltraPlus#_1583858FEF1D4406B570F0CACD485268
// ================================= DEFINITIONS =================================
/*! \brief Struct handler for configuring the multiplexer ADG1408 from Analog Devices
* \param spi            SPI instance for communicating with the FPGA Flash
* \param gpio_csn       GPIO Pin for Chip Select of the FPGA Flash
* \param gpio_progb     GPIO Pin for PROGRAM_B of the FPGA device
* \param gpio_initb     GPIO Pin for INIT_B signal of the FPGA device
* \param gpio_done      GPIO Pin for DONE signal of the FPGA device     
* \param use_initb      Boolean if FPGA device has an INIT_B pin for flashing
* \param use_done       Boolean if FPGA device has a DONE pin for checking if flashing is done
* \param flash_active   Boolean if flash process to FPGA or Flash device is actually running
* \param init_done      Boolean if device configuration is done   
* \param page_size      Integer with number of bytes in each page of the FPGA flash device   
* \param block_size     Integer with number of bytes in each block of the FPGA flash device   
*/
typedef struct{
    spi_rp2_t* spi;
    uint8_t gpio_csn;
    uint8_t gpio_initb;
    bool use_initb;
    uint8_t gpio_progb;
    uint8_t gpio_done;
    bool use_done;
    bool flash_active;    
    bool init_done;
    uint16_t page_size;
    uint16_t block_size;
} flash_fpga_t;


/*! \brief Function for configuring all GPIOs and SPI for flashing the FPGA flash
* \param config         Pointer to device struct
* \return               Bool if configuration is done
*/
bool fpga_program_init(flash_fpga_t *config);


/*! \brief Function for resetting the FPGA in order to flash the FPGA flash
* \param config         Pointer to device struct
* \return               Bool if reset was successful
*/
bool fpga_program_reset_do(flash_fpga_t *config);


/*! \brief Function for checking if the FPGA flash process is done programming
* \param config         Pointer to device struct
* \return               Bool if FPGA is done programming
*/
bool fpga_program_check_done(flash_fpga_t *config);


/*! \brief Function for getting the status register of the FPGA flash
* \param config         Pointer to device struct
* \return               Integer with the content of the status register
*/
uint16_t fpga_flash_get_status_register(flash_fpga_t *config);


/*! \brief Function for getting the Device/Manufacturer ID of the FPGA flash
* \param config         Pointer to device struct
* \return               Integer with the content of the ID register
*/
uint16_t fpga_flash_get_device_id(flash_fpga_t *config);


/*! \brief Function for getting the JEDEC ID of the FPGA flash
* \param config         Pointer to device struct
* \return               Integer with the content of the JEDEC ID register
*/
uint16_t fpga_flash_get_jedec_id(flash_fpga_t *config);


/*! \brief Function for getting the Manufacturer ID of the FPGA flash
* \param config         Pointer to device struct
* \return               Integer with the content of the Manufacturer ID register
*/
uint8_t fpga_flash_get_manufacturer_id(flash_fpga_t *config);


/*! \brief Function for getting the Device/Manufacturer ID of the FPGA flash
* \param config         Pointer to device struct
* \return               Integer with the content of the ID register
*/
uint16_t fpga_flash_get_device_id(flash_fpga_t *config);


/*! \brief Function for sending data to the FPGA flash
* \param config         Pointer to device struct
* \param start_adress   Integer with adress to start for writing
* \param data           Pointer to data buffer to be sent
* \param data_len       Length of the data buffer in bytes
* \return               Bool if sending was successful
*/
bool fpga_flash_write_data(flash_fpga_t *config, uint32_t start_adress, uint8_t* data, size_t data_len);


/*! \brief Function for sending data to the FPGA directly
* \param config         Pointer to device struct
* \param start_adress   Integer with adress to start for reading
* \param data           Pointer to data buffer to be sent
* \param data_len       Length of the data buffer in bytes
* \return               Bool if sending was successful
*/
bool fpga_flash_read_data(flash_fpga_t *config, uint32_t start_adress, uint8_t* data, size_t data_len);


/*! \brief Function for erasing the selected sector inside the FPGA flash (complete process)
* \param config         Pointer to device struct
* \param start_address  Integer with the byte-wise start address of the flash
* \return               Bool if erasing was successful
*/
bool fpga_flash_erasing_sector_complete(flash_fpga_t *config, uint32_t start_address);


/*! \brief Function for erasing the whole FPGA flash content (complete process)
* \param config         Pointer to device struct
* \return               Bool if erasing was successful
*/
bool fpga_flash_erasing_all_complete(flash_fpga_t *config);


/*! \brief Function for erasing the whole FPGA flash content (start process)
* \param config         Pointer to device struct
* \return               Bool if erasing was successful
*/
bool fpga_flash_erasing_all_start(flash_fpga_t *config);


/*! \brief Function for erasing the selected sector inside the FPGA flash (start process)
* \param config         Pointer to device struct
* \param start_address  Integer with the byte-wise start address of the flash
* \return               Bool if erasing was successful
*/
bool fpga_flash_erasing_sector_start(flash_fpga_t *config, uint32_t start_address);


/*! \brief Function for checking if the FPGA flash erasing process is done
* \param config         Pointer to device struct
* \return               Bool if erasing is done
*/
bool fpga_flash_erasing_is_done(flash_fpga_t *config);


/*! \brief Function for erasing the whole FPGA flash (end process)
* \param config         Pointer to device struct
* \return               Bool if erasing was successful
*/
bool fpga_flash_erasing_stop(flash_fpga_t *config);


#endif
