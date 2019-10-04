/**
  * @file    STM24256.cpp
  * @version 1.1.1
  * @author  Adam Mitchell
  * @brief   C++ file of the STM24256 EEPROM driver module
  */

/** Includes
 */
#include "STM24256.h"

/** Constructor. Create an EEPROM interface, connected to the pins specified 
 *  operating at the specified frequency
 * 
 * @param write_control GPIO to enable or disable write functionality
 * @param sda I2C data line pin
 * @param scl I2C clock line pin
 * @param frequency_hz The bus frequency in hertz
 */
STM24256::STM24256(PinName write_control, PinName sda, PinName scl, int frequency_hz) :
                   _write_control(write_control, EEPROM_WRITE_DISABLE), 
                   _i2c(sda, scl), 
                   _i2c_frequency_hz(frequency_hz)
{
    disable_write();
    _i2c.frequency(_i2c_frequency_hz);
}

/** Destructor. Will disable write_control
 */
STM24256::~STM24256()
{
    disable_write();
}

/** Set EEPROM write_control line to logic low; this allows the EEPROM to enter write mode
 */
void STM24256::enable_write()
{
    _write_control = EEPROM_WRITE_ENABLE;
}

/** Set EEPROM write_control line to logic high; this prevents the EEPROM from entering write mode
 */
void STM24256::disable_write()
{
   _write_control = EEPROM_WRITE_DISABLE; 
}

/** At the beginning of a read or write operation an address (to either read from, or write to)
 *  must be specified 
 * 
 * @param address 2 byte address pointing to where the operation will begin
 * @param send_stop Optionally generate an I2C stop condition on the bus
 * @return Indicates success or failure reason
 */
STM24256::EEPROM_Status_t STM24256::set_operation_address(uint16_t address, bool send_stop)
{
    uint8_t address_msb = address >> 8;
    uint8_t address_lsb = address & 0xFF;

    _i2c.lock();

    _i2c.start();

    if(_i2c.write(EEPROM_MEM_ARRAY_ADDRESS_WRITE) != mbed::I2C::ACK) 
    {
        _i2c.stop();
        _i2c.unlock();
        return EEPROM_SET_OP_ADDRESS_FAIL_MEM_ARRAY;
    }

    if(_i2c.write(address_msb) != mbed::I2C::ACK) 
    {
        _i2c.stop();
        _i2c.unlock();
        return EEPROM_SET_OP_ADDRESS_FAIL_MSB;
    }

    if(_i2c.write(address_lsb) != mbed::I2C::ACK) 
    {
        _i2c.stop();
        _i2c.unlock();
        return EEPROM_SET_OP_ADDRESS_FAIL_LSB;
    }

    if(send_stop) 
    {
        _i2c.stop();
    }

    _i2c.unlock();

    return EEPROM_OK;
}

/** 
 *  
 */ 
STM24256::Array_16x2 STM24256::get_array_slice_locs(uint16_t start_address, int data_length, int &boundaries) 
{
    static int chunks[16][2] = {0, 0};

    int current_page = -1;
    int end_page = ((start_address + data_length) - 1) / 64; 

    uint16_t chunk_start_address = start_address;
    int chunk_index = 0;

    for(uint16_t address = start_address; address < start_address + data_length; address++) 
    {
        int byte_page = address / 64;

        if(current_page == -1) 
        {
            current_page = byte_page;
        }
        else if(current_page != byte_page) 
        {
            chunks[chunk_index][LENGTH_DIM] = address - chunk_start_address;
            chunks[chunk_index][ADDRESS_DIM] = chunk_start_address;

            current_page = byte_page;
            chunk_start_address = address;
            chunk_index++;
            boundaries++;
        }
        
        if(current_page == end_page)
        {
            chunks[chunk_index][LENGTH_DIM] = (start_address + data_length) - chunk_start_address; 
            chunks[chunk_index][ADDRESS_DIM] = chunk_start_address;
            break;
        }
    }

    return chunks;
}

/** Read data_length bytes from address into data
 * 
 * @param address 2 byte address that points to start of data
 * @param data Char array in which to store retrieved data
 * @param data_length Amount of data to retrieve in bytes
 * @return Indicates success or failure reason
 */
STM24256::EEPROM_Status_t STM24256::read_from_address(uint16_t address, char *data, int data_length)
{
    /** Do not attempt to read zero bytes
     */
    if(data_length <= 0)
    {
        return EEPROM_DATA_LENGTH_ZERO;
    }

    _i2c.lock();

    /** Determine whether or not we need to do a multi-page read or not
     */
    int boundaries = 0;
    STM24256::Array_16x2 slice_locs = get_array_slice_locs(address, data_length, boundaries);

    /** Single page read
     */
    if(boundaries == 0) 
    {
        EEPROM_Status_t status = set_operation_address(address, true);
        if(status != EEPROM_OK)
        {
            _i2c.unlock();
            return status;
        }

        if(_i2c.read(EEPROM_MEM_ARRAY_ADDRESS_READ, data, data_length) != mbed::I2C::NoACK) 
        {
            _i2c.unlock();
            return EEPROM_READ_FAIL;
        }
    }
    /** Multi-page read
     */
    else 
    {
        /** Each boundary represents a new page within the EEPROM we need to write to
         *  and each individual page requires the operation address to be reset to
         *  the new address
         */
        for(int boundary = 0; boundary <= boundaries; boundary++)
        {
            uint16_t address = slice_locs[boundary][ADDRESS_DIM];

            EEPROM_Status_t status = set_operation_address(address, true);
            if(status != EEPROM_OK)
            {
                _i2c.unlock();
                return status;
            }

            /** Determine array indices we need to write
             */
            int start_idx, end_idx;

            if(boundary == 0) 
            {
                start_idx = 0;
                end_idx   = slice_locs[boundary][LENGTH_DIM] - 1;
            }
            else
            {
                start_idx = slice_locs[boundary - 1][LENGTH_DIM]; 
                end_idx   = (slice_locs[boundary - 1][LENGTH_DIM] + slice_locs[boundary][LENGTH_DIM]) - 1;
            }

            /** Read data chunk
             */
            if(_i2c.read(EEPROM_MEM_ARRAY_ADDRESS_READ, &data[start_idx], slice_locs[boundary][LENGTH_DIM]) != mbed::I2C::NoACK) 
            {
                _i2c.unlock();
                return EEPROM_READ_FAIL;
            }

            /** There must be a minimum of 5 ms delay between EEPROM operations due to the time taken by
             *  the internal processes of the chip. Without this delay, the subsequent write operations 
             *  will fail sporadically
             */
            if(boundary != boundaries) 
            {
                wait_us(5000);
            }
        }
    }
    
    _i2c.unlock();

    return EEPROM_OK;
}

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
STM24256::EEPROM_Status_t STM24256::write_to_address(uint16_t address, char *data, int data_length, bool verify)
{
    /** Do not attempt to write zero bytes
     */
    if(data_length <= 0)
    {
        return EEPROM_DATA_LENGTH_ZERO;
    }

    /** The EEPROM will pad single-byte values, this will result in potentially reading
     *  back 'incorrect' values due to padding. To avoid this we can force the user to pad
     *  the data, i.e. using a uint16_t instead of uint8_t and avoid this issue
     */
    if(data_length % 2 != 0) 
    {
        return EEPROM_DATA_LENGTH_ODD;
    }

    _i2c.lock();

    enable_write();

    /** Determine whether or not we need to do a multi-page write or not
     */
    int boundaries = 0;
    STM24256::Array_16x2 slice_locs = get_array_slice_locs(address, data_length, boundaries);

    /** Single page write
     */
    if(boundaries == 0) 
    {
        EEPROM_Status_t status = set_operation_address(address, false);
        if(status != EEPROM_OK)
        {
            _i2c.unlock();
            return status;
        }

        for(int i = 0; i < data_length; i++) 
        {
            if(_i2c.write(data[i]) != mbed::I2C::ACK)
            {
                disable_write();
                _i2c.unlock();
                return EEPROM_WRITE_FAIL;
            }
        }

        _i2c.stop();
    }
    /** Multi-page write
     */
    else 
    {
        /** Each boundary represents a new page within the EEPROM we need to write to
         *  and each individual page requires the operation address to be reset to
         *  the new address
         */
        for(int boundary = 0; boundary <= boundaries; boundary++)
        {       
            uint16_t address = slice_locs[boundary][ADDRESS_DIM];

            EEPROM_Status_t status = set_operation_address(address, false);
            if(status != EEPROM_OK)
            {
                disable_write();
                _i2c.unlock();
                return status;
            }

            /** Determine array indices we need to write
             */
            int start_idx, end_idx;

            if(boundary == 0) 
            {
                start_idx = 0;
                end_idx   = slice_locs[boundary][LENGTH_DIM] - 1;
            }
            else
            {
                start_idx = slice_locs[boundary - 1][LENGTH_DIM]; 
                end_idx   = (slice_locs[boundary - 1][LENGTH_DIM] + slice_locs[boundary][LENGTH_DIM]) - 1;
            }

            /** Copy data 'slice' into temp write_data buffer
             */ 
            char write_data[64];
            memcpy(write_data, &data[start_idx], slice_locs[boundary][LENGTH_DIM]);

            /** Write each byte
             */
            for(int byte_slice = 0; byte_slice < slice_locs[boundary][LENGTH_DIM]; byte_slice++)
            {
                if(_i2c.write(write_data[byte_slice]) != mbed::I2C::ACK)
                {
                    disable_write();
                    _i2c.unlock();
                    return EEPROM_WRITE_FAIL;
                }
            }

            /** We must create an I2C stop condition on the bus between page write operations
             */
            _i2c.stop();

            /** There must be a minimum of 5 ms delay between EEPROM operations due to the time taken by
             *  the internal processes of the chip. Without this delay, the subsequent write operations 
             *  will fail sporadically
             */
            if(boundary != boundaries) 
            {
                wait_us(5000);
            }
        }
    }

    disable_write();
    _i2c.unlock();

    if(verify) 
    {
        /** There must be a minimum of 5 ms delay between EEPROM operations due to the time taken by
         *  the internal processes of the chip. Without this delay, the subsequent write operations 
         *  will fail sporadically
         */
        wait_us(5000);

        char data_verify[data_length];

        if(read_from_address(address, data_verify, data_length) != EEPROM_OK) 
        {
            return EEPROM_READ_FAIL;
        }

        for(int i = 0; i < data_length; i++)
        {
            if(data[i] != data_verify[i])
            {
                return EEPROM_VERIFY_FAIL;
            }
        }
    }

    return EEPROM_OK;
}