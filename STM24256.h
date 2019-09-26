#pragma once

#include <mbed.h>


#define EEPROM_MEM_ARRAY_ADDRESS_READ 0b10100001
#define EEPROM_MEM_ARRAY_ADDRESS_WRITE 0b10100000


class STM24256 {

    public:

        enum {
            EEPROM_OK = 0,
            EEPROM_SET_OP_ADDRESS_FAIL_MEM_ARRAY = 1,
            EEPROM_SET_OP_ADDRESS_FAIL_MSB       = 2,
            EEPROM_SET_OP_ADDRESS_FAIL_LSB       = 3,
            EEPROM_READ_FAIL                     = 4,
            EEPROM_WRITE_FAIL                    = 5,
            EEPROM_VERIFY_FAIL                   = 6
        } EEPROM_Status_t;

        STM24256(PinName write_control, PinName sda, PinName, scl, int frequency);

        ~STM24256();

        EEPROM_Status_t read_from_address(uint16_t address, char *data, int data_length);

        EEPROM_Status_t write_to_address(uint16_t address, char *data, int data_length, bool verify = true);

    private:

        enum {
            EEPROM_WRITE_ENABLE  = 0,
            EEPROM_WRITE_DISABLE = 1
        };

        void enable_write();

        void disable_write();

        EEPROM_Status_t set_operation_address(uint16_t address, bool stop);

        DigitalOut _write_control;

        I2C _i2c;

        int _i2c_frequency;     
}