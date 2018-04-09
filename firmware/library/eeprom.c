#include <fx2eeprom.h>
#include <fx2i2c.h>

bool eeprom_read(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte) {
  uint8_t addr_bytes[2];

  if(double_byte) {
    addr_bytes[0] = addr >> 8;
    addr_bytes[1] = addr & 0xff;
  } else {
    addr_bytes[0] = addr;
  }

  if(!i2c_start(chip << 1))
    goto stop;
  if(!i2c_write(addr_bytes, 1 + double_byte))
    goto stop;
  if(!i2c_start((chip << 1) | 1))
    goto stop;
  if(!i2c_read(buf, len))
    goto stop;
  return true;

stop:
  i2c_stop();
  return false;
}

bool eeprom_write(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte) {
  uint8_t xfer_bytes[3];
  uint16_t i;

  for(i = 0; i < len; i++) {
    if(double_byte) {
      xfer_bytes[0] = addr >> 8;
      xfer_bytes[1] = addr & 0xff;
    } else {
      xfer_bytes[0] = addr;
    }
    xfer_bytes[1 + double_byte] = buf[i];

    if(!i2c_start(chip << 1))
      goto stop;
    if(!i2c_write(xfer_bytes, 2 + double_byte))
      goto stop;
    if(!i2c_stop())
      return false;

    addr++;
  }

  return true;

stop:
  i2c_stop();
  return false;
}
