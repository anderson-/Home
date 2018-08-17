
/*
Beady_Hello.pde

 Demo program software for the Bulbdial Clock kit desiged by
 Evil Mad Scientist Laboratories: http://www.evilmadscientist.com/go/bulbdialkit

 Designed to work with Arduino 17.
 Also requires DateTime library:  http://www.arduino.cc/playground/Code/DateTime

 Target: ATmega168, clock at 16 MHz.


 Version 1.0 - 12/26/2009
 Copyright (c) 2009 Windell H. Oskay.  All right reserved.
 http://www.evilmadscientist.com/

 This library is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this library.  If not, see <http://www.gnu.org/licenses/>.

 */



// The buttons are located at D5, D6, & D7.
#define buttonmask 224

// LED outputs B0-B2:
#define LEDsB 7

// C0-C3 are LED outputs:
#define LEDsC 15

// TX, PD2,PD3,PD4 are LED outputs.
#define LEDsD 28

// Negative masks of those LED positions, for quick turn-off:

#define LEDsBInv 248
#define LEDsCInv 240
#define LEDsDInv 227

#define LED_B_Off();   DDRB &= LEDsBInv;  PORTB &= LEDsBInv;
#define LED_C_Off();   DDRC &= LEDsCInv;  PORTC &= LEDsCInv;
#define LED_D_Off();   DDRD &= LEDsDInv;  PORTD &= LEDsDInv;

#define AllLEDsOff();  LED_B_Off(); LED_C_Off(); LED_D_Off();



void TakeHigh(byte LEDline)
{
  switch( LEDline )
  {
  case 1:
    DDRB  |= 4;
    PORTB |= 4;
    break;
  case 2:
    DDRC  |= 1;
    PORTC |= 1;
    break;
  case 3:
    DDRC  |= 2;
    PORTC |= 2;
    break;
  case 4:
    DDRC  |= 4;
    PORTC |= 4;
    break;
  case 5:
    DDRC  |= 8;
    PORTC |= 8;
    break;
  case 6:
    DDRD  |= 16;
    PORTD |= 16;
    break;
  case 7:
    DDRD  |= 4;
    PORTD |= 4;
    break;
  case 8:
    DDRB  |= 1;
    PORTB |= 1;
    break;
  case 9:
    DDRD  |= 8;
    PORTD |= 8;
    break;
  case 10:
    DDRB  |= 2;
    PORTB |= 2;
    break;
    // default:
  }
}


void delayTime(byte time)
{
  unsigned int delayvar;
  delayvar = 0;
  while (delayvar <=  time)
  {
    asm("nop");
    delayvar++;
  }
}

void TakeLow(byte LEDline)
{
  switch( LEDline )
  {
  case 1:
    DDRB  |= 4;
    PORTB &= 251;
    break;
  case 2:
    DDRC  |= 1;
    PORTC &= 254;
    break;
  case 3:
    DDRC  |= 2;
    PORTC &= 253;
    break;
  case 4:
    DDRC  |= 4;
    PORTC &= 251;
    break;
  case 5:
    DDRC  |= 8;
    PORTC &= 247;
    break;
  case 6:
    DDRD  |= 16;
    PORTD &= 239;
    break;
  case 7:
    DDRD  |= 4;
    PORTD &= 251;
    break;
  case 8:
    DDRB  |= 1;
    PORTB &= 254;
    break;
  case 9:
    DDRD  |= 8;
    PORTD &= 247;
    break;
  case 10:
    DDRB  |= 2;
    PORTB &= 253;
    break;
    // default:
  }
}




const byte SecHi[30] = {
  2,3,4,5,6,1,3,4,5,6,1,2,4,5,6,1,2,3,5,6,1,2,3,4,6,1,2,3,4,5};
const byte SecLo[30] = {
  1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,6,6,6,6,6};

const byte MinHi[30] = {
  1,7,1,8,1,9,2,7,2,8,2,9,3,7,3,8,3,9,4,7,4,8,4,9,5,7,5,8,5,9};
const byte MinLo[30] = {
  7,1,8,1,9,1,7,2,8,2,9,2,7,3,8,3,9,3,7,4,8,4,9,4,7,5,8,5,9,5};

const byte HrHi[12]  = {
  10, 1, 2,10,10, 6, 3, 10,10, 4, 5,10};
const byte HrLo[12]  = {
  1,10,10, 2, 6,10,10, 3, 4,10,10, 5};

byte SecNow, MinNow, HrNow;

byte HrDisp,MinDisp, SecDisp;

unsigned long LastTime;


byte PINDLast;

unsigned long millisCopy;

byte h0, h1, h2, h3, h4, h5;
byte l0, l1, l2;



void setup()                    // run once, when the sketch starts
{


  PORTB = 0;
  PORTC = 0;
  PORTD = 0;

  DDRB = 0;  // All inputs
  DDRC = 0;  // All inputs
  DDRD = _BV(1); //All inputs except TX.

  PORTD = buttonmask;  // Pull-up resistors for buttons

  SecNow = 0;
  HrNow = 0;
  MinNow = 0;
  HrDisp = 6;
  SecDisp = 15;
  MinDisp = 15;
LastTime = 0;


  PINDLast =  PIND & buttonmask;

}  // End Setup


void checkButtons()
{
  byte PINDcopy;


  PINDcopy = PIND & buttonmask;

  if (PINDcopy != PINDLast)  // Button change detected
  {


    if ((PINDcopy & 32) && ((PINDLast & 32) == 0))
    { //"+" Button was pressed previously, and was just released!

      HrNow++;
      if (HrNow > 11)
        HrNow = 0;
    }


    if ((PINDcopy & 64) && ((PINDLast & 64) == 0))
    { //"-" Button was pressed and just released!
      MinNow++;
      if (MinNow > 59)
        MinNow = 0;
    }

    if ((PINDcopy & 128) && ((PINDLast & 128) == 0))
    { //"Z" Button was pressed and just released!

      SecNow++;
      if (SecNow > 59)
        SecNow = 0;
    }
  }

  PINDLast = PINDcopy;
}

void tick()
{
  millisCopy = millis();

  // The next if statement detects and deals with the millis() rollover.
  // This introduces an error of up to  1 s, about every 50 days.
  //
  // (If you have the standard quartz timebase, this will not dominate the inaccuracy.
  // If you have the optional RTC, this error will be corrected next time we read the
  // time from the RTC.)

  if (millisCopy < LastTime)
    LastTime = 0;

  if ((millisCopy - LastTime) >= 1000)
  {
    LastTime += 1000;

    SecNow++;

    if (SecNow > 59){
      SecNow = 0;
      MinNow++;
    }

    if (MinNow > 59){
      MinNow = 0;
      HrNow++;

      if  (HrNow > 11)
        HrNow = 0;
    }

    SecDisp = (SecNow + 30);  // Offset by 30 s to project *shadow* in the right place.
    if ( SecDisp > 59)
      SecDisp -= 60;
    SecDisp >>= 1;  // Divide by two, since there are 30 LEDs, not 60.

    MinDisp = (MinNow + 30); // Offset by 30 m to project *shadow* in the right place.
    if ( MinDisp > 59)
      MinDisp -= 60;
    MinDisp >>= 1;  // Divide by two, since there are 30 LEDs, not 60.

    HrDisp = (HrNow + 6); // Offset by 6 h to project *shadow* in the right place.

    if ( HrDisp > 11)
      HrDisp -= 12;


  }
}

void draw()
{
  //Calculate which LEDs to light up to give
  // the correct shadows:

  h3 = HrDisp;
  h4 = MinDisp;
  h5 = SecDisp;

  h0 = HrHi[h3];
  l0 = HrLo[h3];

  h1 = MinHi[h4];
  l1  = MinLo[h4];

  h2 = SecHi[h5];
  l2  = SecLo[h5];


  //  This is the loop where we actually light up the LEDs:

  byte i = 0;
  while (i < 128)
  {

    TakeHigh(h0);
    TakeLow(l0);
    delayTime(128);
    AllLEDsOff();

    TakeHigh(h1);
    TakeLow(l1);
    delayTime(128);
    AllLEDsOff();

    TakeHigh(h2);
    TakeLow(l2);
    delayTime(128);
    AllLEDsOff();

    i++;
  }
}

void loop()
{
  checkButtons();

  tick();

  draw();
}
