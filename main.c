#include <msp430.h> 
#include <math.h>

//--------------------------------------------------------
// Credit: Code for seven segment displays based off https://forum.allaboutcircuits.com/blog/msp430-multiplexed-7-segment-displays.559/
//--------------------------------------------------------

#define NUMBER_OF_DIGITS 2
#define DELAY 1000000
#define TIMER_DELAY 630

#define LED0 BIT3 // GREEN

#define LED1 BIT1 // RED

#define K 0.148
#define DEVICE_MASS 0.039
#define GRAVITY 9.8

unsigned int value=0;

char seg[16];
char d[5];
char digit;
int state;
int count = 0;
char cur_counting;
float result;

/*
 * Function to change value to be displayed by seven segment displays
 */
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
  P2SEL2 &= ~BIT6;
  P2SEL  &= ~BIT1;
  P2SEL2 &= ~BIT1;
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

    P1SEL |= BIT1;

    /* Configure ADC Channel */
    ADC10CTL1 = INCH_1 + ADC10DIV_3 ; // Channel 1, ADC10CLK/4
    ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE; //Vcc & Vss as reference
    ADC10AE0 |= BIT1; //P1.1 ADC option
}

void init_button(void) {

    P2SEL  &= ~BIT7;
    P2SEL2 &= ~BIT7;


    P2DIR &= ~BIT7;
    P2OUT |= BIT7;
    P2REN |= BIT7;
    P2IES |= BIT7;
    P2IE  |= BIT7;
    P2IFG &= ~BIT7;

}

void main( void )
{
    WDTCTL = WDTPW + WDTHOLD; // watchdog timer is unused, so turns it off


    init_accelerometer();
    init_seg_display();
    init_button();
    state = 0;

    __enable_interrupt();

    while(1) {

         ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start
         __delay_cycles(50);
         value = ADC10MEM;

         // state 0: waiting to start
         if (state == 0) {
             display_BCD(23);
         } else if (state == 1){ // state 1: start counting, set counter to 0, go to state 2
                 count = 0;
                 cur_counting = 1;
                 display_BCD(state);
                 state = 2;
         } else if (state == 2) { // state 2: counting and waiting for impact
             if (value > 975) { // transition to state 3: stop counting and compute value to output
                cur_counting = 0;
                count = (int) round((DEVICE_MASS/K)*log(cosh((count/200.0)/(sqrt(DEVICE_MASS/GRAVITY))))*3 - 1.5); // computation to output value in feet
                state = 3;
             }
         } else { // state 3: display computed value (feet dropped)
             display_BCD(count);
         }

      }

}

// Used to display the correct digits on each seven segment display
#pragma vector = TIMER0_A0_VECTOR
__interrupt void myTimerISR(void)
{
  digit = ++digit % NUMBER_OF_DIGITS;   //use MOD operator
  P2OUT = ~seg[d[digit]];
  P1OUT = 1 << (digit + 6);
  if (cur_counting == 1) {
     count++;
  }


}

// Watchdog Timer interrupt service routine,  not currently being used
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR                     // Do this ISR for interrupt vector from watchdog timer
__interrupt void watchdog_timer(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
    //__bic_SR_register_on_exit(CPUOFF);     // On exit of ISR, clear bits in status register to leave LPM3


}

// ADC10 interrupt service routine, not currently being used
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

// Button interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2 (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT2_VECTOR))) Port_2 (void)
#else
#error Compiler not supported!
#endif
{
    P2IE  &= ~BIT7;      // Disable interrupts while ISR running

    // Wait to Exit ISR Until Finger Lifted
    while((P2IN & BIT7) == 0x00);

    state = 1;
    P2IFG &= ~(BIT7);
    P2IE  |= BIT7;       // Re-enable interrupts

}

