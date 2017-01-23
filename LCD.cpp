// ---------------------------------------------------------------------------
// Created by Francisco Malpartida on 20/08/11.
// Copyright 2011 - Under creative commons license 3.0:
//        Attribution-ShareAlike CC BY-SA
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// Thread Safe: No
// Extendable: Yes
//
// @file LCD.cpp
// This file implements a basic liquid crystal library that comes as standard
// in the Arduino SDK.
// 
// @brief 
// This is a basic implementation of the HD44780 library of the
// Arduino SDK. This library is a refactored version of the one supplied
// in the Arduino SDK in such a way that it simplifies its extension
// to support other mechanism to communicate to LCDs such as I2C, Serial, SR, ...
// The original library has been reworked in such a way that this will be
// the base class implementing all generic methods to command an LCD based
// on the Hitachi HD44780 and compatible chipsets.
//
// This base class is a pure abstract class and needs to be extended. As reference,
// it has been extended to drive 4 and 8 bit mode control, LCDs and I2C extension
// backpacks such as the I2CLCDextraIO using the PCF8574* I2C IO Expander ASIC.
//
//
// @version API 1.1.0
//
// 2012.03.29 bperrybap - changed comparision to use LCD_5x8DOTS rather than 0
// @author F. Malpartida - fmalpartida@gmail.com
// ---------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <StackTrace.h>

//extern "C" void __cxa_pure_virtual() { while (1); }
#include "LCD.h"

#define FILE_ID 1


// CLASS CONSTRUCTORS
// ---------------------------------------------------------------------------
// Constructor
LCD::LCD () 
{
   
}

// PUBLIC METHODS
// ---------------------------------------------------------------------------
// When the display powers up, it is configured as follows:
// 0. LCD starts in 8 bit mode
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1 
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a application starts (and the
// LiquidCrystal constructor is called).
// A call to begin() will reinitialize the LCD.
//
void LCD::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) 
{
   PUSH_STACK(FILE_ID);
   if (lines > 1) 
   {
      _displayfunction |= LCD_2LINE;
   }
   _numlines = lines;
   _cols = cols;
   
   // for some 1 line displays you can select a 10 pixel high font
   // ------------------------------------------------------------
   if ((dotsize != LCD_5x8DOTS) && (lines == 1)) 
   {
      _displayfunction |= LCD_5x10DOTS;
   }
   
   // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
   // according to datasheet, we need at least 40ms after power rises above 2.7V
   // before sending commands. Arduino can turn on way before 4.5V so we'll wait 
   // 50
   // ---------------------------------------------------------------------------
   delay (100); // 100ms delay
   
   //put the LCD into 4 bit or 8 bit mode
   // -------------------------------------
   if (! (_displayfunction & LCD_8BITMODE)) 
   {
      // this is according to the hitachi HD44780 datasheet
      // figure 24, pg 46
      
      PUSH_STACK(FILE_ID);
      // we start in 8bit mode, try to set 4 bit mode
      // Special case of "Function Set"
      send(0x03, FOUR_BITS);
      delayMicroseconds(4500); // wait min 4.1ms
      POP_STACK;
      
      PUSH_STACK(FILE_ID);
      // second try
      send ( 0x03, FOUR_BITS );
      delayMicroseconds(150); // wait min 100us
      POP_STACK;
      
      PUSH_STACK(FILE_ID);
      // third go!
      send( 0x03, FOUR_BITS );
      delayMicroseconds(150); // wait min of 100us
      POP_STACK;
      
      PUSH_STACK(FILE_ID);
      // finally, set to 4-bit interface
      send ( 0x02, FOUR_BITS );
      delayMicroseconds(150); // wait min of 100us
      POP_STACK;

   } 
   else 
   {
      // this is according to the hitachi HD44780 datasheet
      // page 45 figure 23
      
      PUSH_STACK(FILE_ID);
      // Send function set command sequence
      command(LCD_FUNCTIONSET | _displayfunction);
      delayMicroseconds(4500);  // wait more than 4.1ms
      POP_STACK;
      
      PUSH_STACK(FILE_ID);
      // second try
      command(LCD_FUNCTIONSET | _displayfunction);
      delayMicroseconds(150);
      POP_STACK;
      
      PUSH_STACK(FILE_ID);
      // third go
      command(LCD_FUNCTIONSET | _displayfunction);
      delayMicroseconds(150);
      POP_STACK;

   }
   
   PUSH_STACK(FILE_ID);
   // finally, set # lines, font size, etc.
   command(LCD_FUNCTIONSET | _displayfunction);
   delayMicroseconds ( 60 );  // wait more
   POP_STACK;
   
   // turn the display on with no cursor or blinking default
   _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
   display();
   
   // clear the LCD
   clear();
   
   PUSH_STACK(FILE_ID);
   // Initialize to default text direction (for romance languages)
   _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
   // set the entry mode
   command(LCD_ENTRYMODESET | _displaymode);
   POP_STACK;

   backlight();

   POP_STACK;
}

// Common LCD Commands
// ---------------------------------------------------------------------------
void LCD::clear()
{
   PUSH_STACK(FILE_ID);
   command(LCD_CLEARDISPLAY);             // clear display, set cursor position to zero
   delayMicroseconds(HOME_CLEAR_EXEC);    // this command is time consuming
   POP_STACK;
}

void LCD::home()
{
   PUSH_STACK(FILE_ID);
   command(LCD_RETURNHOME);             // set cursor position to zero
   delayMicroseconds(HOME_CLEAR_EXEC);  // This command is time consuming
   POP_STACK;
}

void LCD::setCursor(uint8_t col, uint8_t row)
{
   PUSH_STACK(FILE_ID);
   const byte row_offsetsDef[]   = { 0x00, 0x40, 0x14, 0x54 }; // For regular LCDs
   const byte row_offsetsLarge[] = { 0x00, 0x40, 0x10, 0x50 }; // For 16x4 LCDs
   
   if ( row >= _numlines ) 
   {
      row = _numlines-1;    // rows start at 0
   }
   
   // 16x4 LCDs have special memory map layout
   // ----------------------------------------
   if ( _cols == 16 && _numlines == 4 )
   {
      PUSH_STACK(FILE_ID);
      command(LCD_SETDDRAMADDR | (col + row_offsetsLarge[row]));
      POP_STACK;
   }
   else 
   {
      PUSH_STACK(FILE_ID);
      command(LCD_SETDDRAMADDR | (col + row_offsetsDef[row]));
      POP_STACK;
   }
   POP_STACK;
}

// Turn the display on/off
void LCD::noDisplay() 
{
   PUSH_STACK(FILE_ID);
   _displaycontrol &= ~LCD_DISPLAYON;
   command(LCD_DISPLAYCONTROL | _displaycontrol);
   POP_STACK;
}

void LCD::display() 
{
   PUSH_STACK(FILE_ID);
   _displaycontrol |= LCD_DISPLAYON;
   command(LCD_DISPLAYCONTROL | _displaycontrol);
   POP_STACK;
}

// Turns the underline cursor on/off
void LCD::noCursor() 
{
   PUSH_STACK(FILE_ID);
   _displaycontrol &= ~LCD_CURSORON;
   command(LCD_DISPLAYCONTROL | _displaycontrol);
   POP_STACK;
}
void LCD::cursor() 
{
   PUSH_STACK(FILE_ID);
   _displaycontrol |= LCD_CURSORON;
   command(LCD_DISPLAYCONTROL | _displaycontrol);
   POP_STACK;
}

// Turns on/off the blinking cursor
void LCD::noBlink() 
{
   PUSH_STACK(FILE_ID);
   _displaycontrol &= ~LCD_BLINKON;
   command(LCD_DISPLAYCONTROL | _displaycontrol);
   POP_STACK;
}

void LCD::blink() 
{
   PUSH_STACK(FILE_ID);
   _displaycontrol |= LCD_BLINKON;
   command(LCD_DISPLAYCONTROL | _displaycontrol);
   POP_STACK;
}

// These commands scroll the display without changing the RAM
void LCD::scrollDisplayLeft(void) 
{
   PUSH_STACK(FILE_ID);
   command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
   POP_STACK;
}

void LCD::scrollDisplayRight(void) 
{
   PUSH_STACK(FILE_ID);
   command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
   POP_STACK;
}

// This is for text that flows Left to Right
void LCD::leftToRight(void) 
{
   PUSH_STACK(FILE_ID);
   _displaymode |= LCD_ENTRYLEFT;
   command(LCD_ENTRYMODESET | _displaymode);
   POP_STACK;
}

// This is for text that flows Right to Left
void LCD::rightToLeft(void) 
{
   PUSH_STACK(FILE_ID);
   _displaymode &= ~LCD_ENTRYLEFT;
   command(LCD_ENTRYMODESET | _displaymode);
   POP_STACK;
}

// This method moves the cursor one space to the right
void LCD::moveCursorRight(void)
{
   PUSH_STACK(FILE_ID);
   command(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVERIGHT);
   POP_STACK;
}

// This method moves the cursor one space to the left
void LCD::moveCursorLeft(void)
{
   PUSH_STACK(FILE_ID);
   command(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVELEFT);
   POP_STACK;
}


// This will 'right justify' text from the cursor
void LCD::autoscroll(void) 
{
   PUSH_STACK(FILE_ID);
   _displaymode |= LCD_ENTRYSHIFTINCREMENT;
   command(LCD_ENTRYMODESET | _displaymode);
   POP_STACK;
}

// This will 'left justify' text from the cursor
void LCD::noAutoscroll(void) 
{
   PUSH_STACK(FILE_ID);
   _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
   command(LCD_ENTRYMODESET | _displaymode);
   POP_STACK;
}

// Write to CGRAM of new characters
void LCD::createChar(uint8_t location, uint8_t charmap[]) 
{
   PUSH_STACK(FILE_ID);
   location &= 0x7;            // we only have 8 locations 0-7
   
   command(LCD_SETCGRAMADDR | (location << 3));
   delayMicroseconds(30);
   
   PUSH_STACK(FILE_ID);
   for (uint8_t i = 0; i < 8; i++)
   {
      write(charmap[i]);      // call the virtual write method
      delayMicroseconds(40);
   }
   POP_STACK;
   POP_STACK;
}

#ifdef __AVR__
void LCD::createChar(uint8_t location, const char *charmap)
{
   PUSH_STACK(FILE_ID);
   location &= 0x7;   // we only have 8 memory locations 0-7
   
   command(LCD_SETCGRAMADDR | (location << 3));
   delayMicroseconds(30);
   
   PUSH_STACK(FILE_ID);
   for (uint8_t i = 0; i < 8; i++)
   {
      write(pgm_read_byte_near(charmap++));
      delayMicroseconds(40);
   }
   POP_STACK;
   POP_STACK;
}
#endif // __AVR__

//
// Switch on the backlight
void LCD::backlight ( void )
{
   PUSH_STACK(FILE_ID);
   setBacklight(255);
   POP_STACK;
}

//
// Switch off the backlight
void LCD::noBacklight ( void )
{
   PUSH_STACK(FILE_ID);
   setBacklight(0);
   POP_STACK;
}

//
// Switch fully on the LCD (backlight and LCD)
void LCD::on ( void )
{
   PUSH_STACK(FILE_ID);
   display();
   backlight();
   POP_STACK;
}

//
// Switch fully off the LCD (backlight and LCD) 
void LCD::off ( void )
{
   PUSH_STACK(FILE_ID);
   noBacklight();
   noDisplay();
   POP_STACK;
}

// General LCD commands - generic methods used by the rest of the commands
// ---------------------------------------------------------------------------
void LCD::command(uint8_t value) 
{
   PUSH_STACK(FILE_ID);
   send(value, COMMAND);
   POP_STACK;
}

#if (ARDUINO <  100)
void LCD::write(uint8_t value)
{
   send(value, DATA);
}
#else
size_t LCD::write(uint8_t value) 
{
   return send(value, DATA);
}
#endif
