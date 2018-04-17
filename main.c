#include <msp430.h>
#include <stdlib.h>
#include "peripherals.h"


unsigned long milli_gallons=0, millis=0;
unsigned int leap_cnt;
bool dispense_gas=false;

void wait_for_response()
{
    char currKey =' ';
    while(currKey!='*')
    {
        currKey = getKey();
    }
}

void swDelay(char numLoops)
{
  volatile unsigned int i,j;

  for (j=0; j<numLoops; j++)
  {
    i = 50000 ;
    while (i > 0)
    i--;
  }
}

//configure the msp buttons
void config_msp_buttons(void)
{
  P2SEL &= ~(BIT1);
  P1SEL &= ~(BIT1);

  P2DIR &= ~(BIT1);
  P1DIR &= ~(BIT1);

  P2REN |= (BIT1);
  P1REN |= (BIT1);

  P2OUT |= (BIT1);
  P1OUT |= (BIT1);
}

//configure the board buttons
void config_board_buttons(void)
{
  P7SEL &= ~(BIT0|BIT4);
  P3SEL &= ~(BIT6);
  P2SEL &= ~(BIT2);

  P7DIR &= ~(BIT0|BIT4);
  P3DIR &= ~(BIT6);
  P2DIR &= ~(BIT2);

  P7REN |= (BIT0|BIT4);
  P3REN |= (BIT6);
  P2REN |= (BIT2);

  P7OUT |= (BIT0|BIT4);
  P3OUT |= (BIT6);
  P2OUT |= (BIT2);
}

//return the state of the buttons
unsigned char buttons_state(void)
{
  unsigned char val=0;
  if(~P7IN&BIT0){
    val|=BIT0; //S1
  }
  if(~P3IN&BIT6){
    val|=BIT1; //S2
  }
  if(~P2IN&BIT2){
    val|=BIT2; //S3
  }
  if(~P7IN&BIT4){
    val|=BIT3; //S4
  }
  return val;
}

//determine and display the cost
void display_cost(unsigned char priceString[], unsigned char grade)
{
  unsigned long price;

  if(grade == 'D')
  {
    price = (milli_gallons * 299)/100;
  }
  else if(grade == 'S')
  {
    price = (milli_gallons * 289)/100;
  }
  else if(grade == 'P')
  {
    price = (milli_gallons * 269)/100;
  }
  else
  {
    price = (milli_gallons * 249)/100;
  }

  convert_price(priceString, price);

  Graphics_clearDisplay(&g_sContext);

  Graphics_drawStringCentered(&g_sContext, "TOTAL:", AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, priceString, AUTO_STRING_LENGTH, 48, 55, TRANSPARENT_TEXT);

  Graphics_flushBuffer(&g_sContext);
}


//convert the price
void convert_price(unsigned char priceString[], unsigned long price)
{
  int digits = 0;
  int num;
  int dot = 1;
  int one = 1;
  int i = 1;
  int j;

  priceString[0] = '$';

  num = price;

  //determine the number of digits in the input number
  while(num >= 10)
  {
    num = num/10;
    digits++;
  }


  for(j=0;j<digits;j++)
  {
    one = one*10;
  }

  while(digits >= 0)
  {
    if((digits == 1)&&(dot == 1))
    {
      priceString[i] = '.';
      dot = 0;      //make sure decimal cannot be added again
      i++;
    }
    else
    {
      num = price/one;
      priceString[i] = '0' + num; //starts at 0x30
      price -= (one*num);
      one /= 10;
      digits--;
      i++;
    }
  }
}


void welcome_screen(void)
{
  Graphics_clearDisplay(&g_sContext);
  Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, "Press * to begin", AUTO_STRING_LENGTH, 48, 45, TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
}


//return grade as D, S, P, or R
//return grade as D, S, P, or R
unsigned char select_grade()
{
    unsigned char grade = ' ';
    Graphics_clearDisplay(&g_sContext); // Clear the display
    Graphics_drawStringCentered(&g_sContext, "Fuel Grades", AUTO_STRING_LENGTH,
                                48, 15, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "D  S  P  R", AUTO_STRING_LENGTH,
                                48, 25, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "1  2  3  4", AUTO_STRING_LENGTH,
                                48, 45, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext); // show LCD changes
    while (grade == ' ')
    {
        if (buttons_state() == BIT0)
        {
            grade = 'D';
        }
        if (buttons_state() == BIT1)
        {
            grade = 'S';
        }
        if (buttons_state() == BIT2)
        {
            grade = 'P';
        }
        if (buttons_state() == BIT3)
        {
            grade = 'R';
        }
    }
    return grade;
}


//start timer
void runtimerA2(void)
{

  // ACLK|16 Bit|up mode|1 divider
  TA2CTL = TASSEL_1 + MC_1 + ID_0;
  TA2CCR0 = 327; // 327+1 ticks is about .01 seconds
  TA2CCTL0 = CCIE; // TA2CCR0 interrupt enabled
}

//Timer Interrupt
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR(void)
{
  if(leap_cnt<65536) //CHECK IF THIS NUMBER IS CORRECT
  {
    millis++;
    leap_cnt++;
  }
  else
  {
    leap_cnt = 0;
  }
}

// This function stops Timer A2 andresets the global gallons variable
void stoptimerA2(int reset)
{

    TA2CTL = MC_0; // stop timer
    TA2CCTL0 &= ~CCIE; // TA2CCR0 interrupt disabled
    if (reset)
    {
        milli_gallons = 0;
        millis = 0;
    }
}

//display milli_gallons
void display_gallons(unsigned int gallons)
{
  unsigned char display[6];

  display[5] = (gallons % 10) + 48;
  display[4] = ((gallons/10) % 10) + 48;
  display[3] = '.';
  display[2] = ((gallons/100) % 10) + 48;
  display[1] = ((gallons/1000) % 10) + 48;
  display[0] = ((gallons/10000) % 10) + 48;

  Graphics_drawStringCentered(&g_sContext, display,6,45,50,OPAQUE_TEXT);
  Graphics_flushBuffer(&g_sContext);
}

//gets values from pump to be ready
//gets values from pump to be ready
void pump_ready(unsigned char grade)
{
    //can call display gallons function
    //probably want to use interrupt with counter to call display gallons
    //need to get time button has been pressed
    //allow for topping off

    stoptimerA2(1);
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "Begin fueling",
    AUTO_STRING_LENGTH,
                                48, 35, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "* to swipe",
    AUTO_STRING_LENGTH,
                                48, 75, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    bool latched = false;
    bool released = false;
    bool finished =false;
    bool topping_off=false;
    bool btn_2_1 = (~P2IN) & BIT1;
    bool btn_1_1 = (~P1IN) & BIT1;
    while (!finished)
    {
        btn_2_1 = (~P2IN) & BIT1;
        btn_1_1 = (~P1IN) & BIT1;
        if (grade == 'D')
        {
            if (btn_2_1)
            {
                latched = true;
                released = false;
                runtimerA2();
            }
            while (latched)
            {
                btn_2_1 = (~P2IN) & BIT1;
                milli_gallons = millis;
                if (milli_gallons % 2 == 0)
                {
                    display_gallons(milli_gallons);
                }
                if (!btn_2_1)
                {
                    released = true;
                }
                if (released && btn_2_1)
                {
                    latched = false;
                    topping_off = true;
                }
            }
            while (topping_off)
            {
                btn_2_1 = (~P2IN) & BIT1;
                if (btn_2_1)
                {
                    runtimerA2();
                    milli_gallons = millis;
                    if (milli_gallons % 2 == 0)
                    {
                        display_gallons(milli_gallons);
                    }
                }
                if (getKey() == '*')
                {
                    topping_off = false;
                    finished = true;
                }
                /* this should be uncommneted to use the timer variant for finishing top off
                 * if(millis>1000){
                 *      topping_off=false;
                 *      finished=true;
                 *  }
                 */
            }
        }
        else
        {
            if (btn_1_1)
            {
                latched = true;
                released = false;
                runtimerA2();
            }
            while (latched)
            {
                btn_1_1 = (~P1IN) & BIT1;
                milli_gallons = millis;
                if (milli_gallons % 2 == 0)
                {
                    display_gallons(milli_gallons);
                }
                if (!btn_1_1)
                {
                    released = true;
                }
                if (released && btn_1_1)
                {
                    latched = false;
                    topping_off = true;
                }
            }
            while (topping_off)
            {
                btn_1_1 = (~P1IN) & BIT1;
                if (btn_1_1)
                {
                    runtimerA2();
                    milli_gallons = millis;
                    if (milli_gallons % 2 == 0)
                    {
                        display_gallons(milli_gallons);
                    }
                }
                if (getKey() == '*')
                {
                    topping_off = false;
                    finished = true;
                }
                /*
                 * if(millis>1000){
                 *      topping_off=false;
                 *      finished=true;
                 *  }
                 */
            }
        }
    }
}


void pay(unsigned char pin[], unsigned char currKey)
{
  unsigned char pin1[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
  unsigned char pin2[8] = {'8', '7', '6', '5', '4', '3', '2', '1'};
  unsigned char displayed_pin[8];
  int i;
  int count = 0;
  unsigned char led_vals = 0xF;
  int correctPin1;
  int correctPin2;
  //gets the pin inputted to display as X's
  for(i=0; i<8; i++){
      displayed_pin[i]=' ';
  }

  Graphics_drawStringCentered(&g_sContext, "ENTER PIN", AUTO_STRING_LENGTH, 48, 65, TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
  stoptimerA2(1);
  while(1)
  {
    milli_gallons = 0;
    runtimerA2();
    correctPin1 = 1;
    correctPin2 = 1;
    i = 0;

    while(i<8)
    {
      if((millis>100)&&(count<2))
      {
        if((millis%4)==0)
        {
          led_vals ^=0x0F;
          BuzzerOn();
          setLeds(led_vals);
        }
      }
      currKey = getKey();
      if((currKey >= '0') && (currKey <= '9'))
      {
        pin[i] = currKey;
        displayed_pin[i]='X';
        i++;
        while(getKey() != 0){}
        currKey = 0;
        milli_gallons = 0;
      }
      Graphics_drawStringCentered(&g_sContext, displayed_pin, 8, 48, 75, TRANSPARENT_TEXT);
      Graphics_flushBuffer(&g_sContext);
    }
    for(i=0;i<8;i++)
    {
      if(pin[i] != pin1[i])
      {
        correctPin1 = 0;
      }

      if(pin[i] != pin2[i])
      {
        correctPin2 = 0;
      }
    }

    if(correctPin1|correctPin2)
    {
      BuzzerOff();
      setLeds(0);
      //jumps out of the while loop
      break;
    }
    else
    {
        for (i = 0; i < 8; i++)
        {
            displayed_pin[i] = ' ';
        }
      Graphics_drawStringCentered(&g_sContext, "INCORRECT", AUTO_STRING_LENGTH, 48, 85, OPAQUE_TEXT);
      Graphics_drawStringCentered(&g_sContext, displayed_pin, 8, 48, 75, OPAQUE_TEXT);
      // Graphics_drawStringCentered(&g_sContext, "ENTER AGAIN", AUTO_STRING_LENGTH, 48, 75, OPAQUE_TEXT);
      Graphics_flushBuffer(&g_sContext);
      count++;
    }
  }
}



void main(void)
{
  unsigned char pin[8];
  unsigned char priceString[10];
  unsigned char currKey=0;
  unsigned char grade=0;
  int i;
  WDTCTL = WDTPW | WDTHOLD;

  configDisplay();
  initLeds();
  configKeypad();
  config_board_buttons();
  config_msp_buttons();

  _BIS_SR(GIE);


  while(1)
  {
    //resets the globals
    for(i=0; i<8; i++){
        pin[i]=NULL;
    }
    for(i=0; i<10; i++){
        priceString[i]=NULL;
    }
    grade=0;
    currKey=0;
    milli_gallons=0;

    //show the welcome
    welcome_screen();

    wait_for_response();

    grade = select_grade();
    pump_ready(grade);

    for(i=0;i<10;i++)
    {
      priceString[i] = NULL;  //initialize to be null
    }

    display_cost(priceString, grade);
    pay(pin, currKey);
    Graphics_clearDisplay(&g_sContext);
  }
}

/*int main(){
    WDTCTL = WDTPW | WDTHOLD;
    initLeds();
    setLeds(0x01);
    configDisplay();
    while (1)
    {
        Graphics_drawStringCentered(&g_sContext, "INCORRECT",
                                    AUTO_STRING_LENGTH, 48, 65, OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext, "ENTER AGAIN",
                                    AUTO_STRING_LENGTH, 48, 75, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }

    return 0;
}*/
