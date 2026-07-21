#ifndef ADC4858_CMOS_H_
#define ADC4858_CMOS_H_


#include "pico/stdlib.h"


// More information: https://www.analog.com/media/en/technical-documentation/data-sheets/ad4858.pdf
//
// This module reads conversion data from the AD4858 over the parallel CMOS
// conversion data bus (8 SDO lanes + SCKI clock) using PWM peripherals and
// interrupts only (no PIO, no DMA). One PWM slice free-runs to generate the
// CNV pulses; a second PWM slice generates the SCKI clock and its wrap
// interrupt reads all 8 SDO lines on every clock edge. A GPIO interrupt on
// the falling edge of BUSY starts the SCKI slice for each new conversion,
// so the whole capture stays synchronized to the ADC without CPU polling.
//
// NOTE: Because every SCKI clock triggers a full interrupt, this approach
// cannot reach the ADC's full 1 MSPS / 8-channel throughput. It is suitable
// for low-to-moderate sample rates. For full-rate acquisition, a PIO- and
// DMA-based implementation is required instead.
//
// NOTE: LVDS vs. CMOS is a hardware pin strap on the AD4858 (LVDS/CMOS pin
// tied to VIO or GND), not a register bit. This module assumes CMOS wiring.
//
// NOTE: The exact bit ordering within a channel packet (MSB first? which
// bit carries the sign?) is only documented as a figure in the datasheet,
// not as text, and is therefore implemented here as a documented assumption.
// Verify it once against the known CHx_TESTPAT values (via
// ad4858_enable_cmos_testpattern()) before trusting real measurements.

/*! \brief Structure for controlling the CMOS interface to get data from the Analog-Digital-Converter AD4858
    \param gpio_convert         GPIO number of the used Convert (CNV) pin
    \param gpio_busy            GPIO number of the used Busy pin
    \param gpio_sdo             GPIO numbers of the 8 parallel SDO lines (SDO0..SDO7); do NOT need to be consecutive
    \param gpio_cmos_sck        GPIO number of the SCKI clock output (host -> ADC)
    \param packet_size          Packet size in bits (20/24/32); filled in automatically by ad4858_init_cmos()
    \param sampling_period_us   Period of the CNV pulse train in microseconds (generated via PWM)
    \param cmos_sclk_frequency  Target frequency of the SCKI clock in Hz (generated via PWM)
    \param pwm_slice_cnv        Internal: PWM slice used for CNV, assigned by ad4858_init_cmos()
    \param pwm_slice_sck        Internal: PWM slice used for SCKI, assigned by ad4858_init_cmos()
    \param lane_accum           Internal: running shift accumulator per SDO lane, updated from the ISR
    \param channel_raw          Internal: last complete raw packet per channel (right-aligned in packet_size bits)
    \param status_raw           Internal: last complete raw status packet
    \param bit_index            Internal: progress counter within the current frame
    \param frame_ready          Internal: set to true by the ISR once a full frame (9 packets) has been captured
    \param running              Internal: true while continuous conversion is active
*/
typedef struct {
    uint8_t gpio_convert;
    uint8_t gpio_busy;
    uint8_t gpio_sdo[8];
    uint8_t gpio_cmos_sck;
    uint8_t packet_size;
    uint32_t sampling_period_us;
    uint32_t cmos_sclk_frequency;
    /* --- Internal state, managed entirely by the functions below --- */
    uint8_t  pwm_slice_cnv;
    uint8_t  pwm_slice_sck;
    volatile uint32_t lane_accum[8];
    volatile uint32_t channel_raw[8];
    volatile uint8_t  status_raw;
    volatile uint16_t bit_index;
    volatile bool     frame_ready;
    volatile bool     running;
} ad4858_cmos_t;


/*! \brief Initialize GPIOs for the CMOS interface and read back the currently
 *         configured packet size from the device over the SPI register bus.
 *  \param device   Pointer to the already-initialized AD4858 device settings (ad4858_init() must have run first)
 *  \param config   Pointer to the CMOS interface settings to initialize
 *  \return         True on success, false if gpio_convert and gpio_cmos_sck
 *                  happen to share the same PWM slice (they need
 *                  independent frequencies and therefore must not)
 */
bool ad4858_init_cmos(ad4858_t* device, ad4858_cmos_t* config);


/*! \brief Trigger a single manual conversion by pulsing the CNV pin once.
 *         Intended for simple polling use; for continuous acquisition use
 *         ad4858_start_continuous_conversion() instead.
 *  \param config   Pointer to the CMOS interface settings
 *  \return         True on success
 */
bool ad4858_do_single_conversion(ad4858_cmos_t* config);


/*! \brief Start continuous acquisition: the CNV PWM slice free-runs at
 *         sampling_period_us, and the falling edge of BUSY (via GPIO
 *         interrupt) arms the SCKI PWM slice for each conversion. Every
 *         SCKI wrap interrupt reads all 8 SDO lines and accumulates them
 *         into the internal packet buffers.
 *  \param config   Pointer to the CMOS interface settings
 *  \return         True on success, false if already running or if
 *                  cmos_sclk_frequency / sampling_period_us are zero
 */
bool ad4858_start_continuous_conversion(ad4858_cmos_t* config);


/*! \brief Stop continuous acquisition: disables both PWM slices and the
 *         BUSY GPIO interrupt, and returns gpio_convert / gpio_cmos_sck to
 *         plain GPIO outputs driven low.
 *  \param config   Pointer to the CMOS interface settings
 *  \return         True on success, false if not currently running
 */
bool ad4858_stop_continuous_conversion(ad4858_cmos_t* config);


/*! \brief Check whether a complete frame (8 channel packets + 1 status
 *         packet) has been captured since the last call to
 *         ad4858_cmos_get_frame(). Safe to call from the main loop while
 *         acquisition is running.
 *  \param config   Pointer to the CMOS interface settings
 *  \return         True if a new frame is ready to be read out
 */
bool ad4858_cmos_frame_ready(ad4858_cmos_t* config);


/*! \brief Retrieve the most recently captured frame and clear the
 *         frame_ready flag. Briefly disables the SCKI wrap interrupt while
 *         copying to avoid a race condition with an in-progress capture.
 *  \param config        Pointer to the CMOS interface settings
 *  \param out_channels  Output buffer for the 8 raw channel packets (right-aligned in packet_size bits)
 *  \param out_status    Output pointer for the raw status packet, or NULL if not needed
 */
void ad4858_cmos_get_frame(ad4858_cmos_t* config, uint32_t out_channels[8], uint8_t* out_status);


/*! \brief Extract the signed 20-bit conversion result from a raw channel packet.
 *         Assumption: the conversion result occupies the most significant
 *         20 bits of the packet, MSB first. Verify this against the known
 *         CHx_TESTPAT values (via ad4858_enable_cmos_testpattern()) before
 *         relying on it for real measurements.
 *  \param packet_raw       Raw channel packet as returned by ad4858_cmos_get_frame()
 *  \param packet_size_bits Configured packet size in bits (20/24/32)
 *  \return                 Sign-extended, two's-complement conversion result
 */
int32_t ad4858_cmos_extract_result20(uint32_t packet_raw, uint8_t packet_size_bits);


#endif
