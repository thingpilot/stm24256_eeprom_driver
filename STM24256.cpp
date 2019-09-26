#include "STM24256.h"


STM24256::STM24256(PinName write_control, PinName sda, PinName scl, int frequency) :
                   _write_control(write_control, EEPROM_WRITE_DISABLE), 
                   _i2c(sda, scl), 
                   _i2c_frequency(frequency)
{
    disable_write();
    _i2c.frequency(_i2c_frequency);
}


STM24256::~STM24256()
{
    disable_write();
}


void STM24256::enable_write()
{
    _write_control = EEPROM_WRITE_ENABLE;
}


void STM24256::disable_write()
{
   _write_control = EEPROM_WRITE_DISABLE; 
}


EEPROM_Status_t STM24256::set_operation_address(uint16_t address, bool stop)
{
    uint8_t address_msb = address >> 8;
    uint8_t address_lsb = address & 0xFF;

    _i2c.start();

    if(_i2c.write(EEPROM_MEM_ARRAY_ADDRESS_WRITE) != mbed::I2C::ACK) 
    {
        return EEPROM_SET_OP_ADDRESS_FAIL_MEM_ARRAY;
    }

    if(_i2c.write(address_msb) != mbed::I2C::ACK) 
    {
        return EEPROM_SET_OP_ADDRESS_FAIL_MSB;
    }

    if(_i2c.write(address_lsb) != mbed::I2C::ACK) 
    {
        return EEPROM_SET_OP_ADDRESS_FAIL_LSB;
    }

    if(send_stop) 
    {
        _i2c.stop()
    }

    return EEPROM_SET_OP_ADDRESS_OK;
}


EEPROM_Status_t STM24256::read_from_address(uint16_t address, char *data, int data_length)
{
    _i2c.lock();

    EEPROM_Status_t status = set_operation_address(address, true);
    if(status != EEPROM_OK)
    {
        _i2c.unlock();
        return status;
    }

    if(_i2c.read(EEPROM_MEM_ARRAY_ADDRESS_READ, data, data_length) != mbed::I2C::ACK) 
    {
        _i2c.unlock();
        return EEPROM_READ_FAIL;
    }

    _i2c.unlock();

    return EEPROM_OK;
}


EEPROM_Status_t STM24256::write_to_address(uint16_t address, char *data, int data_length, bool verify = true)
{
    _i2c.lock();

    enable_write();

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
    disable_write();
    _i2c.unlock();

    if(verify) {
        wait_ms(5);

        char data_verify[data_length];

        if(read_from_address(address, data_verify, data_length) != EEPROM_OK) 
        {
            return EEPOM_READ_FAIL;
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