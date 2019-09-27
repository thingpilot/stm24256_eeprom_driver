## STM24256 Driver Release Notes
**v1.1.0** *27/09/2019*

 - Created typedef for EEPROM_Status; `EEPROM_Status_t`
 - Replaced deprecated call to `wait_ms` with `wait_us`
 - Improved comments and fixed various typos

**v1.0.0** *27/09/2019*

 - Initial release of driver
 - Functionality to control write_control line
 - Set operation address
 - Single page read/write (with write verification)
