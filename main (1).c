#include <msp430fr5969.h>
#include <stdint.h>

#define THRS_DARK 20                   // Threshold value for darkness.


volatile uint16_t adc_val = 0;

int flag = 0;


void uartTx(uint16_t data);

/* MAIN PROGRAM */
void main(void)
{
    // Stop watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;


    // Initialize the clock system to generate 1 MHz DCO clock.
    CSCTL0_H    = CSKEY_H;              // Unlock CS registers.
    CSCTL1      = DCOFSEL_0;            // Set DCO to 1 MHz, DCORSEL for
                                        //   high speed mode not enabled.
    CSCTL2      = SELA__VLOCLK |        // Set ACLK = VLOCLK = 10 kHz.
                  SELS__DCOCLK |        //   Set SMCLK = DCOCLK.
                  SELM__DCOCLK;         //   Set MCLK = DCOCLK.
                                        // SMCLK = MCLK = DCOCLK = 1 MHz.
    CSCTL3      = DIVA__1 |             //   Set ACLK divider to 1.
                  DIVS__1 |             //   Set SMCLK divider to 1.
                  DIVM__1;              //   Set MCLK divider to 1.
                                        // Set all dividers to 1.
    CSCTL0_H    = 0;                    // Lock CS registers.


    // Initialize unused GPIOs to minimize energy-consumption.
    // Port 1:
    P1DIR = 0xFF;
    P1OUT = 0x00;
    // Port 2:
    P2DIR = 0xFF;
    P2OUT = 0x00;
    // Port 3:
    P3DIR = 0xFF;
    P3OUT = 0x00;
    // Port 4:
    P4DIR = 0xFF;
    P4OUT = 0x00;
    // Port J:
    PJDIR = 0xFFFF;
    PJOUT = 0x0000;

    // Initialize port 1:
    P1DIR |= BIT0;                      // P1.0 - output for LED2, off.
    P1OUT &= ~BIT0;

    // Initialize port 4:
    P4DIR |= BIT6;                      // P4.6 - output for LED1, off.
    P4OUT &= ~BIT6;


    /* TODO INIT ADC PIN */

    // Initialize ADC input pins:

    P4SEL0 |= BIT2;                             // Enable ADC channel inputs P4.2
    P4SEL1 |= BIT2;
    P4DIR  |= BIT2;                             // Setting P4.2 to output direction
    // Initialize port 2:
    // Select Tx and Rx functionality of eUSCI0 for hardware UART.
    // P2.0 - UART Tx (UCA0TXD).
    // P2.1 - UART Rx (UCA0RXD).
    P2SEL0 &= ~(BIT1 | BIT0);
    P2SEL1 |= BIT1 | BIT0;

    // Disable the GPIO power-on default high-impedance mode to activate the
    // previously configured port settings.
    PM5CTL0 &= ~LOCKLPM5;


    /* Initialization of the serial UART interface */

    UCA0CTLW0 |= UCSSEL__SMCLK |        // Select clock source SMCLK = 1 MHz.
                 UCSWRST;               // Enable software reset.
    // Set Baud rate of 9600 Bd.
    // Recommended settings available in table 30-5, p. 779 of the User's Guide.
    UCA0BRW = 6;                        // Clock prescaler of the
                                        //   Baud rate generator.
    UCA0MCTLW = UCBRF_8 |               // First modulations stage.
                UCBRS5 |                // Second modulation stage.
                UCOS16;                 // Enable oversampling mode.
    UCA0CTLW0 &= ~UCSWRST;              // Disable software reset and start
                                        //   eUSCI state machine.


    /* TODO INIT ADC REF */

    /* Initialization of the reference voltage generator */

    // Configure the internal voltage reference for 2.5 V:
    while(REFCTL0 & REFGENBUSY); // If reference generator is busy, wait
    REFCTL0 |= REFVSEL_2 | REFON; // Select internal reference voltage = 2.5V and switch on the internal reference voltage

    while(!(REFCTL0 & REFGENRDY));    // Waiting for the reference generator to settle

    /* TODO INIT ADC FAST */

    // Configure the analog-to-digital converter such that it samples
    // as fast as possible.


    ADC12CTL0 = 0;                      // Disable ADC trigger.
    ADC12CTL0 = ADC12ON |               // Turn on ADC.
                ADC12MSC |             // Multiple sample and conversion mode.
                ADC12SHT0_6;            // Sample-and-hold time of 128 cycles of MODCLK.
    ADC12CTL1 = ADC12SHP |              // Use sampling timer.
                ADC12CONSEQ_2;          // Repeat-single-channel mode.
    ADC12CTL2 |= ADC12RES_2;            // 12 bit conversion resolution.
    ADC12IER0 |= ADC12IE0;              // Enable ADC conversion interrupt.
    ADC12MCTL0 = ADC12VRSEL_1 | ADC12INCH_10;    // Select ADC input channel A10 ; VR+ = VREF buffered, VR- = AVSS
    ADC12CTL0 |= ADC12ENC | ADC12SC;               //  Enable ADC trigger; Trigger sampling.



    // Initialize timer A0 to trigger ADC12 at 4 Hz:

    TA0CTL = TASSEL_1 + MC_1;       // Timer A set to ACLK, UPMODE(counts upto CCR0)
    TA0CCTL1 = OUTMOD_3;                // TACCTL1 set/reset.
    TA0CCR0 = 2500;                     // Period time: 250 ms
                                        //   -> 4 Hz sampling rate.



    TA0CCR1 = 250;                       // Duty cycle: 2.5  ms
                                         //   -> 2.5 ms sample-and-hold time.


   // Let LED 1 blink according to trigger:
   TA0CCTL1 |= CCIE;                   // Enable interrupt of CCR0.

   while(ADC12IFGR0 & ADC12IFG10);     // Wait for first 8 bit sample.

   // Enable interrupts globally.
    __bis_SR_register(GIE);

    /* MAIN LOOP */
    while(1)
    {
        uartTx(adc_val);
        if(adc_val < THRS_DARK || flag == 1)
        {
        // Initialize timer A0 to trigger ADC12 at 4 Hz:
            //TA0CCTL1 |= CCIE;                   // Enable interrupt of CCR1.
           TA1CCR0 = 37500;                     // Period time: 60 sec; 1 cycle takes 625 cycles

           TA1CCTL1 = OUTMOD_3;                // TACCTL1 set/reset.
           TA1CCR1 = 63;                       // Duty cycle: 100 ms

           TA1CTL = TASSEL_1 + MC_1 + ID_3;       // Timer A set to ACLK, UPMODE(counts upto CCR0), prescaler 8
           TA1EX0 = TAIDEX_1;                     // Input divider expansion , divide by 2
           //uartTx(adc_val);
           //P4OUT |= BIT6;              // Turn on LED 1
           TA1CCTL0 |= CCIE;                   // Enable interrupt of CCR0.
           TA1CCTL1 |= CCIE;                   // Enable interrupt of CCR1.
           P4OUT |= BIT6;             // Turn on LED 1.
                   P1OUT |= BIT0;             // Turn on LED 2.
        }
        else if(adc_val > THRS_DARK || flag == 2)
        {
            //TA0CCTL1 |= CCIE;           // Enable interrupt of CCR1.
            TA1CCR0 = 6250;             // Period time: 10 sec; 1 cycle takes 625 cycles
            TA1CCTL1 |= OUTMOD_3;       // TACCTL1 set/reset.
            TA1CCR1 = 31;               // Duty cycle: 50 ms

            TA1CTL = TASSEL_1 + MC_1 + ID_3 ; // Timer A set to ACLK, UPMODE(counts upto CCR0), prescaler 8
            TA1EX0 = TAIDEX_1;          // Input divider expansion , divide by 2
            uartTx(adc_val);
            //P4OUT |= BIT6;              // Turn on LED 1
            TA1CCTL0 |= CCIE;                   // Enable interrupt of CCR0.
            TA1CCTL1 |= CCIE;                   // Enable interrupt of CCR1.
            P4OUT |= BIT6;             // Turn on LED 1.
                    P1OUT |= BIT0;             // Turn on LED 2.
        }

    }

}
    /* ### ADC ### */

    /* ISR ADC */
    #pragma vector = ADC12_VECTOR
    __interrupt void ADC12_ISR(void)
    {
        switch(__even_in_range(ADC12IV, ADC12IV_ADC12RDYIFG))
        {
        case ADC12IV_NONE:        break;        // Vector  0:  No interrupt
        case ADC12IV_ADC12OVIFG:  break;        // Vector  2:  ADC12MEMx Overflow
        case ADC12IV_ADC12TOVIFG: break;        // Vector  4:  Conversion time overflow
        case ADC12IV_ADC12HIIFG:  break;        // Vector  6:  ADC12BHI
        case ADC12IV_ADC12LOIFG:  break;        // Vector  8:  ADC12BLO
        case ADC12IV_ADC12INIFG:  break;        // Vector 10:  ADC12BIN
        case ADC12IV_ADC12IFG0:                 // Vector 12:  ADC12MEM0 Interrupt

            adc_val = ADC12MEM0;

            break;
        case ADC12IV_ADC12IFG1:   break;        // Vector 14:  ADC12MEM1
        case ADC12IV_ADC12IFG2:   break;        // Vector 16:  ADC12MEM2
        case ADC12IV_ADC12IFG3:   break;        // Vector 18:  ADC12MEM3
        case ADC12IV_ADC12IFG4:   break;        // Vector 20:  ADC12MEM4
        case ADC12IV_ADC12IFG5:   break;        // Vector 22:  ADC12MEM5
        case ADC12IV_ADC12IFG6:   break;        // Vector 24:  ADC12MEM6
        case ADC12IV_ADC12IFG7:   break;        // Vector 26:  ADC12MEM7
        case ADC12IV_ADC12IFG8:   break;        // Vector 28:  ADC12MEM8
        case ADC12IV_ADC12IFG9:   break;        // Vector 30:  ADC12MEM9
        case ADC12IV_ADC12IFG10:  break;        // Vector 32:  ADC12MEM10
        case ADC12IV_ADC12IFG11:  break;        // Vector 34:  ADC12MEM11
        case ADC12IV_ADC12IFG12:  break;        // Vector 36:  ADC12MEM12
        case ADC12IV_ADC12IFG13:  break;        // Vector 38:  ADC12MEM13
        case ADC12IV_ADC12IFG14:  break;        // Vector 40:  ADC12MEM14
        case ADC12IV_ADC12IFG15:  break;        // Vector 42:  ADC12MEM15
        case ADC12IV_ADC12IFG16:  break;        // Vector 44:  ADC12MEM16
        case ADC12IV_ADC12IFG17:  break;        // Vector 46:  ADC12MEM17
        case ADC12IV_ADC12IFG18:  break;        // Vector 48:  ADC12MEM18
        case ADC12IV_ADC12IFG19:  break;        // Vector 50:  ADC12MEM19
        case ADC12IV_ADC12IFG20:  break;        // Vector 52:  ADC12MEM20
        case ADC12IV_ADC12IFG21:  break;        // Vector 54:  ADC12MEM21
        case ADC12IV_ADC12IFG22:  break;        // Vector 56:  ADC12MEM22
        case ADC12IV_ADC12IFG23:  break;        // Vector 58:  ADC12MEM23
        case ADC12IV_ADC12IFG24:  break;        // Vector 60:  ADC12MEM24
        case ADC12IV_ADC12IFG25:  break;        // Vector 62:  ADC12MEM25
        case ADC12IV_ADC12IFG26:  break;        // Vector 64:  ADC12MEM26
        case ADC12IV_ADC12IFG27:  break;        // Vector 66:  ADC12MEM27
        case ADC12IV_ADC12IFG28:  break;        // Vector 68:  ADC12MEM28
        case ADC12IV_ADC12IFG29:  break;        // Vector 70:  ADC12MEM29
        case ADC12IV_ADC12IFG30:  break;        // Vector 72:  ADC12MEM30
        case ADC12IV_ADC12IFG31:  break;        // Vector 74:  ADC12MEM31
        case ADC12IV_ADC12RDYIFG: break;        // Vector 76:  ADC12RDY
        default: break;
        }
    }

    /* ISR TIMER A0 - CCR0 */
    #pragma vector = TIMER1_A0_VECTOR
    __interrupt void Timer1_A0_ISR (void)
    {


    }



    /* ISR TIMER A0 - CCR1, CCR2 AND TAIFG */
    #pragma vector = TIMER1_A1_VECTOR
    __interrupt void Timer1_A1_ISR(void)
    {
        switch (__even_in_range(TA1IV, TA1IV_TA1IFG))
        {
       case TA1IV_TA1CCR1: // TA1 CCR1

        //while(!(TA0CCTL1 & CCIFG)); //Timer  counts from 0 to TA0CCR1
        P4OUT &= ~BIT6;             // Turn off LED 1.// TA1 CCR0
        P1OUT &= ~BIT0;

        TA1CCR1 = 0;
        TA1CTL |= TACLR;
        TA1CTL = 0;
        TA1CCTL1 &= ~CCIFG;
        while(!(TA1CCTL0 & CCIFG))  //Timer  counts from 0 to TA0CCR0
        {
            uartTx(adc_val);
            if(adc_val > THRS_DARK) // Check whether light turned on
            {
                flag = 2;          // Set the value of flag as 2
                TA1CCR0 &= ~CCIFG;
                break;              //  and break the loop that was on for 60 sec
            }
            /*else if(adc_val < THRS_DARK) // Check whether light turned off
            {
                flag = 1;           // Set the value of flag as 1
                TA1CCR0 &= ~CCIFG;
                break;              // break the while loop that was blinking LED1 every 10 sec
            }*/
        }TA1CCTL0 &= ~CCIFG;



        case TA1IV_TA1IFG:
            break;
        }
    }

/* ### SERIAL INTERFACE ### */

void uartTx(uint16_t data)
{
    // Higher byte
    while((UCA0STATW & UCBUSY));            // Wait while module is busy.
    UCA0TXBUF = (0xFF00 & data) >> 8;       // Transmit data byte.
    // Lower byte
    while((UCA0STATW & UCBUSY));            // Wait while module is busy.
    UCA0TXBUF = 0x00FF & data;              // Transmit data byte.
}




