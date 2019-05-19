#include <fx2regs.h>
#include <fx2debug.h>
#include <stdio.h>

DEFINE_DEBUG_PUTCHAR_FN(PA0, 57600)

int main() {
  // Any of these will work at 57600 baud:
  CPUCS = 0;
  // CPUCS = _CLKSPD0;
  // CPUCS = _CLKSPD1;

  OEA = (1U<<0);
  PA0 = 1;

  printf("Hello, world!\n");
  while(1);
}
