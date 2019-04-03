/*
 * Automatic Plant Watering System
 *
 * Created on: 2018-12-29
 * Author: Richardo Prajogo
 *
 * Description : Automatically waters 3 different pot of plants sequentially if they are dry. Uses MSP430G2553 MCU.
 *
 */


#include <msp430.h>
#include <stdint.h>

#define PLANT1 1
#define PLANT2 2
#define PLANT3 3

#define MOISTURE_MIN 200
#define MOISTURE_MAX 600

/*
 * Disable watchdog
 */
void disableWatchdog(void)
{
    WDTCTL = WDTPW + WDTHOLD;
}

/*
 * Initialize the clock, crystal, and ports.
 */
void initialize(void)
{
    __disable_interrupt();                    // Disable global interrupts

    BCSCTL1 = CALBC1_1MHZ;                    // Set range to calibrated 1MHz
    DCOCTL  = CALDCO_1MHZ;                    // Set DCO step and modulation to calibrated 1MHz

    BCSCTL1 |= DIVA_3;                        // ACLK/8
    BCSCTL3 |= XCAP_3;                        //12.5pF cap- setting for 32768Hz crystal

    P1DIR |= 0x07;                            // Set P1.0, P1.1, P1.2 as output
    P1OUT &= ~(BIT0 + BIT1 + BIT2);           // Set all output pins to low

    P2DIR |= 0x0F;                            // Set P2.0, P2.1, P2.2, P2.3 as output
    P2OUT &= ~(BIT0 + BIT1 + BIT2 +BIT3);     // Set all output pins to low

    __enable_interrupt();                     //Enable global interrupts
}

/*
 * Second delay using TimerA.
 *
 * note: max seconds it can handle is 128.
 */
void delay(int seconds)
{
    if(seconds<120)
    {
        TACTL = TASSEL_1 + ID_3 + MC_1;        // ACLK, /8, upmode
        CCTL0 = CCIE;                          // CCR0 interrupt enabled
        CCR0 = (512* seconds)-1;               // 512 -> 1 sec, 30720 -> 1 min

        _BIS_SR(LPM3_bits + GIE);              // Enter LPM3 w/ interrupt
    }
}

/*
 * Hour delay using nested second delay.
 */
void hdelay(int hours)
{
    int i;
    for(i = hours*60; i>0; i--)
    {
        delay(60);
    }
}

/*
 * Check the moisture on plant 1 and water if necessary.
 */
void readyPlantOne()
{
    int checkMoisture = 0;
    P1OUT |= BIT0;
    __disable_interrupt();
    ADC10CTL1 = INCH_3 + ADC10DIV_3;
    ADC10CTL0 = ADC10SHT_1 + ADC10ON + ADC10IE;
    ADC10AE0 |= BIT3;
    __enable_interrupt();
    ADC10CTL0 |= ENC + ADC10SC;
    _BIS_SR(LPM0_bits + GIE);
    checkMoisture = ADC10MEM;
    P1OUT &= ~BIT0;
    if(checkMoisture<MOISTURE_MIN)
    {
        P2OUT |= BIT0;
        delay(1);
        while(checkMoisture<MOISTURE_MAX)
        {
            P2OUT |= BIT3;
            P1OUT |= BIT0;
            ADC10CTL0 |= ENC + ADC10SC;
            _BIS_SR(LPM0_bits + GIE);
            checkMoisture = ADC10MEM;
            P1OUT &= ~BIT0;
        }
        P2OUT &= ~BIT3;
        delay(2);
        P2OUT &= ~BIT0;
    }
    __disable_interrupt();
    ADC10CTL0 &= ~ENC;
    ADC10AE0 &= ~BIT3;
    __enable_interrupt();
}

/*
 * Check the moisture on plant 2 and water if necessary.
 */
void readyPlantTwo()
{
    int checkMoisture = 0;
    P1OUT |= BIT1;
    __disable_interrupt();
    ADC10CTL1 = INCH_4 + ADC10DIV_3;
    ADC10CTL0 = ADC10SHT_1 + ADC10ON + ADC10IE;
    ADC10AE0 |= BIT4;
    __enable_interrupt();
    ADC10CTL0 |= ENC + ADC10SC;
    _BIS_SR(LPM0_bits + GIE);
    checkMoisture = ADC10MEM;
    P1OUT &= ~BIT1;
    if(checkMoisture<MOISTURE_MIN)
    {
        P2OUT |= BIT1;
        delay(1);
        while(checkMoisture<MOISTURE_MAX)
        {
            P2OUT |= BIT3;
            P1OUT |= BIT1;
            ADC10CTL0 |= ENC + ADC10SC;
            _BIS_SR(LPM0_bits + GIE);
            checkMoisture = ADC10MEM;
            P1OUT &= ~BIT1;
        }
        P2OUT &= ~BIT3;
        delay(2);
        P2OUT &= ~BIT1;
    }
    __disable_interrupt();
    ADC10CTL0 &= ~ENC;
    ADC10AE0 &= ~BIT4;
    __enable_interrupt();
}

/*
 * Check the moisture on plant 3 and water if necessary.
 */
void readyPlantThree()
{
    int checkMoisture = 0;
    P1OUT |= BIT2;
    __disable_interrupt();
    ADC10CTL1 = INCH_5 + ADC10DIV_3;
    ADC10CTL0 = ADC10SHT_1 + ADC10ON + ADC10IE;
    ADC10AE0 |= BIT5;
    __enable_interrupt();
    ADC10CTL0 |= ENC + ADC10SC;
    _BIS_SR(LPM0_bits + GIE);
    checkMoisture = ADC10MEM;
    P1OUT &= ~BIT2;
    if(checkMoisture<MOISTURE_MIN)
    {
        P2OUT |= BIT2;
        delay(1);
        while(checkMoisture<MOISTURE_MAX)
        {
            P2OUT |= BIT3;
            P1OUT |= BIT2;
            ADC10CTL0 |= ENC + ADC10SC;
            _BIS_SR(LPM0_bits + GIE);
            checkMoisture = ADC10MEM;
            P1OUT &= ~BIT2;
        }
        P2OUT &= ~BIT3;
        delay(2);
        P2OUT &= ~BIT2;
    }
    __disable_interrupt();
    ADC10CTL0 &= ~ENC;
    ADC10AE0 &= ~BIT5;
    __enable_interrupt();
}

void main(void)
{
    disableWatchdog();

    initialize();

    while(1)
    {
        readyPlantOne();
        readyPlantTwo();
        readyPlantThree();
        hdelay(1);
    }
}

/*
 * Timer interrupt from second delay.
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer_A(void)
{
    _BIC_SR_IRQ(LPM3_bits);
}

/*
 * ADC interrupt when ADC has finished sampling and converting.
 */
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    _BIC_SR_IRQ(LPM0_bits);
}
