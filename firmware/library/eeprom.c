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
    return false;
  return true;

stop:
  i2c_stop();
  return false;
}

bool eeprom_write(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte,
                  uint8_t page_size, uint8_t timeout) {
  uint16_t written = 0;

  if(!i2c_start(chip << 1))
    goto stop;

  while(written < len) {
    uint8_t addr_bytes[2];
    uint16_t chunk_len;
    uint8_t attempt;

    if(double_byte) {
      addr_bytes[0] = addr >> 8;
      addr_bytes[1] = addr & 0xff;
    } else {
      addr_bytes[0] = addr;
    }
    if(!i2c_write(addr_bytes, 1 + double_byte))
      goto stop;

    chunk_len = (1 << page_size) - addr % (1 << page_size);
    if(chunk_len > len - written)
      chunk_len = len - written;
    if(!i2c_write(&buf[written], chunk_len))
      goto stop;

    // Stop condition (not repeated start!) is required to start the write cycle.
    if(!i2c_stop())
      return false;

    for(attempt = 0; timeout == 0 || attempt < timeout; attempt++) {
      if(i2c_start(chip << 1))
        break;
    }
    if(attempt == timeout)
      return false;

    addr += chunk_len;
    written += chunk_len;
  }

stop:
  i2c_stop();
  return written == len;
}
