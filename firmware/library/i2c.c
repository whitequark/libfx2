#include <fx2i2c.h>
#include <fx2regs.h>
#include <fx2delay.h>

volatile bool i2c_cancel;

bool i2c_wait(bool need_ack) {
  while(!(I2CS & _DONE)) {
    if(i2c_cancel) {
      i2c_cancel = false;
      break;
    }
  }

  if(I2CS & _BERR)
    return false;
  if(need_ack)
    return I2CS & _ACK;
  return true;
}

bool i2c_start(uint8_t chip) {
  I2CS  = _START;
  I2DAT = chip;
  return i2c_wait(/*need_ack=*/true);
}

bool i2c_stop() {
  I2CS  = _STOP;
  while(I2CS & _STOP);

  if(I2CS & _BERR)
    return false;
  return true;
}

bool i2c_write(uint8_t *buf, uint16_t len) {
  uint16_t i = 0;

  if(len == 0)
    return true;

  for(i = 0; i != len; i++) {
    I2DAT = buf[i];
    if(!i2c_wait(/*need_ack=*/true))
      goto end;
  }

end:
  return i == len;
}

bool i2c_read(uint8_t *buf, uint16_t len) {
  uint16_t i = 0;

  if(len == 0)
    return true;

  if(len == 1)
    I2CS = _LASTRD;
  I2DAT; // prime the transfer

  for(i = 0; i != len; i++) {
    if(!i2c_wait(/*need_ack=*/false))
      goto end;
    if(i + 2 == len)
      I2CS = _LASTRD;
    else if(i + 1 == len)
      I2CS = _STOP;
    buf[i] = I2DAT;
  }

  while(I2CS & _STOP);
  if(I2CS & _BERR)
    return false;

end:
  return i == len;
}
