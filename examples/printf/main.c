#include <fx2regs.h>
#include <fx2debug.h>
#include <stdio.h>


// An example of how to push characters to a software UART on PA0.
// As a test, the CLK speed is changed to check the code works for
// a variety of frequencies.
// 38400 likely works in all scenarios, YMMV with higher values.  


DEFINE_DEBUG_PUTCHAR_FN(PA0, 38400)

int main(void) {
  OEA = (1U<<0);
  PA0 = 1;

  CPUCS = 0;
  printf("Hello, world at 12MHz CPU clock!\r\n");
  
  CPUCS = _CLKSPD0;
  printf("Hello, world at 24MHz CPU clock!\r\n");
  
  CPUCS = _CLKSPD1;
  printf("Hello, world at 48MHz CPU clock!\r\n");
  
  while(1);
}
