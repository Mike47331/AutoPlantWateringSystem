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
 * 2019-01-26: Changed timer module to get rid of 32kHz crystal.
 * 2019-02-23: Switched pinouts to make PCB routing easier.
 */


#include <msp430.h>
#include <stdint.h>

#define MOISTURE_MIN 200
#define MOISTURE_MAX 600
#define MAX_TRAVEL 5
#define SOAK_TIME 3

struct plantProperty
{
    int enableADC;
    int selectADC;
    int sampleADC;
    int activateSolenoid;
    int travelTime;
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

    BCSCTL2 = DIVM_3 + DIVS_3;                  // Set MCLK and SMCLK to /8

    P1DIR |= 0x15;                              // Set P1.0, P1.2, P1.4 as output
    P1OUT &= ~(BIT0 + BIT2 + BIT4);             // Set all output pins to low

    P2DIR |= 0x0F;                              // Set P2.0, P2.1, P2.2, P2.3 as output
    P2OUT &= ~(BIT0 + BIT1 + BIT2 +BIT3);       // Set all output pins to low

    ADC10CTL1 |= ADC10DIV_7;
    ADC10CTL0 = ADC10SHT_1 + ADC10ON + ADC10IE; // Turn on ADC, enable interrupt and sample for 8x ADC10CLKs

    __enable_interrupt();                       // Enable global interrupts
}

/*
 * Milisecond delay using Timer A.
 */
void msdelay(int mseconds)
{
    int i;
    for(i = mseconds; i>0; i--)
    {
        TACTL = TASSEL_2 + ID_3 + MC_1;          // SMCLK, /8, upmode
        CCTL0 = CCIE;
        CCR0 = 16-1;

        _BIS_SR(LPM1_bits + GIE);
    }
}

/*
 * Second delay using TimerA.
 */
void delay(int seconds)
{
    int i;
    for(i = seconds; i>0; i--)
    {
        TACTL = TASSEL_2 + ID_3 + MC_1;          // SMCLK, /8, upmode
        CCTL0 = CCIE;
        CCR0 = 15625-1;

        _BIS_SR(LPM1_bits + GIE);
    }
}

/*
 * Hour delay using nested second delay.
 */
void hdelay(int hours)
{
    int i;
    for(i = hours; i>0; i--)
    {
        delay(3600);
    }
}

/*
 * Initialize ADC bits.
 */
void initializeADC(struct plantProperty* ptr_plant)
{
    __disable_interrupt();
    P1OUT |= (*ptr_plant).enableADC;
    ADC10CTL1 |= (*ptr_plant).selectADC;
    ADC10AE0 |= (*ptr_plant).sampleADC;
    __enable_interrupt();
    msdelay(500);
}

/*
 * De-initialize ADC bits.
 */
void deinitializeADC(struct plantProperty* ptr_plant)
{
    __disable_interrupt();
    ADC10CTL0 &= ~ENC;
    ADC10CTL1 &= ~(*ptr_plant).selectADC;
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
 * Pre-waters the plant and verifies configuration.
 */
void preWaterPlant(struct plantProperty* ptr_plant)
{
    int moisture = checkMoisture();
    int counter = 0;
    P2OUT |= (*ptr_plant).activateSolenoid;
    delay(1);
    while(moisture<MOISTURE_MAX || counter<MAX_TRAVEL)
    {
        P2OUT |= BIT3;
        delay(1);
        moisture = checkMoisture();
        counter++;
    }
    if(counter>=MAX_TRAVEL)
    {
        while(1)
        {
            P1OUT |= (BIT0 + BIT2 + BIT4);
            delay(1);
            P1OUT &= ~(BIT0 + BIT2 + BIT4);
            delay(1);
        }
    }
    P2OUT &= ~BIT3;
    delay(2);
    P2OUT &= ~(*ptr_plant).activateSolenoid;
    (*ptr_plant).travelTime = counter;
}

/*
 * Deep waters the plant.
 */
void waterPlant(struct plantProperty* ptr_plant)
{
    int waterTime = (*ptr_plant).travelTime + SOAK_TIME;
    P2OUT |= (*ptr_plant).activateSolenoid;
    delay(1);
    P2OUT |= BIT3;
    delay(waterTime);
    P2OUT &= ~BIT3;
    delay(2);
    P2OUT &= ~(*ptr_plant).activateSolenoid;
}

/*
 * Runs the sequence required to water plants.
 */
void plantState(struct plantProperty* ptr_plant)
{
    initializeADC(ptr_plant);
    int moisture = checkMoisture();
    if(moisture<MOISTURE_MIN)
    {
        preWaterPlant(ptr_plant);
        delay(60);
        waterPlant(ptr_plant);
    }
    waterPlant(ptr_plant);
    deinitializeADC(ptr_plant);
}

void main(void)
{
    disableWatchdog();

    initialize();

    struct plantProperty plant1 = {.enableADC = BIT0,
                                   .selectADC = INCH_1,
                                   .sampleADC = BIT1,
                                   .activateSolenoid = BIT0,
                                   .travelTime = 0};
    struct plantProperty *ptr_plant1 = &plant1;

    struct plantProperty plant2 = {.enableADC = BIT2,
                                   .selectADC = INCH_3,
                                   .sampleADC = BIT3,
                                   .activateSolenoid = BIT1,
                                   .travelTime = 0};
    struct plantProperty *ptr_plant2 = &plant2;

    struct plantProperty plant3 = {.enableADC = BIT4,
                                   .selectADC = INCH_5,
                                   .sampleADC = BIT5,
                                   .activateSolenoid = BIT2,
                                   .travelTime = 0};
    struct plantProperty *ptr_plant3 = &plant3;


    while(1)
    {
        plantState(ptr_plant1);
        plantState(ptr_plant2);
        plantState(ptr_plant3);
        hdelay(48);
    }
}

/*
 * Timer interrupt from second delay.
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer_A(void)
{
    _BIC_SR_IRQ(LPM1_bits);
}

/*
 * ADC interrupt when ADC has finished sampling and converting.
 */
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    _BIC_SR_IRQ(LPM0_bits);
}
