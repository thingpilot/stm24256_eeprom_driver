## STM24256 Driver Release Notes
**v1.2.1** *07/10/2019*

 - Limit maximum data read/write size to 1024 bytes

**v1.2.0** *04/10/2019*

 - Add multi-page read/write functionality

**v1.1.1** *27/09/2019*

 - Unify usage of brackets throughout library

**v1.1.0** *27/09/2019*

 - Created typedef for EEPROM_Status; `EEPROM_Status_t`
 - Replaced deprecated call to `wait_ms` with `wait_us`
 - Improved comments and fixed various typos

**v1.0.0** *27/09/2019*

 - Initial release of driver
 - Functionality to control write_control line
 - Set operation address
 - Single page read/write (with write verification)
