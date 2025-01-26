#include <fx2regs.h>
#include <fx2debug.h>
#include <stdio.h>

DEFINE_DEBUG_PUTCHAR_FN(PA0, 38400)

int main(void) {
  // Any of these will work at 38400 baud:

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
