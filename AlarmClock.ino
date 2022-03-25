/* ------------------------------ */
/* Alarm Clock by Daniel Kiwilsza */
/* ------------------------------ */

/* TODO
    - add code for speaker
*/


//segment pins
#define segmentA 12
#define segmentB 11
#define segmentC 10
#define segmentD 9
#define segmentE 8
#define segmentF 7
#define segmentG 6
#define segmentDP 5

//digit pins
#define digit1 4
#define digit2 16
#define digit3 2
#define digit4 14
#define digitL1L2L3 15

//button pins
#define button1 17
#define button2 18
#define button3 19
#define button4 20
#define button5 21

//display function input arguments
#define colon 10
#define colon_alarm 11
#define off 12
#define display_alarm 13

//mode select
unsigned char mode_select = 0;
#define normalMode 0
#define editClockMinutesMode 1
#define editClockHoursMode 2
#define editAlarmMinutesMode 3
#define editAlarmHoursMode 4

//for debugging, 0: display hours and minutes, 1: display minutes and seconds
unsigned char display_selector = 0;

//set default button states
volatile unsigned char buttonState1 = 0;
volatile unsigned char buttonState2 = 0;
volatile unsigned char buttonState3 = 0;
volatile unsigned char buttonState4 = 0;
volatile unsigned char buttonState5 = 0;

unsigned char counter = 0;              //incremented at 4 Hz
unsigned char alarmActive = false;
unsigned char changedClock = false;
unsigned char toggleAlarm = false;
unsigned char snoozeActive = false;

unsigned char timer_seconds_01 = 0;     //keeps track of 1's place of second counter
unsigned char timer_seconds_10 = 0;     //keeps track of 10's place of second counter
unsigned char timer_minutes_01 = 0;     //keeps track of 1's place of minute counter
unsigned char timer_minutes_10 = 3;     //keeps track of 10's place of minute counter
unsigned char timer_hours_01 = 2;       //keeps track of 1's place of hour counter
unsigned char timer_hours_10 = 1;       //keeps track of 10's place of hour counter

unsigned char alarm_minutes_01 = 1;     //specified 1's place of minute counter for alarm
unsigned char alarm_minutes_10 = 3;     //specified 10's place of minute counter for alarm
unsigned char alarm_hours_01 = 2;       //specified 1's place of hour counter for alarm
unsigned char alarm_hours_10 = 1;       //specified 10's place of hour counter for alarm

unsigned char snooze_seconds = 0;       //keeps track of how long the clock has been snoozed for

//used for time edit mode
unsigned char frozen_seconds_01 = 0;    
unsigned char frozen_seconds_10 = 0; 
unsigned char frozen_minutes_01 = 0; 
unsigned char frozen_minutes_10 = 0; 
unsigned char frozen_hours_01 = 0;      
unsigned char frozen_hours_10 = 0;      

//choose what is displayed on 7-segment
unsigned char display_matrix[][8] = {
    {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW, LOW},   // 0
    {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW},       // 1
    {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH, LOW},    // 2
    {HIGH, HIGH, HIGH, HIGH, LOW, LOW, HIGH, LOW},    // 3
    {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH, LOW},     // 4
    {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH, LOW},    // 5
    {HIGH, LOW, HIGH, HIGH, HIGH, HIGH, HIGH, LOW},   // 6
    {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW},      // 7
    {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW},  // 8
    {HIGH, HIGH, HIGH, LOW, LOW, HIGH, HIGH, LOW},    // 9

    {HIGH, HIGH, LOW, LOW, LOW, LOW, LOW, LOW},       // colon
    {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW, LOW},      // colon with alarm
    {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW},         // turn all segments off
    {LOW, LOW, HIGH, LOW, LOW, LOW, LOW, LOW}         // alarm on
};


void setup()
{
  //display connections
  pinMode(segmentA, OUTPUT);  
  pinMode(segmentB, OUTPUT);  
  pinMode(segmentC, OUTPUT);  
  pinMode(segmentD, OUTPUT);   
  pinMode(segmentE, OUTPUT);  
  pinMode(segmentF, OUTPUT);  
  pinMode(segmentG, OUTPUT);  
  pinMode(segmentDP, OUTPUT);    

  //NMOS connections
  pinMode(digit1, OUTPUT);   
  pinMode(digit2, OUTPUT);   
  pinMode(digit3, OUTPUT);   
  pinMode(digit4, OUTPUT);  
  pinMode(digitL1L2L3, OUTPUT);  

  //button connections
  pinMode(button1, INPUT_PULLUP);  
  pinMode(button2, INPUT_PULLUP);   
  pinMode(button3, INPUT_PULLUP);   
  pinMode(button4, INPUT_PULLUP);   
  pinMode(button5, INPUT_PULLUP); 

  Serial.begin(9600);

  //interrupt setup
  unsigned int per_value;
  cli();                    //stop interrupts

  //62425 (F3D9) -- ahead by 0.2 after 59 mins
  //62429 (F3DD)
  //62499 (F423)
    
  //use default 64 prescale factor

  per_value = 0xF3DD;                         //value required for ~4 Hz (62499) F423
  TCA0.SINGLE.PER = per_value;                //set period register
  TCA0.SINGLE.CMP1 = per_value;               //set compare channel match value
  TCA0.SINGLE.INTCTRL |= bit(5);              //enable channel 1 compare match interrupt

  sei(); //allow interrupts
  
}


//--------------------------------------------------------------------------


//display segments function
void display(unsigned char input_arg)
{
  digitalWrite(segmentA, display_matrix[input_arg][0]);   
  digitalWrite(segmentB, display_matrix[input_arg][1]);   
  digitalWrite(segmentC, display_matrix[input_arg][2]);   
  digitalWrite(segmentD, display_matrix[input_arg][3]);    
  digitalWrite(segmentE, display_matrix[input_arg][4]);    
  digitalWrite(segmentF, display_matrix[input_arg][5]);   
  digitalWrite(segmentG, display_matrix[input_arg][6]); 
  digitalWrite(segmentDP, display_matrix[input_arg][7]);

  delay(1);

}


//display segments (with alarm toggled) function
void displayAlarmToggled(unsigned char input_arg)
{
  digitalWrite(segmentA, display_matrix[input_arg][0]);   
  digitalWrite(segmentB, display_matrix[input_arg][1]); 
  digitalWrite(segmentC, display_matrix[input_arg][2]);   
  digitalWrite(segmentD, display_matrix[input_arg][3]);    
  digitalWrite(segmentE, display_matrix[input_arg][4]);    
  digitalWrite(segmentF, display_matrix[input_arg][5]);   
  digitalWrite(segmentG, display_matrix[input_arg][6]);  

  if (!toggleAlarm)
  {
    digitalWrite(segmentDP, display_matrix[input_arg][7]);
  }
  else
  {
    digitalWrite(segmentDP, HIGH);
  }
  
  delay(1);
}


//display colon function
void displayColon()
{
  digitalWrite(segmentA, display_matrix[colon][0]);   
  digitalWrite(segmentB, display_matrix[colon][1]); 

  if (!alarmActive)
  {
    digitalWrite(segmentC, display_matrix[colon][2]);   
  }
  else
  {
    digitalWrite(segmentC, display_matrix[colon_alarm][2]);   
  }

  digitalWrite(segmentD, display_matrix[colon][3]);    
  digitalWrite(segmentE, display_matrix[colon][4]);    
  digitalWrite(segmentF, display_matrix[colon][5]);   
  digitalWrite(segmentG, display_matrix[colon][6]);   
  digitalWrite(segmentDP, display_matrix[colon][7]);
  delay(1);
}


//--------------------------------------------------------------------------


//turn on first digit
void digit_1()
{
  display(off);

  digitalWrite(digit1, HIGH);    
  digitalWrite(digit2, LOW);     
  digitalWrite(digit3, LOW);     
  digitalWrite(digit4, LOW);    
  digitalWrite(digitL1L2L3, LOW);    
}


//turn on second digit
void digit_2()
{
  display(off);
  
  digitalWrite(digit1, LOW);     
  digitalWrite(digit2, HIGH);    
  digitalWrite(digit3, LOW);     
  digitalWrite(digit4, LOW);    
  digitalWrite(digitL1L2L3, LOW);    
}


//turn on third digit
void digit_3()
{
  display(off);

  digitalWrite(digit1, LOW);     
  digitalWrite(digit2, LOW);     
  digitalWrite(digit3, HIGH);    
  digitalWrite(digit4, LOW);    
  digitalWrite(digitL1L2L3, LOW);    
}


//turn on fourth digit
void digit_4()
{
  display(off);

  digitalWrite(digit1, LOW);     
  digitalWrite(digit2, LOW);     
  digitalWrite(digit3, LOW);     
  digitalWrite(digit4, HIGH);   
  digitalWrite(digitL1L2L3, LOW);    
}


//turn on L1/L2/L3 digit
void digit_L1L2L3()
{
  display(off);

  digitalWrite(digit1, LOW);     
  digitalWrite(digit2, LOW);     
  digitalWrite(digit3, LOW);     
  digitalWrite(digit4, LOW);    
  digitalWrite(digitL1L2L3, HIGH);   
}


//--------------------------------------------------------------------------


//for debugging, print the time to the serial monitor
void print_time()
{
  Serial.print(timer_hours_10);
  Serial.print(timer_hours_01);
  Serial.print(":");
  Serial.print(timer_minutes_10);
  Serial.print(timer_minutes_01);
  Serial.print(":");
  Serial.print(timer_seconds_10);
  Serial.print(timer_seconds_01);
  Serial.println();
}


void loop() 
{ 

  if (mode_select == editClockMinutesMode)
  {
    digit_1();
    display(frozen_hours_10);
    
    digit_2();
    display(frozen_hours_01);

    if (counter == 0 || counter == 1)
    {
      digit_3();
      display(frozen_minutes_10);

      digit_4();
      displayAlarmToggled(frozen_minutes_01);

    }
    else
    {
      digit_3();
      display(off);

      digit_4();
      displayAlarmToggled(off);
    }

    digit_L1L2L3();
    displayColon();


    //check if button3 has been pressed (time edit mode)
    if (buttonState3 == 0 && digitalRead(button3) == LOW)
    {
      buttonState3 = 1;
      mode_select = editClockHoursMode;
    }

    if (buttonState3 == 1 && digitalRead(button3) == HIGH)
    {
      buttonState3 = 0;
    }


    //check if button5 has been pressed (increment)
    if (buttonState5 == 0 && digitalRead(button5) == LOW)
    {
      changedClock = 1;

      buttonState5 = 1;
      frozen_minutes_01++;
     
      if (frozen_minutes_01 > 9)
      {
        frozen_minutes_10++;
        frozen_minutes_01 = 0;
      }
  
      if (frozen_minutes_10 > 5)
      {
        frozen_minutes_10 = 0;
      }

    }

    if (buttonState5 == 1 && digitalRead(button5) == HIGH)
    {
      buttonState5 = 0;
    }


  }
  else if (mode_select == editClockHoursMode)
  {
    if (counter == 0 || counter == 1)
    {
      digit_1();
      display(frozen_hours_10);

      digit_2();
      display(frozen_hours_01);

    }
    else
    {
      digit_1();
      display(off);
      delay(1);

      digit_2();
      display(off);
      delay(1);
    }

    digit_3();
    display(frozen_minutes_10);

    digit_4();
    displayAlarmToggled(frozen_minutes_01);

    digit_L1L2L3();
    displayColon();



    //check if button3 has been pressed (time edit mode)
    if (buttonState3 == 0 && digitalRead(button3) == LOW)
    {
      buttonState3 = 1;
      //editClockHours = !editClockHours;
      mode_select = normalMode;

      if (changedClock)
      {
        timer_seconds_01 = 0;
        timer_seconds_10 = 0;
        timer_minutes_01 = frozen_minutes_01;
        timer_minutes_10 = frozen_minutes_10;
        timer_hours_01 = frozen_hours_01;
        timer_hours_10 = frozen_hours_10;
      }
    }

    if (buttonState3 == 1 && digitalRead(button3) == HIGH)
    {
      buttonState3 = 0;
    }

    //check if button5 has been pressed (increment)
    if (buttonState5 == 0 && digitalRead(button5) == LOW)
    {
      changedClock = 1;

      buttonState5 = 1;
      frozen_hours_01++;

      if (frozen_hours_10 == 2)
      {
        if (frozen_hours_01 > 3)
        {
          frozen_hours_01 = 0;
          frozen_hours_10 = 0;
        }
      }
      else
      {
        if (frozen_hours_01 > 9)
        {
          frozen_hours_10++;
          frozen_hours_01 = 0;
        }
      }
    }

    if (buttonState5 == 1 && digitalRead(button5) == HIGH)
    {
      buttonState5 = 0;
    }


  }
  else if(mode_select == editAlarmMinutesMode)
  {
    digit_1();
    display(alarm_hours_10);
    
    digit_2();
    display(alarm_hours_01);

    if (counter == 0 || counter == 1)
    {
      digit_3();
      display(alarm_minutes_10);

      digit_4();
      displayAlarmToggled(alarm_minutes_01);

    }
    else
    {
      digit_3();
      display(off);;

      digit_4();
      displayAlarmToggled(off);
    }

    digit_L1L2L3();
    displayColon();


    //check if button4 has been pressed (alarm edit mode)
    if (buttonState4 == 0 && digitalRead(button4) == LOW)
    {
      buttonState4 = 1;
      mode_select = editAlarmHoursMode;
    }

    if (buttonState4 == 1 && digitalRead(button4) == HIGH)
    {
      buttonState4 = 0;
    }


    //check if button5 has been pressed (increment)
    if (buttonState5 == 0 && digitalRead(button5) == LOW)
    {
      buttonState5 = 1;
      alarm_minutes_01++;
     
      if (alarm_minutes_01 > 9)
      {
        alarm_minutes_10++;
        alarm_minutes_01 = 0;
      }
  
      if (alarm_minutes_10 > 5)
      {
        alarm_minutes_10 = 0;
      }
    }

    if (buttonState5 == 1 && digitalRead(button5) == HIGH)
    {
      buttonState5 = 0;
    }



  }
  else if (mode_select == editAlarmHoursMode)
  {
    if (counter == 0 || counter == 1)
    {
      digit_1();
      display(alarm_hours_10);
      
      digit_2();
      display(alarm_hours_01);

    }
    else
    {
      digit_1();
      display(off);
      delay(1);

      digit_2();
      display(off);
      delay(1);
    }

    digit_3();
    display(alarm_minutes_10);

    digit_4();
    displayAlarmToggled(alarm_minutes_01);

    digit_L1L2L3();
    displayColon();


    //check if button4 has been pressed (alarm edit mode)
    if (buttonState4 == 0 && digitalRead(button4) == LOW)
    {
      buttonState4 = 1;
      //editAlarmHours = !editAlarmHours;
      mode_select = normalMode;
    }

    if (buttonState4 == 1 && digitalRead(button4) == HIGH)
    {
      buttonState4 = 0;
    }

    //check if button5 has been pressed (increment)
    if (buttonState5 == 0 && digitalRead(button5) == LOW)
    {
      buttonState5 = 1;
      alarm_hours_01++;

      if (alarm_hours_10 == 2)
      {
        if (alarm_hours_01 > 3)
        {
          alarm_hours_01 = 0;
          alarm_hours_10 = 0;
        }
      }
      else
      {
        if (alarm_hours_01 > 9)
        {
          alarm_hours_10++;
          alarm_hours_01 = 0;
        }
      }
    }

    if (buttonState5 == 1 && digitalRead(button5) == HIGH)
    {
      buttonState5 = 0;
    }

  }
  else if (mode_select == normalMode)
  {
    if (!display_selector)
    {
      digit_1();
      display(timer_hours_10);
      
      digit_2();
      display(timer_hours_01);
      
      digit_3();
      display(timer_minutes_10);

      digit_4();
      displayAlarmToggled(timer_minutes_01);

    }
    else
    {
      digit_1();
      display(timer_minutes_10);
      
      digit_2();
      display(timer_minutes_01);
      
      digit_3();
      display(timer_seconds_10);

      digit_4();
      displayAlarmToggled(timer_seconds_01);

    }

    digit_L1L2L3();
    displayColon();
    
  
    //check if button1 has been pressed (snooze)
    if (buttonState1 == 0 && digitalRead(button1) == LOW && alarmActive)
    {
      buttonState1 = 1;
      snoozeActive = true;
    }

    if (buttonState1 == 1 && digitalRead(button1) == HIGH)
    {
      buttonState1 = 0;
    }

    //check if button2 has been pressed (toggle alarm)
    if (buttonState2 == 0 && digitalRead(button2) == LOW)
    {
      buttonState2 = 1;

      if (toggleAlarm && alarmActive)
      {
        alarmActive = !alarmActive;
      }
      

      toggleAlarm = !toggleAlarm;

    }

    if (buttonState2 == 1 && digitalRead(button2) == HIGH)
    {
      buttonState2 = 0;
    }


    //check if button3 has been pressed (time edit mode)
    if (buttonState3 == 0 && digitalRead(button3) == LOW)
    {
      buttonState3 = 1;
      changedClock = 0;
      mode_select = editClockMinutesMode;

      frozen_seconds_01 = timer_seconds_01;
      frozen_seconds_10 = timer_seconds_10;
      frozen_minutes_01 = timer_minutes_01;
      frozen_minutes_10 = timer_minutes_10;
      frozen_hours_01 = timer_hours_01;
      frozen_hours_10 = timer_hours_10;
    }

    if (buttonState3 == 1 && digitalRead(button3) == HIGH)
    {
      buttonState3 = 0;
    }


    //check if button4 has been pressed (alarm edit mode)
    if (buttonState4 == 0 && digitalRead(button4) == LOW)
    {
      buttonState4 = 1;
      mode_select = editAlarmMinutesMode;
    }

    if (buttonState4 == 1 && digitalRead(button4) == HIGH)
    {
      buttonState4 = 0;
    }
  }
}


//interrupt service routine (4 Hz)
ISR(TCA0_CMP1_vect) 
{
    cli(); //stop interrupts

    TCA0.SINGLE.INTFLAGS |= bit(5);     //clear interrupt flag                 

    counter++;

    //1 second has passed...
    if (counter == 4)
    {
      counter = 0;
      timer_seconds_01++;

      if (timer_seconds_01 > 9)
      {
        timer_seconds_10++;
        timer_seconds_01 = 0;
      }
  
      if (timer_seconds_10 > 5)
      {
        timer_minutes_01++;
        timer_seconds_10 = 0;
      }
  
      if (timer_minutes_01 > 9)
      {
        timer_minutes_10++;
        timer_minutes_01 = 0;
      }
  
      if (timer_minutes_10 > 5)
      {
        timer_hours_01++;
        timer_minutes_10 = 0;
      }
  
      if (timer_hours_10 == 2)
      {
        if (timer_hours_01 > 3)
        {
          timer_hours_01 = 0;
          timer_hours_10 = 0;
        }
      }
      else
      {
        if (timer_hours_01 > 9)
        {
          timer_hours_10++;
          timer_hours_01 = 0;
        }
      }

      //check if current time matches alarm time
      if (toggleAlarm)
      {
        if ((timer_hours_10 == alarm_hours_10) && (timer_hours_01 == alarm_hours_01) && (timer_minutes_10 == alarm_minutes_10) && (timer_minutes_01 == alarm_minutes_01) && (timer_seconds_10 == 0) && (timer_seconds_01 == 0))      
        {
          alarmActive = true;
        }
      }

      if (snoozeActive)
      {
        snooze_seconds++;
      }

      if (snooze_seconds > 30)
      {
        snoozeActive = false;
        snooze_seconds = 0;

        if (toggleAlarm)
        {
          alarmActive = true;
        }

      }
    }

    if (snoozeActive)
    {
      alarmActive = false;
    }


    sei(); //allow interrupts
}
