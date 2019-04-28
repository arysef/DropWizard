#include <msp430.h> 

//--------------------------------------------------------
// Project #2 - Multiplexed Seven-segment LED interface
// 2013.06.11 - MrChips
//--------------------------------------------------------

#define NUMBER_OF_DIGITS 2
#define DELAY 1000000
#define TIMER_DELAY 630

char seg[16];
char d[5];
char digit;

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

#pragma vector = TIMER0_A0_VECTOR
__interrupt void myTimerISR(void)
{
  digit = ++digit % NUMBER_OF_DIGITS;   //use MOD operator
  P2OUT = ~seg[d[digit]];
  P1OUT = 1 << (digit + 6);
}

void init(void)
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

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
  P1DIR  = 0xFF;       // enable segment outputs
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

void main( void )
{
    /*
    //TACTL
    BCSCTL3 |= LFXT1S_2;                      // ACLK = VLO
    WDTCTL = WDT_ADLY_16;                     // WDT 16ms, ACLK, interval timer (if the WDT were running at 32 kHz, it would be 1/16 ms)
    IE1 |= WDTIE;                             // Enable WDT interrupt

    P2DIR |= BIT3;                            // Set P1.6 to output direction 1.6 is RED
    P2SEL |= BIT3;                            // Set P1.6 to primary function (output for TA1.0)
    P2SEL2 &= ~BIT3;                          // Ensure secondary function isn't enabled

    P2DIR |= BIT1;                            // Set P2.2 to output direction 2.1 is green
    P2SEL |= BIT1;                            // Set P2.2 to primary function
    P2SEL2 &= ~BIT1;                          // Ensure secondary function isn't enable

    P2DIR |= BIT5;                            // Set P2.2 to output direction 2.5 is blue
    P2SEL |= BIT5;                            // Set P2.2 to primary function
    P2SEL2 &= ~BIT5;                          // Ensure secondary function isn't enable

    TA1CCR0 = 100;                            // Set upper bound for counter of Timer A1 to 100 (120 Hz)                           // Set duty cycle of Timer A1 to 100/100 (pin on 100% of period)
    TA0CCR0 = 100;                            // Set upper bound for counter of Timer A0 to 100 (120 Hz)                         // Set duty cycle of Timer A1 to 0/100 (pin on 0% of period)

    TA1CCR1 = 0;
    TA1CCR2 = 0;
    TA0CCR1 = 0;
    TA0CCTL1 = OUTMOD_3;                      // Set timer to PWM set/reset mode (on for 0 -> CCR1, off for CCR1 -> CCR0)
    TA1CCTL1 = OUTMOD_3;                      // Set timer to PWM set/reset mode (on for 0 -> CCR1, off for CCR1 -> CCR0)
    TA1CCTL2 = OUTMOD_3;
    TA1CTL |= TASSEL_1 + MC_1;                // Chooses ACLK as the source and sets the timer in up mode
    TA0CTL |= TASSEL_1 + MC_1;                // Chooses ACLK as the source and sets the timer in up mode

    //int dir = 1;                              // Flag to know whether pin controlled by TA0 is counting up or down
    ADC10CTL0 &= ~ENC;
    ADC10CTL1 = INCH_0 + ADC10DIV_3 + ADC10SSEL_1;         // Temp Sensor ADC10CLK/4
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ADC10IE + REF2_5V;
    ADC10AE0 |= BIT1;
    */
    //SREF_1 means use internal reference
    int test = 1;
    int temp = 0;
    int centered;
    int init_temp;
    int red, blue, green;
    // Code from https://www.embeddedrelated.com/showarticle/199.php begins
    WDTCTL = WDTPW + WDTHOLD; // Stop WDT

    BCSCTL1 = CALBC1_1MHZ; // Set range

    DCOCTL = CALDCO_1MHZ;

    BCSCTL2 &= ~(DIVS_3); // SMCLK = DCO = 1MHz

    //P1DIR |= LED0 + LED1;

    P1SEL |= BIT1; //ADC Input pin P1.1

    //P1OUT &= ~(LED0 + LED1);
    ADC10CTL1 = INCH_5 + ADC10DIV_3 ; // Channel 5, ADC10CLK/4

    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE; //Vcc & Vss as reference

    ADC10AE0 |= BIT2; //P1.5 ADC option
    __enable_interrupt();
    // Code from website mentioned above ends
    /*
    __bis_SR_register(GIE);                   // Enables interrupts
    ADC10CTL0 |= ENC + ADC10SC;               // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);          // LPM0 with interrupts enabled
    init_temp = ADC10MEM;
    */
    while (1) {
        //__delay_cycles(1000); // Wait for ADC Ref to settle

        ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start

        __bis_SR_register(CPUOFF + GIE); // LPM0 with interrupts enabled

        test = ADC10MEM;

        //TA1CCR2 is blue, TA0CCR1 is green, TA1CCR1 is red

        /*
        if (temp >= init_temp - 7) {
          TA1CCR2 = 100;
          TA0CCR1 = 0;
          TA1CCR1 = 0;
        } else if (temp >= init_temp - 12 && temp <= init_temp -7) {
          TA1CCR2 = 100;
          TA0CCR1 = 100;
          TA1CCR1 = 0;
        } else if (temp >= init_temp - 16 && temp <= init_temp -12) {
          TA1CCR2 = 0;
          TA0CCR1 = 100;
          TA1CCR1 = 0;
        } else if (temp >= init_temp - 19 && temp <= init_temp -16) {
          TA1CCR2 = 0;
          TA0CCR1 = 100;
          TA1CCR1 = 100;
        } else{
          TA1CCR2 = 0;
          TA0CCR1 = 0;
          TA1CCR1 = 100;
        }
        */
        if ((temp < 500) && (test == 1)) {
            // blue
            TA1CCR2 = 0;
            TA0CCR1 = 0;
            TA1CCR1 = 100;
            temp = 60;
            //test = 1;
            //__delay_cycles(250000);
        } else {
            /*
            TA1CCR2 = 100;
            TA0CCR1 = 0;
            TA1CCR1 = 0;
            */
            // red
            TA1CCR2 = 100;
            TA0CCR1 = 100;
            TA1CCR1 = 100;
            test = 0;
            //__delay_cycles(250000);
        }
        //__bis_SR_register(LPM3_bits);         // Enter LPM3 w/interrupt
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
    __bic_SR_register_on_exit(LPM3_bits);     // On exit of ISR, clear bits in status register to leave LPM3
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
