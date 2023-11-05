#ifndef FX2INTS_H
#define FX2INTS_H

enum fx2_core_interrupt {
  _INT_IE0       =  0, //< Pin PA0 / INT0#
  _INT_TF0       =  1, //< Internal, Timer 0
  _INT_IE1       =  2, //< Pin PA1 / INT1#
  _INT_TF1       =  3, //< Internal, Timer 1
  _INT_RI_TI_0   =  4, //< Internal, USART0
  _INT_TF2       =  5, //< Internal, Timer 2
  _INT_RESUME    =  6, //< Pin WAKEUP or Pin PA3/WU2
  _INT_RI_TI_1   =  7, //< Internal, USART1
  _INT_USB       =  8, //< Internal, USB
  _INT_I2C       =  9, //< Internal, I2C Bus Controller
  _INT_GPIF_IE4  = 10, //< Internal, GPIF/FIFOs or Pin INT4 (100 and 128 pin only)
  _INT_IE5       = 11, //< Pin INT5# (100 and 128 pin only)
  _INT_IE6       = 12, //< Pin INT6 (100 and 128 pin only)
};

/**
 * \name 8051 core interrupts
 * @{
 */

/**
 * Clears the I2C interrupt request.
 */
#define CLEAR_I2C_IRQ() \
  do { EXIF &= ~_I2CINT; } while(0)

/**
 * Clears the INT4# interrupt request.
 */
#define CLEAR_INT4_IRQ() \
  do { EXIF &= ~_IE4; } while(0)

/**
 * Clears the INT5# interrupt request.
 */
#define CLEAR_INT5_IRQ() \
  do { EXIF &= ~_IE5; } while(0)

void isr_IE0(void) __interrupt(_INT_IE0);
void isr_TF0(void) __interrupt(_INT_TF0);
void isr_IE1(void) __interrupt(_INT_IE1);
void isr_TF1(void) __interrupt(_INT_TF1);
void isr_RI_TI_0(void) __interrupt(_INT_RI_TI_0);
void isr_TF2(void) __interrupt(_INT_TF2);
void isr_RESUME(void) __interrupt(_INT_RESUME);
void isr_RI_TI_1(void) __interrupt(_INT_RI_TI_1);
void isr_USB(void) __interrupt(_INT_USB);
void isr_I2C(void) __interrupt(_INT_I2C);
void isr_GPIF_IE4(void) __interrupt(_INT_GPIF_IE4);
void isr_IE5(void) __interrupt(_INT_IE5);
void isr_IE6(void) __interrupt(_INT_IE6);

/**@}*/

/**
 * \name Autovectored USB interrupts
 * @{
 */

/**
 * Enables the autovectored USB interrupt and the corresponding jump table.
 */
#define ENABLE_USB_AUTOVEC() \
  do { EUSB = 1; INTSETUP |= _AV2EN; } while(0)

/**
 * Clears the main USB interrupt request.
 * This must be done before clearing the individual USB interrupt request latch.
 */
#define CLEAR_USB_IRQ() \
  do { EXIF &= ~_USBINT; } while(0)

void isr_SUDAV(void) __interrupt;
void isr_SOF(void) __interrupt;
void isr_SUTOK(void) __interrupt;
void isr_SUSPEND(void) __interrupt;
void isr_USBRESET(void) __interrupt;
void isr_HISPEED(void) __interrupt;
void isr_EP0ACK(void) __interrupt;
void isr_EP0IN(void) __interrupt;
void isr_EP0OUT(void) __interrupt;
void isr_EP1IN(void) __interrupt;
void isr_EP1OUT(void) __interrupt;
void isr_EP2(void) __interrupt;
void isr_EP4(void) __interrupt;
void isr_EP6(void) __interrupt;
void isr_EP8(void) __interrupt;
void isr_IBN(void) __interrupt;
void isr_EP0PING(void) __interrupt;
void isr_EP1PING(void) __interrupt;
void isr_EP2PING(void) __interrupt;
void isr_EP4PING(void) __interrupt;
void isr_EP6PING(void) __interrupt;
void isr_EP8PING(void) __interrupt;
void isr_ERRLIMIT(void) __interrupt;
void isr_EP2ISOERR(void) __interrupt;
void isr_EP4ISOERR(void) __interrupt;
void isr_EP6ISOERR(void) __interrupt;
void isr_EP8ISOERR(void) __interrupt;

/**@}*/

/**
 * \name Autovectored GPIF interrupts
 * @{
 */

/**
 * Enables the autovectored GPIF interrupt and the corresponding jump table.
 * Note that this makes it impossible to provide an INT4 handler.
 */
#define ENABLE_GPIF_AUTOVEC() \
  do { EX4 = 1; INTSETUP |= _AV4EN; } while(0)

/**
 * Clears the main GPIF interrupt request.
 * This must be done before clearing the individual GPIF interrupt request latch.
 */
#define CLEAR_GPIF_IRQ() \
  do { EXIF &= ~_IE4; } while(0)

void isr_EP2PF(void) __interrupt;
void isr_EP4PF(void) __interrupt;
void isr_EP6PF(void) __interrupt;
void isr_EP8PF(void) __interrupt;
void isr_EP2EF(void) __interrupt;
void isr_EP4EF(void) __interrupt;
void isr_EP6EF(void) __interrupt;
void isr_EP8EF(void) __interrupt;
void isr_EP2FF(void) __interrupt;
void isr_EP4FF(void) __interrupt;
void isr_EP6FF(void) __interrupt;
void isr_EP8FF(void) __interrupt;
void isr_GPIFDONE(void) __interrupt;
void isr_GPIFWF(void) __interrupt;

/**@}*/

#endif
