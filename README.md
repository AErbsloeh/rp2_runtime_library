# rp2_runtime_library
This repo contains source files for different sensor / devices which can be used in Pico/RP2xxx projects.
For example you can use the corresponding project template using the [Link](https://github.com/AErbsloeh/pico_runtime_example).

Enjoy. If you have questions and suggestions, please create an issue or contact the authors. Further examples can be found in this [repo](https://github.com/analogdevicesinc/no-OS/tree/main).


## Hardware Abstraction Layer (hal)
- [x] FIFO buffer (`hal/fifo`, queue)
- [x] FIFO sampling buffer (with DMA, `use_dma` in `fifo_t`)
- [x] DAQ interface with example (data type per stream, thread-safe for core 1)
- [x] Processing of DAQ data on core 1
- [x] RP2-ADC interface
- [x] SPI interface
- [x] I2C interface
- [x] UART interface
- [x] USB interface
- [x] Clock generating with PIO module
- [x] Power handling for external devices (with/without feedback)
- [x] Timer interrupt
- [x] Setting of the system clock
- [x] CRC helper functions
- [x] Transport layer for USB, WiFi (TCP) and Bluetooth Classic SPP (`hal_transport_usb`, `hal_transport_wifi`, `hal_transport_bt`)
- [x] LED control
- [x] Pulse Width Modulation
- [x] Timer
- [x] Bluetooth Classic SPP for Pico (2) W and Raspberry Pi Radio Module 2
- [x] Wifi with different data packets like UDP
- [x] LED control of WS2812b (using PIO)
- [x] Clock Generator with GPIO output (using PIO)


## Device Libraries
- [x] ADC ADS8881
- [x] ADC AD7779
- [x] ADC AD4858 (basic and CMOS interface)
- [x] DAC AD57x4
- [x] DAC AD5765
- [x] DAC LTC2668
- [x] Multiplexer: ADG1408
- [x] Multiplexer: ADGS1208
- [x] Digital Potentiometer: AD528x
- [x] Digital Potentiometer: AD526x
- [x] Digital Potentiometer: AD5141 (I2C, SPI)
- [x] Digital Potentiometer: AD5142A


## Peripherie Devices
- [x] SD card support
- [ ] Ethernet module W5500 (WIP)
- [x] FPGA Handler + Flashing (SPI 1x, no bus alignment) for AMD, Lattice and CologneChip FPGAs (WIP)
- [ ] Adafruit OLED Display 4400 (WIP)


## Sensor Libraries
- [x] Environment Sensor: SHT21
- [x] Accelerator: ADXL345
- [x] Accelerator and Gyroscope: BMI270
- [x] Light: VEML7700
- [x] Distance Estimation: VL6180x
- [x] Current Measuring Unit: PAC193X
- [x] Microphone Amplifier: MAX9814
