#include <AP_HAL.h>
#if CONFIG_HAL_BOARD == HAL_BOARD_MPNG

#include <avr/io.h>
#include <AP_HAL.h>
#include "SPIDriver.h"
#include "SPIDevices.h"
#include "GPIO.h"
#include "utility/pins_arduino_mega.h"
using namespace MPNG;

extern const AP_HAL::HAL& hal;

void MPNGSPIDeviceManager::init(void* machtnichts) {
    
    /* dataflow cs is on arduino pin 53, PORTB0 */
    AVRDigitalSource* df_cs = new AVRDigitalSource(_BV(0), PB);
    df_cs->mode(GPIO_OUTPUT);
    df_cs->write(1);
    
    /* dataflash: divide clock by 2 to 8Mhz, set SPI_MODE_3
     * spcr gets 0x0C to set SPI_MODE_3
     * spsr gets bit SPI2X for clock divider
    _dataflash = new AVRSPI0DeviceDriver(df_cs, 0x03, 0x00, _BV(SPI2X));
     */
    AVRDigitalSource* cs = new AVRDigitalSource(_BV(4), PC);
    _dataflash = new AVRSPI0DeviceDriver(cs, 0x03, 0x00, _BV(SPI2X));
    _dataflash->init();

}

AP_HAL::SPIDeviceDriver* MPNGSPIDeviceManager::device(enum AP_HAL::SPIDevice d) 
{
    switch (d) {
        case AP_HAL::SPIDevice_Dataflash:
            return _dataflash;
        default:
            return NULL;
    };
}

#endif
