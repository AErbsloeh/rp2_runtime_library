#ifndef AD4858_SPI_H_
#define AD4858_SPI_H_

#include <stdint.h>
#include <stdbool.h>
#include "adc/ad4858/ad4858.h"

// More information: https://www.analog.com/media/en/technical-documentation/data-sheets/ad4858.pdf
//
// This module reads AD4858 conversion data over the existing SPI register
// bus (CS/CSCK/CSDIO/CSDO) instead of the parallel CMOS/LVDS bus. After a
// conversion, all 8 channel packets are clocked out in a single ordinary
// SPI transfer.
//
// IMPORTANT HARDWARE CAVEAT: whether the conversion data actually appears
// on the pin your SPI peripheral's MISO line is wired to depends on the
// AD4858's CSDO_ON_SDO0 setting (SPI_CONFIG_D register) and on your PCB
// wiring (dedicated CSDO pin vs. SDO0). This module deliberately leaves
// that routing exactly as your existing ad4858_init() already configured
// it for register access, since flipping it blindly could break whichever
// wiring you already rely on. Verify the very first read against the
// known CHx_TESTPAT values (ad4858_enable_cmos_testpattern()) before
// trusting real measurements.

#define AD4858_SPI_NUM_CHANNELS 8

/*! \brief Raw (still packed) conversion data for all 8 channels, as read
 *         back over the SPI register bus.
    \param raw              20-bit conversion result per channel (unsigned, right-aligned)
    \param or_ur_status     Overrange/underrange flag per channel (only valid for 24-/32-bit packet formats)
    \param chn_id           Channel ID reported back by the device itself, per channel (only valid for 24-/32-bit packet formats)
*/
typedef struct {
    uint32_t raw[8];
    uint8_t  or_ur_status[8];
    uint8_t  chn_id[8];
} ad4858_spi_conv_data_t;



typedef struct {
    uint8_t gpio_convert;
    uint8_t gpio_busy;
    uint8_t package_size;
} ad4858_spi_t;


/*! \brief Convenience wrapper: triggers a single conversion (CNV pulse),
 *         waits for BUSY to go low, and reads back the resulting data for
 *         all 8 channels over SPI.
 *  \param config   Pointer to the AD4858 device settings
 *  \param device   Pointer to DAQ SPI unit of the AD4858 device
 *  \param data     Pointer to the output structure to fill
 *  \return         True on success, false on timeout waiting for BUSY or on SPI error
 */
bool ad4858_spi_read_data(ad4858_t* config, ad4858_spi_t* device, ad4858_spi_conv_data_t* data);


/*! \brief Read the most recently completed conversion's data for all 8
 *         channels over the SPI bus, in a single burst transfer, and
 *         unpack it according to the currently configured packet size.
 *         Does NOT trigger a conversion or wait for BUSY -- call this only
 *         once BUSY has gone low, or use ad4858_spi_read_data() instead.
 *  \param config   Pointer to the AD4858 device settings
 *  \param device   Pointer to DAQ SPI unit of the AD4858 device
 *  \param data     Pointer to the output structure to fill
 *  \return         True on success, false on SPI error or invalid/unsupported packet size
 */
bool ad4858_spi_do_single_conversion(ad4858_t* config, ad4858_spi_t* device, ad4858_spi_conv_data_t* data);


/*! \brief Sign-extend a raw 20-bit two's-complement conversion result.
 *  \param raw20   Raw, unsigned 20-bit value (e.g. from ad4858_spi_conv_data_t.raw[])
 *  \return        Sign-extended signed result
 */
int32_t ad4858_spi_extract_result20(uint32_t raw20);


/*! \brief Switch the device's SPI register bus into single-instruction data
 *         mode, so that conversion data (instead of a single register byte)
 *         is returned by the following SPI transfer (Init)
 *  \param config   Pointer to the AD4858 device settings
 *  \param device   Pointer to DAQ SPI unit of the AD4858 device
 *  \return         True on success
 */
bool ad4858_spi_data_init(ad4858_t* config, ad4858_spi_t* device);


/*! \brief Switch the device's SPI register bus back to normal
 *         (config/register access) mode. Must be called before using any
 *         of the regular register read/write functions again. (De-Init)
 *  \param config   Pointer to the AD4858 device settings
 *  \param device   Pointer to DAQ SPI unit of the AD4858 device
 *  \return         True on success
 */
bool ad4858_spi_data_deinit(ad4858_t* config, ad4858_spi_t* device);



#endif