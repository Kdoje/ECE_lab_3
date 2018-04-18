#include <msp430.h>
#include <stdlib.h>
#include "peripherals.h"
const unsigned int days_jan = 31;
const unsigned int days_feb = 31 + 28;
const unsigned int days_mar = 31 + 28 + 31;
const unsigned int days_apr = 31 + 28 + 31 + 30;
const unsigned int days_may = 31 + 28 + 31 + 30 + 31;
const unsigned int days_jun = 31 + 28 + 31 + 30 + 31 + 30;
const unsigned int days_jul = 31 + 28 + 31 + 30 + 31 + 30 + 31;
const unsigned int days_aug = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31;
const unsigned int days_sep = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30;
const unsigned int days_oct = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31;
const unsigned int days_nov = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31
        + 30;
const unsigned int days_to_sec = 24 * 60 * 60;
const unsigned int hours_to_sec = 60 * 60;
const unsigned int mins_to_sec = 60;
unsigned int utc_secs=0;
unsigned char edit_state=0;
bool done_editing=false;
unsigned int month, days, hours, minutes, seconds;
unsigned int temp=0;
unsigned int temp_buffer_f[60]={0};
unsigned int temp_buffer_c[60]={0};
unsigned int scroll=0;
void config_launch();
void timer_tick();
void adc_interrupt();
void adc_config();
void stop_timer();
void disp_temp();
void disp_time();
void run_timer();
void edit_mode();
void swDelay(char numLoops);

void main(void)
{
  WDTCTL = WDTPW | WDTHOLD;
  configDisplay();
  initLeds();
  adc_config();
  configKeypad();
  config_launch();
  run_timer();
  while(1){
      if(!(P2IN&BIT1)){
          edit_mode();
      }
  }
}

void edit_mode(){
    volatile char *new_date="JAN:00";
    volatile char *new_time="00:00:00";
    volatile char *temp_month="JAN";
    volatile char *temp_day="00";
    while(!done_editing){
        ADC12CTL0 |= ADC12ENC + ADC12SC;
        switch (edit_state)
        {
        case 0:
        {
            if (scroll < 341){
                temp_month = "JAN";
            }
            else if (scroll < 683){
                temp_month = "FEB";
            }
            else if (scroll < 1024){
                temp_month = "MAR";
            }
            else if (scroll < 1365){
                temp_month = "APR";
            }
            else if (scroll < 1707){
                temp_month = "MAY";
            }
            else if (scroll < 2048){
                temp_month = "JUN";
            }
            else if (scroll < 2389){
                temp_month = "JUL";
            }
            else if (scroll < 2731){
                temp_month = "AUG";
            }
            else if (scroll < 3072){
                temp_month = "SEP";
            }
            else if (scroll < 3413){
                temp_month = "OCT";
            }
            else if (scroll < 3755){
                temp_month = "NOV";
            }
            else if (scroll < 4096){
                temp_month = "DEC";
            }
            int i;
            for (i = 0; i < 3; i++)
            {
                new_date[i] = temp_month[i];
            }
            Graphics_drawStringCentered(&g_sContext, temp_month,
                                        6, 28, 30,
                                        OPAQUE_TEXT);
            Graphics_drawLine(&g_sContext, 20, 34, 34, 34);
            Graphics_flushBuffer(&g_sContext);
            break;
        }
        case 1:
        {
            break;
        }
        case 2:{
            break;
        }
        case 3:{
            break;
        }
        case 4:{

            break;
        }
        }
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
    ADC12CTL0 |= ADC12ENC + ADC12SC;
    disp_temp();
    disp_time();
}
void print_month_day(char month[3], char day[2], unsigned int days){
    char *month_store="JAN";

    if(days<days_jan){
        month_store="JAN";
    }
    else if(days<(days_feb)){
        days-=days_jan;
        month_store="FEB";
    }
    else if(days<(days_mar)){
        days-=days_feb;
        month_store="MAR";
    }
    else if(days<(days_apr)){
        days-=days_mar;
        month_store="APR";
    }
    else if(days<(days_may)){
        days-=days_apr;
        month_store="MAY";
    }
    else if(days<(days_jun)){
        days-=days_may;
        month_store="JUN";
    }
    else if(days<(days_jul)){
        days-=days_jun;
        month_store="JUL";
    }
    else if(days<(days_aug)){
        days-=days_jul;
        month_store="AUG";
    }
    else if(days<(days_sep)){
        days-=days_aug;
        month_store="SEP";
    }
    else if(days<(days_oct)){
        days-=days_sep;
        month_store = "OCT";
    }
    else if (days < (days_nov))
    {
        days -= days_oct;
        month_store = "NOV";
    }
    else
    {
        days-=days_nov;
        month_store = "DEC";
    }
    int i;
    for(i=0; i<3; i++){
        month[i]=month_store[i];
    }
    day[0]=(days/10)+0x30;
    day[1]=(days%10)+0x30;
}
void disp_time(){
    //month, days, hours, mins, secs;
    unsigned int cur_secs = utc_secs;
    char month[3], day[2];
    char months_and_days[6];
    char clock_time[8];
    days = cur_secs / days_to_sec;
    cur_secs -= (cur_secs / days_to_sec) * days_to_sec;

    hours = cur_secs / hours_to_sec;
    cur_secs -= (cur_secs / hours_to_sec) * hours_to_sec;

    minutes = cur_secs / mins_to_sec;
    cur_secs -= (cur_secs / mins_to_sec) * mins_to_sec;

    seconds = cur_secs;
    print_month_day(month, day, days);
    months_and_days[0]=month[0];
    months_and_days[1]=month[1];
    months_and_days[2]=month[2];
    months_and_days[3]=':';
    months_and_days[4]=day[0];
    months_and_days[5]=day[1];

    clock_time[0]=(hours/10)+0x30;
    clock_time[1]=(hours%10)+0x30;
    clock_time[2]=':';
    clock_time[3]=(minutes/10)+0x30;
    clock_time[4]=(minutes%10)+0x30;
    clock_time[5]=':';
    clock_time[6]=(seconds/10)+0x30;
    clock_time[7]=(seconds%10)+0x30;
    Graphics_drawStringCentered(&g_sContext, months_and_days, 6, 40, 15,
    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, clock_time, 8, 40, 25,
    OPAQUE_TEXT);
    Graphics_flushBuffer(&g_sContext);
}
void print_temp(char final_temp_disp[6], volatile int final_temp, char unit)
{
    final_temp_disp[0] = final_temp / (100 * 10) + 0x30;
    final_temp -= (final_temp_disp[0] - 0x30) * (100 * 10);
    if(final_temp_disp[0]=='0'){
        final_temp_disp[0]=' ';
    }
    final_temp_disp[1] = final_temp / (10 * 10) + 0x30;
    final_temp -= (final_temp_disp[1] - 0x30) * (10 * 10);
    if(final_temp_disp[0]==' '&&final_temp_disp[1]=='0'){
        final_temp_disp[1]=' ';
    }
    final_temp_disp[2] = final_temp / (1 * 10) + 0x30;
    final_temp -= (final_temp_disp[2] - 0x30) * (1 * 10);
    final_temp_disp[3] = '.';
    final_temp_disp[4] = final_temp + 0x30;
    final_temp_disp[5] = unit;
}

void disp_temp(){
    //TODO add calibration stuff here
    //get calibration values for the adc voltage (1.5V)
    unsigned int CALADC12_15V_30C = *((unsigned int *) 0x1A1A);
    unsigned int CALADC12_15V_85C = *((unsigned int *) 0x1A1C);

    //calculate temperature from adc code value considering calibration values
    //convert from celsius to fahrenheit
    float denom = (float) (CALADC12_15V_85C) - (float) (CALADC12_15V_30C);
    float conversion = ((float) (85.0 - 30.0)) / denom;
    volatile float voltage_adjust = ((float) temp) - CALADC12_15V_30C;
    volatile float temp_c = (voltage_adjust * conversion) + 30;
    volatile float temp_f = temp_c * 9.0 / 5.0 + 32.0;
    //this is where the temperature in 1/10ths of degrees is stored
    volatile long final_temp_c = 0;
    volatile long final_temp_f = 0;

    char final_temp_disp_c[6];
    char final_temp_disp_f[6];

    temp_buffer_f[utc_secs % 10] = temp_f*10;
    temp_buffer_c[utc_secs % 10] = temp_c*10;

    if (utc_secs < 60)
    {
        final_temp_c = temp_c * 10; //gets the temp from the adc_interrupt stuff
        final_temp_f = temp_f * 10;
    }

    else
    {
        long avg_c=0, avg_f = 0;
        int i;
        for (i = 0; i < 10; i++)
        {
            avg_c += temp_buffer_c[i];
            avg_f += temp_buffer_f[i];
        }
        final_temp_f = avg_f / 10;
        final_temp_c = avg_c / 10;
    }
    print_temp(final_temp_disp_c, final_temp_c, 'C');
    print_temp(final_temp_disp_f, final_temp_f, 'F');
   // final_temp_disp_f="101.5C"
    Graphics_drawStringCentered(&g_sContext, final_temp_disp_c, 6, 20, 45,
    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, final_temp_disp_f, 6, 20, 55,
    OPAQUE_TEXT);
    Graphics_flushBuffer(&g_sContext);
}
void swDelay(char numLoops)
{
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2013

    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j=0; j<numLoops; j++)
    {
        i = 50000 ;                 // SW Delay
        while (i > 0)               // could also have used while (i)
           i--;
    }
}
