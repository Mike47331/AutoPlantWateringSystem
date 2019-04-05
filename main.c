/*
 * Automatic Plant Watering System
 *
 * Created on: 2018-12-30
 * Author: Richardo Prajogo
 *
 * Description: Automatically waters 3 different pot of plants sequentially if they are dry. Uses MSP430G2553 MCU.
 *
 * Changelog
 * 2018-12-30: Compartmentalized into 4 functions.
 */


#include <msp430.h>
#include <stdint.h>

#define MOISTURE_MIN 200
#define MOISTURE_MAX 600

struct plantProperty
{
    int enableADC;
    int selectADC;
    int sampleADC;
    int activateSolenoid;
};

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
    __disable_interrupt();                      // Disable global interrupts

    BCSCTL1 = CALBC1_1MHZ;                      // Set range to calibrated 1MHz
    DCOCTL  = CALDCO_1MHZ;                      // Set DCO step and modulation to calibrated 1MHz

    BCSCTL1 |= DIVA_3;                          // ACLK/8
    BCSCTL3 |= XCAP_3;                          // 12.5pF cap- setting for 32768Hz crystal

    P1DIR |= 0x07;                              // Set P1.0, P1.1, P1.2 as output
    P1OUT &= ~(BIT0 + BIT1 + BIT2);             // Set all output pins to low

    P2DIR |= 0x0F;                              // Set P2.0, P2.1, P2.2, P2.3 as output
    P2OUT &= ~(BIT0 + BIT1 + BIT2 +BIT3);       // Set all output pins to low

    ADC10CTL1 |= ADC10DIV_3;
    ADC10CTL0 = ADC10SHT_1 + ADC10ON + ADC10IE; // Turn on ADC, enable interrupt and sample for 8x ADC10CLKs

    __enable_interrupt();                       // Enable global interrupts
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
        TACTL = TASSEL_1 + ID_3 + MC_1;          // ACLK, /8, upmode
        CCTL0 = CCIE;                            // CCR0 interrupt enabled
        CCR0 = (512* seconds)-1;                 // 512 -> 1 sec, 30720 -> 1 min

        _BIS_SR(LPM3_bits + GIE);                // Enter LPM3 w/ interrupt
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
 * Initialize ADC bits.
 */
void initializeADC(struct plantProperty* ptr_plant)
{
    __disable_interrupt();
    P1OUT |= (*ptr_plant).enableADC;
    ADC10CTL1 = (*ptr_plant).selectADC;
    ADC10AE0 |= (*ptr_plant).sampleADC;
    __enable_interrupt();
}

/*
 * De-initialize ADC bits.
 */
void deinitializeADC(struct plantProperty* ptr_plant)
{
    __disable_interrupt();
    ADC10CTL0 &= ~ENC;
    ADC10AE0 &= ~(*ptr_plant).sampleADC;
    P1OUT &= ~(*ptr_plant).enableADC;
    __enable_interrupt();
}

/*
 * Check moisture level.
 */
int checkMoisture()
{
    int moisture = 0;
    ADC10CTL0 |= ENC + ADC10SC;
    _BIS_SR(LPM0_bits + GIE);
    moisture = ADC10MEM;
    return moisture;
}

/*
 * Runs moisture level check and waters plant if needed.
 */
void waterPlant(struct plantProperty* ptr_plant)
{
    int moisture = checkMoisture();
    if(moisture<MOISTURE_MIN)
    {
        P2OUT |= (*ptr_plant).activateSolenoid;
        delay(1);
        while(moisture<MOISTURE_MAX)
        {
            P2OUT |= BIT3;
            moisture = checkMoisture();
        }
        P2OUT &= ~BIT3;
        delay(2);
        P2OUT &= ~(*ptr_plant).activateSolenoid;
    }
}

/*
 * Runs the sequence required to water plants.
 */
void plantState(struct plantProperty* ptr_plant)
{
    initializeADC(ptr_plant);
    waterPlant(ptr_plant);
    deinitializeADC(ptr_plant);
}

void main(void)
{
    disableWatchdog();

    initialize();

    struct plantProperty plant1 = {.enableADC = BIT0,
                                         .selectADC = INCH_3,
                                         .sampleADC = BIT3,
                                         .activateSolenoid = BIT0};
    struct plantProperty *ptr_plant1 = &plant1;

    struct plantProperty plant2 = {.enableADC = BIT1,
                                         .selectADC = INCH_4,
                                         .sampleADC = BIT4,
                                         .activateSolenoid = BIT1};
    struct plantProperty *ptr_plant2 = &plant2;

    struct plantProperty plant3 = {.enableADC = BIT2,
                                         .selectADC = INCH_5,
                                         .sampleADC = BIT5,
                                         .activateSolenoid = BIT2};
    struct plantProperty *ptr_plant3 = &plant3;


    while(1)
    {
        plantState(ptr_plant1);
        plantState(ptr_plant2);
        plantState(ptr_plant3);
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
