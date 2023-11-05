#include <fx2i2c.h>
#include <fx2regs.h>
#include <fx2delay.h>

volatile bool i2c_cancel;

bool i2c_wait(bool need_ack) {
  while(!(I2CS & _DONE)) {
    if(i2c_cancel) {
      i2c_cancel = false;
      return false;
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

bool i2c_stop(void) {
  if(I2CS & _BERR)
    return false;

  I2CS = _STOP;
  while(I2CS & _STOP);

  return true;
}

bool i2c_write(const uint8_t *buf, uint16_t len) {
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
    if(i + 1 == len) {
      // TRM 13.5.4.14: Read the final byte from I2DAT immediately (the next
      // instruction) after setting the STOP bit.
      // This is quite tricky to actually achieve; we use the autopointers
      // and the fact that I2DAT immediately follows I2CS.
      // Alternatively, MPAGE could be used.
      uint8_t tmp;
__asm
      mov  _AUTOPTRSETUP, #0b11 ; APTR1INC|APTREN
      mov  _AUTOPTRH1, #(_I2CS >> 8)
      mov  _AUTOPTRL1, #(_I2CS & 0xff)
      mov  dptr, #_XAUTODAT1
      mov  a, #0b01000000 ; STOP
      movx @dptr, a
      movx a, @dptr
__endasm;
      // This ensures that ACC won't get overwritten by the compiler before
      // it has a chance to get saved to buf[i].
      tmp = ACC;
      buf[i] = tmp;
    } else {
      buf[i] = I2DAT;
    }
  }

  if(I2CS & _BERR)
    return false;

  while(I2CS & _STOP);

end:
  return i == len;
}
