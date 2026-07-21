#ifndef AD4858_H_
#define AD4858_H_

#include "hal/spi/spi.h"


// More information: https://www.analog.com/media/en/technical-documentation/data-sheets/ad4858.pdf
// ====================== DEFINITIONS ======================
#define AD4858_VRANGE_SINGLE_2V5    0x00
#define AD4858_VRANGE_DUAL_2V5      0x01
#define AD4858_VRANGE_SINGLE_5V0    0x02
#define AD4858_VRANGE_DUAL_5V0      0x03
#define AD4858_VRANGE_SINGLE_6V25   0x04
#define AD4858_VRANGE_DUAL_6V25     0x05
#define AD4858_VRANGE_SINGLE_10V    0x06
#define AD4858_VRANGE_DUAL_10V      0x07
#define AD4858_VRANGE_SINGLE_12V5   0x08
#define AD4858_VRANGE_DUAL_12V5     0x09
#define AD4858_VRANGE_SINGLE_20V    0x0A
#define AD4858_VRANGE_DUAL_20V      0x0B
#define AD4858_VRANGE_SINGLE_25V    0x0C
#define AD4858_VRANGE_DUAL_25V      0x0D
#define AD4858_VRANGE_SINGLE_40V    0x0E
#define AD4858_VRANGE_DUAL_40V      0x0F

#define AD4858_OSR_RATIO_1          0xFF
#define AD4858_OSR_RATIO_2          0x00
#define AD4858_OSR_RATIO_4          0x01
#define AD4858_OSR_RATIO_8          0x02
#define AD4858_OSR_RATIO_16         0x03
#define AD4858_OSR_RATIO_32         0x04
#define AD4858_OSR_RATIO_64         0x05
#define AD4858_OSR_RATIO_128        0x06
#define AD4858_OSR_RATIO_256        0x07
#define AD4858_OSR_RATIO_512        0x08
#define AD4858_OSR_RATIO_1024       0x09
#define AD4858_OSR_RATIO_2048       0x0A
#define AD4858_OSR_RATIO_4096       0x0B
#define AD4858_OSR_RATIO_8192       0x0C
#define AD4858_OSR_RATIO_16384      0x0D
#define AD4858_OSR_RATIO_32768      0x0E
#define AD4858_OSR_RATIO_65536      0x0F

#define AD4858_PACKETSIZE_20BIT     0x00
#define AD4858_PACKETSIZE_24BIT     0x01
#define AD4858_PACKETSIZE_32BIT     0x02


/*! \brief Structure with Settings for handling the Analog-Digital Converter AD4858 from Analog Devices (with data packet size of 24-bit)
    \param spi_mod          SPI handler for communication with the AD4858 (MODE0, MSB true)
    \param gpio_csn         GPIO number of used CSN
    \param gpio_pwr_dwn     GPIO number of used Power Down pin (if not used, put 255)
    \param softspan_level   Softspan level / voltage range applied on all channels
    \param osr_ratio        Defining the Oversampling ratio (1 ... 65536 in two scale values)
    \param use_4wire_spi    Boolean for enabling 4-wire SPI communication
    \param use_seamless_hdr Boolean for enabling seamless HDR mode
    \param use_ref_buffer   Boolean for using the internal reference buffer
    \param use_ext_ref      Boolean for using an external reference voltage source
    \param enable_crc       Boolean for enabling CRC transmission in data
    \param init_done        Boolean if initialization is done
*/
typedef struct {
    spi_rp2_t* spi_mod;
    uint8_t gpio_csn;    
    uint8_t gpio_pwr_dwn;
    uint8_t softspan_level;
    uint8_t osr_ratio;
    bool use_seamless_hdr;
    bool use_4wire_spi;
    bool use_ref_buffer;
    bool use_ext_ref;
    bool enable_crc;
    bool init_done;
} ad4858_t;


// ====================== FUNCTIONS ======================
/*! \brief Read the device type of the ADC AD4858
 *  \param config     Pointer to AD4858 device settings
 *  \return           8-bit device type (0x07)
 */
uint8_t ad4858_get_device_type(ad4858_t* config);


/*! \brief Read the 16-bit product ID of the ADC AD4858
 *  \param config     Pointer to AD4858 device settings
 *  \return           16-bit product ID
 */
uint16_t ad4858_get_product_id(ad4858_t* config);


/*! \brief Read the 16-bit vendor ID of the ADC AD4858
 *  \param config     Pointer to AD4858 device settings
 *  \return           16-bit vendor ID
 */
uint16_t ad4858_get_vendor_id(ad4858_t* config);


/*! \brief Read the SPI status register
 *  \param config       Pointer to AD4858 device settings
 *  \return             Byte containing the current SPI status
 */
uint8_t ad4858_get_spi_status(ad4858_t* config);


/*! \brief Read the device status register
 *  \param config   Pointer to AD4858 device settings
 *  \return           Byte containing the current device status
 */
uint8_t ad4858_get_device_status(ad4858_t* config);


/*! \brief Reading the package size used in the ADC AD4858
 *  \param config       Pointer to AD4858 device settings
 *  \param packet_size  2-bit configuration to determine the packet size of the data frame
 */
void ad4858_set_packet_size(ad4858_t* config, uint8_t packet_size);


/*! \brief Reading the package size used in the ADC AD4858
 *  \param config   Pointer to AD4858 device settings
 *  \return         2-bit configuration to determine the packet size of the data frame
 */
uint8_t ad4858_get_packet_size(ad4858_t* config);


/*! \brief Configure the SoftSpan voltage range for a specific channel
 *  \param config   Pointer to AD4858 device settings
 *  \param channel    ADC channel number (0 to 7)
 *  \param lvl        SoftSpan level (4-bit value according to datasheet range table)
 */
void ad4858_set_softspan(ad4858_t* config, uint8_t channel, uint8_t lvl);


/*!
 *  \brief Set a specific channel into sleep mode
 *  \param config   Pointer to AD4858 device settings
 *  \param channel    ADC channel number (0 to 7)
 *  \param go_sleep   Boolean to set the sleep mode (true = sleep, false = active)
 */
void ad4858_set_channel_into_sleep(ad4858_t *config, uint8_t channel, bool go_sleep);


/*! \brief Configure the Power Mode of the device
 *  \param config       Pointer to AD4858 device settings
 *  \param enable       Boolean to set the power mode (true = enable, false = power_down)
 */
void ad4858_set_power_mode(ad4858_t* config, bool enable);


/*! \brief Controlling the testpattern option from SPI to CMOS/LVDS interface of the ADC AD4858
 *  \param config       Pointer to AD4858 device settings
 *  \param enable       Boolean for enabling CMOS/LVDS test pattern output
 */
void ad4858_enable_cmos_testpattern(ad4858_t* config, bool enable);


/*! \brief Send a software reset command to the ADC AD4858
 *  \param config   Pointer to AD4858 device settings
 */
void ad4858_do_reset(ad4858_t* config);


/*! \brief Function for initializing the AD4858 device
 * \param config       Pointer to the AD4858 device structure
 * \return               Bool if initialization was successful
*/
bool ad4858_init(ad4858_t *config);


#endif