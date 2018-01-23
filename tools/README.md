The `regtxt2c.py` script allows semi-automatic generation of register definition from the datasheet by parsing the text that results from copying the register summary table. The copied text is garbled when the Description field is multi-line and in some other cases, and has many typos, so it was manually adjusted and put into `regs.txt`. If there is an error, the `fx2regs.h` header can be regenerated with:

    python3 tools/regtxt2c.py <tools/regs.txt >firmware/library/include/fx2regs.h

Aside from typos and general reformatting, the following adjustments were done manually in `regs.txt`:

  * added "#" prior to section names
  * removed "([NOT] bit addressable)" in descriptions
  * upper-cased all register and bit names and removed extra dashes
  * removed all high speed `EPnFIFOPFb` registers
  * in `TMOD`, added `_0` and `_1` suffixes for high and low nibble bits
  * in `PORTECFG`, renamed `INT6` to `INT6EX`
  * in `EPnGPIFPFSTOP`, renamed `FIFOnFLAG` to `FIFOFLAG`
