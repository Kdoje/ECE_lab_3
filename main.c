#include <msp430.h>
#include <stdlib.h>
#include "peripherals.h"

unsigned int utc_secs=0;
unsigned int temp=0;
unsigned int temp_buffer[10]={0};
unsigned int scroll=0;
void config_launch();
void timer_tick();
void adc_interrupt();
void acd_config();
void stop_timer();
void disp_temp();
void run_timer();

void main(void)
{
  WDTCTL = WDTPW | WDTHOLD;
  configDisplay();
  initLeds();
  configKeypad();
  config_launch();
  run_timer();
  _BIS_SR(GIE);
  while(1){

  }
}


void config_launch(){
    P2SEL &= ~(BIT1);
    P1SEL &= ~(BIT1);

    P2DIR &= ~(BIT1);
    P1DIR &= ~(BIT1);

    P2REN |= BIT1;
    P1REN |= BIT1;

    P2OUT |= BIT1;
    P1OUT |= BIT1;
}
void run_timer(void)
{
    //uses ACLk with a 32767 count to interrupt every second
     TA2CTL = TASSEL_1 + MC_1 + ID_0;
     TA2CCR0 = 32767;
     TA2CCTL0 = CCIE;
}

void stop_timer(int reset)
{
    TA2CTL = MC_0;
    TA2CCTL0 &= ~CCIE;
    if (reset)
        utc_secs = 0;
}

 void adc_config(void)
 {
     REFCTL0 &= ~REFMSTR;

     //383 clk cycles sample and hold time, turn on reference (default to 1.5V)
     //turn on ADC, multi sample conversion
     ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON | ADC12MSC;
     ADC12CTL1 = ADC12SHP | ADC12CSTARTADD_1 | ADC12CONSEQ_1;

     //2 memory channels 1 for temperature using input channel 10
     //0 for scroll wheel using input channel 0, end conversion sequence
     ADC12MCTL1 = ADC12SREF_1 + ADC12INCH_10;
     ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0 | ADC12EOS;

     //enable interrupts
     P6SEL |= BIT0;
     ADC12IE = BIT1;
     _BIS_SR(GIE);

     ADC12CTL0 |= ADC12ENC;
 }

#pragma vector=ADC12_VECTOR
__interrupt void adc_interrupt (void)
{
    //grabs the updated values
    temp = ADC12MEM1 & 0x0FFF;
    scroll = ADC12MEM0 & 0x0FFF;
}

// Timer A2 interrupt service routine
#pragma vector=TIMER2_A0_VECTOR
__interrupt void timer_tick (void)
{
    utc_secs++;
    disp_temp();
}

void disp_temp(){
    //TODO add calibration stuff here
    //get calibration values for the adc voltage (1.5V)
    unsigned int CALADC12_15V_30C = *((unsigned int *) 0x1A1A);
    unsigned int CALADC12_15V_85C = *((unsigned int *) 0x1A1C);

    //calculate temperature from adc code value considering calibration values
    //convert from celsius to fahrenheit
    float bit;
    float denom = (float) (CALADC12_15V_85C) - (float) (CALADC12_15V_30C);
    bit = ((float) (85.0 - 30.0)) / denom;
    volatile float inter1 = ((float) temp) - CALADC12_15V_30C;
    volatile float inter2 = inter1 * bit;
    volatile float tempC = inter2 + 30.0;
    volatile float tempF = tempC * 9.0 / 5.0 + 32.0;

    temp_buffer[utc_secs%10]=temp;
    if(utc_secs<10){
        final_temp=temp;
    }
    else{
        int avg=0;
        int i;
        for(i=0;i<10;i++){
            final_temp+=temp_buffer[i];
        }
        final_temp=avg/10;
    }
    char final_temp_disp[3];
    if(final_temp>100){
    final_temp_disp[0]=final_temp/100+0x30;
    }
    final_temp_disp[1]=final_temp/10+0x30;
    if(final_temp>10){
    final_temp_disp[2]=final_temp%10+0x30;
    }
    Graphics_drawStringCentered(&g_sContext, final_temp_disp, 3,
                                48, 45, OPAQUE_TEXT);
    Graphics_flushBuffer(&g_sContext);
}
