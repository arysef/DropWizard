#include <msp430.h> 

//--------------------------------------------------------
// Project #2 - Multiplexed Seven-segment LED interface
// 2013.06.11 - MrChips
//--------------------------------------------------------

#define NUMBER_OF_DIGITS 2
#define DELAY 1000000
#define TIMER_DELAY 630

#define LED0 BIT3 // GREEN

#define LED1 BIT1 // RED

unsigned int value=0;

char seg[16];
char d[5];
char digit;
int state;
// simple software delay
void delay(unsigned long d)
{
  unsigned long i;
  for (i = 0; i < d; i++);
}

void display_hex(unsigned short v)
{ // enter with 16-bit integer v
  // fill global array d[ ]
  short i;
  for (i = 0; i < 4; i++)
  {
     d[i] = v % 16;
     v = v/16;
  }
}

void display_BCD(unsigned short v)
{// enter with 16-bit integer v
 // fill global array d[ ]
  short i;
  for (i = 0; i < 5; i++)
  {
     d[i] = v % 10;
     v = v/10;
  }
}


void init_seg_display(void)
{
  // Stop watchdog timer to prevent time out reset
  //WDTCTL = WDTPW + WDTHOLD;

  // initialize Timer0_A
  TA0CCR0 = TIMER_DELAY;                  // set up terminal count
  TA0CTL = TASSEL_2 + ID_3 + MC_1;  // configure and start timer

  // enable interrupts
  TA0CCTL0 |= CCIE;   // enable timer interrupts
  __enable_interrupt();    // set GIE in SR
  P2SEL  &= ~BIT6;
  P2SEL  &= ~BIT7;
  P2SEL2 &= ~BIT6;
  P2SEL2 &= ~BIT7;
  P1DIR  = BIT6 + BIT7;       // enable segment outputs
  P2DIR  = 0xFF;   // enable digit select

  // create 7-segment table
  seg[0]  = 0x3F;
  seg[1]  = 0x06;
  seg[2]  = 0x5B;
  seg[3]  = 0x4F;
  seg[4]  = 0x66;
  seg[5]  = 0x6D;
  seg[6]  = 0x7D;
  seg[7]  = 0x07;
  seg[8]  = 0x7F;
  seg[9]  = 0x67;
  seg[10] = 0x77;
  seg[11] = 0x7C;
  seg[12] = 0x39;
  seg[13] = 0x5E;
  seg[14] = 0x79;
  seg[15] = 0x71;

  digit = 0;
}

void init_accelerometer(void) {
    BCSCTL1 = CALBC1_1MHZ; // Set range
    DCOCTL = CALDCO_1MHZ;
    BCSCTL2 &= ~(DIVS_3); // SMCLK = DCO = 1MHz

    P2DIR |= LED0 + LED1;
    P1SEL |= BIT1;
    P2OUT &= ~(LED0 + LED1 + BIT5);
    /* Configure ADC Channel */

    ADC10CTL1 = INCH_1 + ADC10DIV_3 ; // Channel 1, ADC10CLK/4
    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE; //Vcc & Vss as reference
    ADC10AE0 |= BIT1; //P1.1 ADC option
}

void init_button(void) {
    P1DIR &= ~BIT3;
    P1OUT |= BIT3;
    P1REN |= BIT3;
    P1IES |= BIT3;
    P1IE  |= BIT3;
    P1IFG &= ~BIT3;
}

void main( void )
{
    WDTCTL = WDT_ADLY_16;                     // WDT 16ms, ACLK, interval timer (if the WDT were running at 32 kHz, it would be 1/16 ms)
    init_seg_display();
    init_accelerometer();
    init_button();
    state = 0;
    int counter = 0;
    __enable_interrupt(); // Enable interrupts.

    while(1) {
         //__delay_cycles(1000); // Wait for ADC Ref to settle

         ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start
         __bis_SR_register(CPUOFF + GIE); // LPM0 with interrupts enabled
         value = ADC10MEM;

         if (state == 0) {
             P2OUT &= ~(LED0 + LED1);
             P2OUT |= LED0;

         } else {
             if (value<950 && state == 1) {
                P2OUT &= ~(LED0 + LED1);
                P2OUT |= LED0;
             } else {
                P2OUT &= ~(LED0 + LED1);
                P2OUT |= LED1;
                state = 2;
             }
         }
         if (state == 1) {
             counter += 1;
             // current issue: watchdog timer not working to wake up system (not going into interrupt? haven't checked)
             //__bis_SR_register(CPUOFF + GIE);
         }

      }
 /*
 unsigned int n;
 init();

 n = 0;


 while (1)
 {
   P2OUT = ~BIT5;
   P1OUT |= BIT1;
   P1OUT |= BIT0;

   n++;
   display_BCD(n);
   __delay_cycles(1000000);

 }
 */

}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void myTimerISR(void)
{
  digit = ++digit % NUMBER_OF_DIGITS;   //use MOD operator
  //P2OUT = ~seg[d[digit]];
  //P1OUT = 1 << (digit + 6);
}

// Watchdog Timer interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR                     // Do this ISR for interrupt vector from watchdog timer
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
    __bic_SR_register_on_exit(CPUOFF);     // On exit of ISR, clear bits in status register to leave LPM3
}

//From adc10_temp example
// ADC10 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
#else
#error Compiler not supported!
#endif
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

// Button interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT1_VECTOR))) Port_1 (void)
#else
#error Compiler not supported!
#endif
{
    P2IE  &= ~BIT3;      // Disable interrupts while ISR running
    // Wait to Exit ISR Until Finger Lifted
    while((P1IN & BIT3) == 0x00){
        // Sets duty cycle to 50%
    }
    state = 1;
    //TA1CCR1 = 0;
    P1IFG &= ~(BIT3);
    P1IE  |= BIT3;       // Re-enable interrupts

}

