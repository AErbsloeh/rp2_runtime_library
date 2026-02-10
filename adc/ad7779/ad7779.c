#include "sens/ad7779.h"


// ======================================== INTERNAL READ/WRITE FUNCTIONS ===============================================
/*! \brief Function for writing a register to the AD7779
 *  \param handler  Pointer to typedef struct ad7779_t to handle the settings
 *  \return         true if the write was successful, false otherwise.
 */
static bool ad7779_write_reg(ad7779_t *handler,
                             uint8_t reg,
                             uint8_t data)
{
    uint8_t frame[2];
    // R/W=0 (Write) in Bit7, Adresse in Bits[6:0]
    frame[0] = (0 << 7) | (reg & 0x7F);
    frame[1] = data;

    if (send_data_spi_module(handler->spi, handler->gpio->cs_pin, frame, 2) < 0) {
        return false;
    }
    sleep_ms(1);
    
    return true;
}

/*! \brief Function for reading a register from the AD7779
 *  \param handler  Pointer to typedef struct ad7779_t to handle the settings
 *  \return         true if the read was successful, false otherwise.
 */
static bool ad7779_read_reg(ad7779_t *handler,
                            uint8_t reg,
                            uint8_t *data)
{
    uint8_t tx[2], rx[2];
    // R/W=1 (Read) in Bit7, Adress in Bits[6:0]
    tx[0] = (1 << 7) | (reg & 0x7F);
    tx[1] = 0x00;

    if (receive_data_spi_module(handler->spi, handler->gpio->cs_pin, tx, rx, 2) < 0) {
        return false;
    }
    sleep_ms(1);
    *data = rx[1];  // the second byte contains the register data

    return true;
}


// ======================== AD7779 FUNCTIONS =========================
bool ad7779_soft_reset(ad7779_t *handler){
    uint8_t tx_frame[2];
    int8_t result;

    // --- First Write for Soft-Reset (Bits [1:0] = 11b) ---
    tx_frame[0] = (0 << 7) | (AD7779_REG_GENERAL_USER_CONFIG_1 & 0x7F);
    tx_frame[1] = AD7779_SOFT_RST_FIRST_WRITE;
    result = send_data_spi_module(handler->spi, handler->gpio->cs_pin, tx_frame, 2);
    if (result < 0) {
        return false;
    }

    // Short delay to ensure the chip processes the first command
    sleep_ms(1);

    // --- Second Write for Soft-Reset (Bits [1:0] = 10b) ---
    tx_frame[1] = AD7779_SOFT_RST_SECOND_WRITE;
    result = send_data_spi_module(handler->spi, handler->gpio->cs_pin, tx_frame, 2);
    if (result < 0) {
        return false;
    }

    // Now the AD7779 internally waits a maximum of 225 µs until the first valid DRDY (Data Ready).
    // To be safe, we take a buffer of e.g. 1 ms.
    sleep_ms(1);
    return true;
}


bool ad7779_soft_sync(ad7779_t *handler){
    uint8_t tx_read[2];
    uint8_t rx_read[2];
    ad7779_read_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, &rx_read[1]); // Read register and save the value
    ad7779_write_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, rx_read[1] ^ (1 << 0)); // Changing the last Bit of the Register to 0 this will trigger a SYNC event
    sleep_ms(10);
    ad7779_write_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, rx_read[1]); // Change it back to the original value
    return true;
}


bool ad7779_init(ad7779_t *handler){
    // --- GPIO initialization ---
    if(!handler->gpio->init_done){
        ad7779_trigger_reset(handler);
        ad7779_start_pin(handler);
        ad7779_cs_pin(handler);
        handler->gpio->init_done = true;
    }


    // --- SPI initialization ---
    if(!handler->spi->init_done){
        configure_spi_module(handler->spi, false);
    }


    // --- Softreset for AD7779 ---
    if (!ad7779_soft_reset(handler)) {
        handler->init_done = false;
        return false;
    }

    sleep_ms(10);

    uint8_t tx_read[2];
    uint8_t rx_read[2];
    int8_t  result;

    tx_read[0] = (1 << 7) | (AD7779_REG_ERROR_STATUS_REGISTER_3 & 0x7F);
    tx_read[1] = 0x00;  // Dummy-Byte

    result = receive_data_spi_module(handler->spi, handler->gpio->cs_pin, tx_read, rx_read, 2);
    if (result < 0) {
        handler->init_done = false;
        return false;
    }
    // Check for CHIP_ERROR and INIT_COMPLETE
    while (1) {
        ad7779_read_reg(handler, AD7779_REG_GENERAL_ERRORS_REGISTER_2, &rx_read[1]);
        if (rx_read[1] == 0x00) {
            ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_3, &rx_read[1]);
            if (rx_read[1] & AD7779_REG_ERROR_STATUS_REGISTER_3_INIT_COMPLETE_BIT) { // Bit 4 = INIT_COMPLETE
                if(DEBUG_MODE) printf("AD7779 successful init: 0x%02X; Adress: 0x%02X\n", rx_read[1], AD7779_REG_ERROR_STATUS_REGISTER_3);
                handler->init_done = true; 
                break;
            }
        }
        if(DEBUG_MODE) printf("AD7779 ERROR detected! Code: 0x%02X; Adress: 0x%02X\n", rx_read[1], AD7779_REG_GENERAL_ERRORS_REGISTER_2);
        sleep_ms(100);
    }
    // Print all status and error registers
    if (DEBUG_MODE) {
        ad7779_print_all_status_error_register(handler);
    }   
    return true;
}


bool ad7779_read_all_channel_data(ad7779_t *handler,
                                uint8_t *pointer_for_rx){    
    //Option 1: Read all channel data in one SPI transaction
    uint8_t tx[NUM_CHANNELS *4];
    memset(tx, 0x80, 32);
    receive_data_spi_module(handler->spi, handler->gpio->cs_pin, tx, pointer_for_rx, NUM_CHANNELS * 4);
    
    //Option 2: Read all channel data in multiple SPI transactions
    /*
    uint8_t tx_byte = 0x80; // Dummy bytes to clock out the data
    uint8_t rx_byte = 0x00;
    for (int i = 0; i < NUM_CHANNELS * 4; i++) {
        receive_data_spi_module(handler->spi, gpio_handler->cs_pin, &tx_byte, &rx_byte, 1);
        pointer_for_rx[i] = rx_byte;
    }
    */
    return true;
}
// ======================== AD7779 Register configuration =========================
bool ad7779_set_spi_sync(ad7779_t *handler){
    uint8_t current_value =0;
    if(! ad7779_read_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, &current_value)){
            return false;
        }
    current_value |= 0b00000001;

    if (! ad7779_write_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, current_value)) {
        return false;
    }
    return true;
}


bool default_state_mux_register(ad7779_t *handler){
    uint8_t mux_config_val = 0b00000000; // Setup for external reference and normal input muxing
    if (!ad7779_write_reg(handler, AD7779_REG_ADC_MUX_CONFIG,  mux_config_val)) {
        return false;
    }
    return true;
}


bool ad7779_set_sdo_driver_strength(ad7779_t *handler){
    uint8_t current_value =0;
    if(! ad7779_read_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, &current_value)){
            return false;
        }
    current_value &= ~0b00011000; // Clear SDO Driver Strength bits
    switch (handler->sdo_driver_strength) {
        case 0: // Normal
            break;
        case 1: // Strong
            current_value |= 0b00001000;  // Set to Strong
            break;
        case 2: // Weak
            current_value |= 0b00010000;  // Set to Weak
            break;
        case 3: // Extra Strong
            current_value |= 0b00011000;  // Set to Extra Strong
            break;
        default:
            return false; // Invalid strength value
    }
    
    if (! ad7779_write_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_2, current_value)) {
        return false;
    }
    return true;
}


bool ad7779_set_power_mode(ad7779_t *handler){
    uint8_t current_value =0;
    if(! ad7779_read_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_1, &current_value)){
            return false;
        }

    if (handler->high_power_mode) {
        current_value |= 0b01000000; // Set POWERMODE=1
    } else {
        current_value &= ~0b01000000; // Clear POWERMODE bit to 0
    }
    if (! ad7779_write_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_1, current_value)) {
        return false;
    }
    sleep_ms(1);
    ad7779_start_pin(handler);
    
    return true;
}


bool ad7779_set_sampling_rate(ad7779_t *handler)
{
    uint8_t current_value =0;
    uint32_t f_mod = 0; // Modulator Frequency
    if (handler->high_power_mode) {
        f_mod = 8192000/4; // High Power Mode
    } else {
        f_mod = 4096000/8; // Low Power Mode
    }

    uint32_t dec_int_N = f_mod / handler->sampling_rate_SPS; // Total Decimation Integer N
    uint64_t remainder = f_mod % handler->sampling_rate_SPS; // Remainder for fractional part

    uint32_t dec_if = (uint32_t)((remainder * 65536ULL) / handler->sampling_rate_SPS);

    if (dec_int_N > 0xFFF) { // N is a 12-bit value
        return false;
    }
    
    if (!ad7779_read_reg(handler, AD7779_REG_LOAD_UPDATE_REGISTER, &current_value)) {
            return false;
        }
    current_value &= 0xF0;
    current_value |= (uint8_t)((dec_int_N >> 8) & 0x0F); // Set the upper 4 bits of N

    if (! ad7779_write_reg(handler, AD7779_REG_DECIMATION_RATE_REGISTER_N_MSB, current_value)) {
        return false;
    }
    if (! ad7779_write_reg(handler, AD7779_REG_DECIMATION_RATE_REGISTER_N_LSB, (uint8_t)(dec_int_N & 0xFF))) {
        return false;
    }
    if (! ad7779_write_reg(handler, AD7779_REG_DECIMATION_RATE_REGISTER_IF_MSB, (uint8_t)((dec_if >> 8) & 0xFF))) {
        return false;
    }
    if (! ad7779_write_reg(handler, AD7779_REG_DECIMATION_RATE_REGISTER_IF_LSB, (uint8_t)(dec_if & 0xFF))) {
        return false;
    }
    
    // Load the new settings

    current_value =0;
    if(! ad7779_read_reg(handler, AD7779_REG_LOAD_UPDATE_REGISTER, &current_value)){
            return false;
        }
    current_value |= 0x01; // Set LOAD bit

    if (! ad7779_write_reg(handler, AD7779_REG_LOAD_UPDATE_REGISTER, current_value)) { // Load new settings
        return false;
    }
    sleep_ms(1);

    current_value &= ~0x01; // Clear LOAD bit
    if (! ad7779_write_reg(handler, AD7779_REG_LOAD_UPDATE_REGISTER, current_value)) {
        return false;
    }
    return true;
}


bool ad7779_enable_disable_channels(ad7779_t *handler){ //needs to be tested
    uint8_t inverted_channel_mask = ~(handler->channel_select); // Invert the channel select mask because 1 = disable, 0 = enable
    if (!ad7779_write_reg(handler, AD7779_REG_CH_DISABLE, inverted_channel_mask)) { //Disable the selected channels
        return false;
    }   
    ad7779_start_pin(handler);
    return true;
}


bool ad7779_configure_pga_for_all_channels(ad7779_t *handler) //needs to be tested
{
    uint8_t channel_config_reg[] ={0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007};
    uint8_t reg_value = 0;
    uint8_t current_value =0;
    
    switch (handler ->pga_gain)
    {
    case 1:
        reg_value = 0b00000000;
        break;
    case 2:
        reg_value = 0b01000000;
        break;
    case 4:
        reg_value = 0b10000000;
        break;
    case 8:
        reg_value = 0b11000000;
        break;
    default:
        return false;
    }

    for (int i = 0; i < 8; i++) {
        if(! ad7779_read_reg(handler, channel_config_reg[i], &current_value)){
            return false;
        }
        current_value &= 0x3F; // Clear PGA bits
        current_value |= reg_value; // Set new PGA bits
        if (!ad7779_write_reg(handler, channel_config_reg[i], current_value)) {
            return false;
        }
    }
    ad7779_start_pin(handler);
    return true;
}


bool ad7779_switch_daq_header(ad7779_t *handler){
    uint8_t current_value =0;
    if(! ad7779_read_reg(handler, AD7779_REG_GEN_ERR_REG_1_EN, &current_value)){
            return false;
        }
    if (handler->error_header) {
        current_value |= 0b00000001; // Set to Error Header
    } else {
        current_value &= ~0b00000001; // Clear to CRC Header
    }
    if (! ad7779_write_reg(handler, AD7779_REG_GEN_ERR_REG_1_EN, current_value)) {
        return false;
    }
    return true;
}


bool ad7779_enable_disable_test_mode(ad7779_t *handler){   //Needs to be tested
    uint8_t reg_value;
    uint8_t channel_config_reg[] ={0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007};
   if (!ad7779_read_reg(handler, AD7779_REG_ADC_MUX_CONFIG, &reg_value)) {
        return false;
    }

    reg_value &= ~0b00111100; // Clear TEST_MODE bit
    if (handler ->test_mode) {
        reg_value |= 0b00001000; // Set TEST_MODE bit
    } else {
        reg_value &= ~0b00001000; // Clear TEST_MODE bit
    }
    if (!ad7779_write_reg(handler, AD7779_REG_ADC_MUX_CONFIG, reg_value)) {
        return false;
    }

    for (int i = 0; i < 8; i++) {
        if (!ad7779_read_reg(handler, channel_config_reg[i], &reg_value)) {
            return false;
        }
        if (handler ->test_mode) {
            reg_value |= 0b00010000; // Set TEST_MODE bit in channel config
        } else {
            reg_value &= ~0b00010000; // Clear TEST_MODE bit in channel config
        }
        if (!ad7779_write_reg(handler, channel_config_reg[i], reg_value)) {
            return false;
        }
    }
    ad7779_start_pin(handler);
    return true;
}


bool ad7779_data_read_mode(ad7779_t *handler){
    uint8_t current_value =0;
    bool new_state;
    if (! ad7779_read_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_3, &current_value)){
            return false;
        }
    if (! handler-> data_read_mode) {
        current_value |= 0b00010000; // Enable Read SPI Mode
        new_state = true;
    } else {
        current_value &= ~0b00010000; // Disable Read SPI Mode
        new_state = false;
    }
    if (! ad7779_write_reg(handler, AD7779_REG_GENERAL_USER_CONFIG_3, current_value)) {
        return false;
    }
    handler->data_read_mode = new_state;
    return true;
}


// IMPORTANT: This function changes the SPI read mode to register read mode!
void ad7779_print_all_status_error_register(ad7779_t *handler){
    uint8_t tx_read[2];
    uint8_t rx_read[2];
    if (handler->data_read_mode){
        ad7779_data_read_mode(handler); // Disable Data Read Mode to be able to read registers
    }
    ad7779_data_read_mode(handler); // Disable Data Read Mode to be able to read registers
    printf("- - - - AD7779 Status and Error Registers - - - -\n");

    ad7779_read_reg(handler, AD7779_REG_CHANNEL0_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 0 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL0_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL1_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 1 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL1_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL2_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 2 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL2_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL3_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 3 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL3_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL4_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 4 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL4_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL5_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 5 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL5_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL6_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 6 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL6_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL7_STATUS_REGISTER, &rx_read[1]);
    printf("Status Register Channel 7 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL7_STATUS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL0_CHANNEL1_DSP_ERRORS_REGISTER, &rx_read[1]);
    printf("Saturation Error Register Channel 0 and 1 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL0_CHANNEL1_DSP_ERRORS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL2_CHANNEL3_DSP_ERRORS_REGISTER, &rx_read[1]);
    printf("Saturation Error Register Channel 2 and 3 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL2_CHANNEL3_DSP_ERRORS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL4_CHANNEL5_DSP_ERRORS_REGISTER, &rx_read[1]);
    printf("Saturation Error Register Channel 4 and 5 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL4_CHANNEL5_DSP_ERRORS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_CHANNEL6_CHANNEL7_DSP_ERRORS_REGISTER, &rx_read[1]);
    printf("Saturation Error Register Channel 6 and 7 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_CHANNEL6_CHANNEL7_DSP_ERRORS_REGISTER, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_GENERAL_ERRORS_REGISTER_1, &rx_read[1]);
    printf("General Error Register 1 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_GENERAL_ERRORS_REGISTER_1, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_GENERAL_ERRORS_REGISTER_2, &rx_read[1]);
    printf("General Error Register 2 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_GENERAL_ERRORS_REGISTER_2, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_1, &rx_read[1]);
    printf("Error Status Register 1 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_ERROR_STATUS_REGISTER_1, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_2, &rx_read[1]);
    printf("Error Status Register 2 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_ERROR_STATUS_REGISTER_2, rx_read[1]);

    ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_3, &rx_read[1]);
    printf("Error Status Register 3 (Address 0x%02X) (normal value 0x00): 0x%02X\n", AD7779_REG_ERROR_STATUS_REGISTER_3, rx_read[1]);

    printf("- - - - AD7779 Status and Error Registers End- - - -\n");
}

void ad7779_output_all_status_error_register(ad7779_t *handler, uint8_t *buffer){
    uint8_t tx_read[2];
    uint8_t rx_read[2];
    if (handler->data_read_mode){
        ad7779_data_read_mode(handler); // Disable Data Read Mode to be able to read registers
    }

    ad7779_read_reg(handler, AD7779_REG_CHANNEL0_STATUS_REGISTER, &rx_read[1]);
    buffer[1] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL1_STATUS_REGISTER, &rx_read[1]);
    buffer[2] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL2_STATUS_REGISTER, &rx_read[1]);
    buffer[3] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL3_STATUS_REGISTER, &rx_read[1]);
    buffer[4] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL4_STATUS_REGISTER, &rx_read[1]);
    buffer[5] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL5_STATUS_REGISTER, &rx_read[1]);
    buffer[6] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL6_STATUS_REGISTER, &rx_read[1]);
    buffer[7] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL7_STATUS_REGISTER, &rx_read[1]);
    buffer[8] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL0_CHANNEL1_DSP_ERRORS_REGISTER, &rx_read[1]);
    buffer[9] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL2_CHANNEL3_DSP_ERRORS_REGISTER, &rx_read[1]);
    buffer[10] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL4_CHANNEL5_DSP_ERRORS_REGISTER, &rx_read[1]);
    buffer[11] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_CHANNEL6_CHANNEL7_DSP_ERRORS_REGISTER, &rx_read[1]);
    buffer[12] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_GENERAL_ERRORS_REGISTER_1, &rx_read[1]);
    buffer[13] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_GENERAL_ERRORS_REGISTER_2, &rx_read[1]);
    buffer[14] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_1, &rx_read[1]);
    buffer[15] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_2, &rx_read[1]);
    buffer[16] = rx_read[1];

    ad7779_read_reg(handler, AD7779_REG_ERROR_STATUS_REGISTER_3, &rx_read[1]);
    buffer[17] = rx_read[1];
}

// ======================== AD7779 GPIO Functions =========================
bool ad7779_trigger_reset(ad7779_t *handler){
    if(!handler->gpio->init_done){
        gpio_init(handler->gpio->reset_pin);
        gpio_set_dir(handler->gpio->reset_pin, GPIO_OUT);
    }
    gpio_put(handler->gpio->reset_pin, 1); 
    sleep_ms(10);
    gpio_put(handler->gpio->reset_pin, 0); 
    sleep_ms(10);
    gpio_put(handler->gpio->reset_pin, 1);
    sleep_ms(10);
    return true;
}


bool ad7779_start_pin(ad7779_t *handler){
    if (!handler->gpio->init_done){
        gpio_init(handler->gpio->start_pin);
        gpio_set_dir(handler->gpio->start_pin, GPIO_OUT);
    }
    gpio_put(handler->gpio->start_pin, 1); //set start_pin to high, this is the required state, if this pin is not used
    sleep_ms(10);
    gpio_put(handler->gpio->start_pin, 0);
    sleep_ms(10);
    gpio_put(handler->gpio->start_pin, 1);
    sleep_ms(10);
    return true;
}

bool ad7779_cs_pin(ad7779_t *handler){
    if (!handler->gpio->init_done){
        gpio_init(handler->gpio->cs_pin);
        gpio_set_dir(handler->gpio->cs_pin, GPIO_OUT);
        gpio_put(handler->gpio->cs_pin, 1);
    }
    return true;
}