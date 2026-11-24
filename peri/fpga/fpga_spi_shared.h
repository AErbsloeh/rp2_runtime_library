#ifndef FPGA_SPI_SHARED_H_
#define FPGA_SPI_SHARED_H_


#include "pico/stdlib.h"
#include "peri/fpga/flash.h"
#include "peri/fpga/logic.h"


/*! \brief Struct for sharing one SPI bus between the MCU, the FPGA flash and the FPGA logic
*         The lines of the FPGA flash are the configuration SPI pins of the FPGA, which are released as user I/O after configuration.
*         The MCU programs the flash (FPGA in reset) or talks to the FPGA logic (FPGA configured, flash deselected).
*         Pins, chip selects, DONE signal and the flash state are taken from the flash and logic structs.
* \param flash              Pointer to the FPGA flash configuration (SPI handler, chip select, DONE signal, flash_active)
* \param logic              Pointer to the FPGA logic configuration (chip select, number of bytes), has to use the same SPI handler
* \param fspi_khz_flash     SPI clock [in kHz] for programming the FPGA flash
* \param fspi_khz_logic     SPI clock [in kHz] for communicating with the FPGA logic
* \param attached           Boolean if the bus is attached to the FPGA logic
*/
typedef struct{
    fpga_flash_t *flash;
    fpga_logic_t *logic;
    uint16_t fspi_khz_flash;
    uint16_t fspi_khz_logic;
    bool attached;
} fpga_spi_shared_t;


/*! \brief Function for checking if the bus can be attached to the FPGA logic
* \param bus        Pointer to the shared bus handler
* \return           Bool if the flash is not using the bus and the FPGA is configured
*/
bool fpga_spi_shared_is_ready(fpga_spi_shared_t *bus);


/*! \brief Function for attaching the bus pins to the FPGA logic (flash is deselected)
* \param bus        Pointer to the shared bus handler
* \return           Bool if the bus is attached
*/
bool fpga_spi_shared_attach(fpga_spi_shared_t *bus);


/*! \brief Function for releasing all bus pins and the flash chip select (high-Z)
* \param bus        Pointer to the shared bus handler
*/
void fpga_spi_shared_detach(fpga_spi_shared_t *bus);


/*! \brief Function for sending and receiving data to the FPGA logic (attaches the bus if necessary)
* \param bus            Pointer to the shared bus handler
* \param data_tx        Data array (uint8_t) for sending data, length of logic->number_bytes
* \param data_rx        Data array (uint8_t) for getting data, length of logic->number_bytes
* \return               Bool if the transmission was done, false if the bus is not available
*/
bool fpga_spi_shared_transfer(fpga_spi_shared_t *bus, uint8_t *data_tx, uint8_t *data_rx);


#endif
