#include <fx2regs.h>
#include <fx2ints.h>

// Register an interrupt handler for TIMER0 overflow
void isr_TF0(void) __interrupt(_INT_TF0) {
  static int i;
  if(i++ % 64 == 0)
    PA0 = !PA0;
}

int main(void) {
  // Configure pins
  PA0 = 1;      // set PA0 to high
  OEA = 0b1;    // set PA0 as output

  // Configure TIMER0
  TCON = _M0_0; // use 16-bit counter mode
  ET0 = 1;      // generate an interrupt
  TR0 = 1;      // run

  // Enable interrupts
  EA  = 1;
  while(1);
}
