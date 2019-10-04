/**
  * @file    STM24256.h
  * @version 1.2.0
  * @author  Adam Mitchell
  * @brief   Header file of the STM24256 EEPROM driver module
  */

/** Define to prevent recursive inclusion
 */
#pragma once

/** Includes 
 */
#include <mbed.h>

/** 8-bit I2C address for the EEPROM memory array. This should be set according to the 
 *  configuration of the hardware address pins
 */
#define EEPROM_MEM_ARRAY_ADDRESS_READ 0b10100001
#define EEPROM_MEM_ARRAY_ADDRESS_WRITE 0b10100000

/** Base class for the STM24256 series EEPROM 
 */ 
class STM24256 
{

    public:

        typedef int EEPROM_Status_t;

        enum 
        {
            EEPROM_OK                            = 0,
            EEPROM_SET_OP_ADDRESS_FAIL_MEM_ARRAY = 1,
            EEPROM_SET_OP_ADDRESS_FAIL_MSB       = 2,
            EEPROM_SET_OP_ADDRESS_FAIL_LSB       = 3,
            EEPROM_READ_FAIL                     = 4,
            EEPROM_WRITE_FAIL                    = 5,
            EEPROM_VERIFY_FAIL                   = 6,
            EEPROM_DATA_LENGTH_ODD               = 7,
            EEPROM_DATA_LENGTH_ZERO              = 8
        };

        /** Constructor. Create an EEPROM interface, connected to the pins specified 
         *  operating at the specified frequency
         * 
         * @param write_control GPIO to enable or disable write functionality
         * @param sda I2C data line pin
         * @param scl I2C clock line pin
         * @param frequency_hz The bus frequency in hertz
         */
        STM24256(PinName write_control, PinName sda, PinName scl, int frequency_hz);

        /** Destructor. Will disable write_control
         */
        ~STM24256();

        /** Read data_length bytes from address into data
         * 
         * @param address 2 byte address that points to start of data
         * @param data Char array in which to store retrieved data
         * @param data_length Amount of data to retrieve in bytes
         * @return Indicates success or failure reason
         */
        EEPROM_Status_t read_from_address(uint16_t address, char *data, int data_length);

        /** Write data_length bytes from char to address, option to verify this write
         *  by reading and checking the data byte by byte
         * 
         * @param address 2 byte address pointing to where the write operation will begin
         * @param data Char array storing data to be written
         * @param data_length Amount of data to write in bytes
         * @param verify Decide whether or not you want to verify the data that has been written
         *               to the EEPROM. Defaults to true
         * @return Indicates success or failure reason
         */
        EEPROM_Status_t write_to_address(uint16_t address, char *data, int data_length, bool verify = true);

    private:

        enum 
        {
            EEPROM_WRITE_ENABLE  = 0,
            EEPROM_WRITE_DISABLE = 1
        };

        enum
        {
            LENGTH_DIM  = 0,
            ADDRESS_DIM = 1
        };

        /** Set EEPROM write_control line to logic low; this allows the EEPROM to enter write mode
         */
        void enable_write();

        /** Set EEPROM write_control line to logic high; this prevents the EEPROM from entering write mode
         */
        void disable_write();

        /** At the beginning of a read or write operation an address (to either read from, or write to)
         *  must be specified 
         * 
         * @param address 2 byte address pointing to where the operation will begin
         * @param send_stop Optionally generate an I2C stop condition on the bus
         * @return Indicates success or failure reason
         */ 
        EEPROM_Status_t set_operation_address(uint16_t address, bool stop);

        /** Data type to handle 2-D array of size 16 x 2
         */  
        typedef int (&Array_16x2)[16][2];

        /** Given a 64 byte page size within the EEPROM, determine where data of length data_length
         *  and starting at start_address will cross page boundaries
         * 
         *  @param start_address 2 byte address pointing to the intended start location of the operation
         *                       in memory
         *  @param data_length Amount of data to be written in bytes
         *  @param &boundaries Reference to an integer object to store amount of page boundaries detected
         *  @return Returns a 2-D array of size 16 x 2 containing amount of bytes to be written in the 
         *                  0th dimension and address to be written to in the 1st dimension
         */ 
        Array_16x2 get_array_slice_locs(uint16_t start_address, int data_length, int &boundaries);

        DigitalOut _write_control;

        I2C _i2c;

        int _i2c_frequency_hz;     
};