#ifndef FPGA_LOGIC_H_
#define FPGA_LOGIC_H_


#include <stdio.h>
#include "hal/spi/spi.h"


/*! \brief Struct fon configuring the Logic/System programmed on Field Programmable Gate Array using a SPI interface
* \param spi            Pointer on HAL SPI
* \param number_bytes   size_t with number of bytes for transmission
* \param gpio_csn       Integer with GPIO number of the Chip Select Line
* \param gpio_rstn      Integer with GPIO number of the Reset Line
* \param gpio_rdy       Integer with GPIO number of Ready Line
* \param init_done      Boolean with state of the module initialization
*/
typedef struct {
    spi_rp2_t *spi;
    size_t number_bytes;
    uint8_t gpio_csn;
    uint8_t gpio_rstn;
    uint8_t gpio_rdy;
    bool init_done;
} fpga_logic_t;


/*! \brief Function for initialising the FPGA Communication Interface
* \param config         Pointer to FPGA configuration using SPI
* \return               Bool if initialization was successful
*/
bool fpga_logic_init(fpga_logic_t *config);


/*! \brief Function for controlling the Resest line to the FPGA directly
* \param config         Pointer to FPGA configuration using SPI
* \param enable         Boolean with doing a reset (true: low --> do reset with active low, false: high --> normal state)
* \return               Boolean with GPIO state of the reset line
*/
bool fpga_logic_reset_do(fpga_logic_t *config, bool enable);


/*! \brief Function for controlling the Resest line to the FPGA iteratively for a number of cycles
* \param config             Pointer to FPGA configuration using SPI
* \param num_iterations     Integer to repeat full reset cycles
* \return                   Boolean with GPIO state of the reset line
*/
bool fpga_logic_reset_cycle(fpga_logic_t *config, uint8_t num_iterations);


/*! \brief Function for sending data/command to FPGA (without reading)
* \param config             Pointer to FPGA configuration using SPI
* \param data               uint8_t data array with content to send
* \param length             Length of byte data array
' \param data_rx            uint8_t data array with content received
* \return                   Boolean with transmission is done successfully
*/
bool fpga_logic_send_data(fpga_logic_t *config, uint8_t *data, uint8_t *data_rx);


#endif
