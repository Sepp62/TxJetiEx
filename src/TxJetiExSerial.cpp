/* 
  Jeti Transmitter EX Telemetry C++ Library
  
  TxJetiExSerial - Transmitter telemetry EX serial implementation
                   For Teensy: uncomment #define SERIAL_9BIT_SUPPORT in
                   ...\Arduino\hardware\teensy\avr\cores\teensy3\HardwareSerial.h
  --------------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.90   11/02/2015  created
  0.91   11/12/2017  8 bit UART mode added.
                     Define TXJETIEX_ARDUINO_UART in TxJetiExSerial.h
  0.92   11/13/2017  8 bit UART mode bug fix and baud rate tuning.
  0.93   11/15/2017  first experimental shot for ESP32 support
  
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

**************************************************************/

#include "TxJetiExSerial.h"

#ifdef ESP32
  #define HAVE_HWSERIAL1
  HardwareSerial Serial1(2);
#endif 

// Teensy
/////////
#if defined( CORE_TEENSY ) 

  TxJetiExSerial * TxJetiExSerial::CreatePort( int comPort )
  {
    return new TxJetiExTeensySerial( comPort );
  } 

  TxJetiExTeensySerial::TxJetiExTeensySerial( int comPort ) 
  {
    switch( comPort )
    {
    default:
    case 2: m_pSerial = &Serial2; break;
    case 1: m_pSerial = &Serial1; break;
    case 3: m_pSerial = &Serial3; break;
    }
  }

  void TxJetiExTeensySerial::Init()
  {
     m_pSerial->begin( 9600, SERIAL_9O1 );
  }

  uint16_t TxJetiExTeensySerial::Getchar(void) 
  {                                         
    if( m_pSerial->available() > 0 )
      return m_pSerial->read();

    return 0;
  }

#elif defined (TXJETIEX_ARDUINO_UART)

  // Arduino HardwareSerial
  /////////////////////////
  TxJetiExSerial * TxJetiExSerial::CreatePort( int comPort )
  {
    return new TxJetiExArduinoSerial( comPort );
  } 

  TxJetiExArduinoSerial::TxJetiExArduinoSerial( int comPort ) 
  {
    c_minus1 = c_minus2 = c_minus3 = 0;

    switch( comPort )
    {
    default:
      #ifdef HAVE_HWSERIAL0
        m_pSerial = &Serial;
      #elif defined ( HAVE_HWSERIAL1 )
        m_pSerial = &Serial1;
      #endif 
      break;
    case 1:
      #ifdef HAVE_HWSERIAL1
        m_pSerial = &Serial1;
      #endif 
      break;
    case 2:
      #ifdef HAVE_HWSERIAL2
        m_pSerial = &Serial2;
      #endif
      break;
    case 3:
      #ifdef HAVE_HWSERIAL2
        m_pSerial = &Serial3;
      #endif
      break;
    }
  }

  void TxJetiExArduinoSerial::Init()
  {
    #ifdef ESP32
      m_pSerial->begin( 9600, SERIAL_8O2 ); 
    #else
      m_pSerial->begin( 9420, SERIAL_8N1 ); // 9420...9450 on AtMega 
    #endif
  }

  uint16_t TxJetiExArduinoSerial::Getchar(void) 
  {                                         
    if( m_pSerial->available() > 0 )
    {
      uint16_t c = m_pSerial->read() | 0x100;
      // 9th bit emulation, check for sequence 0xfe, 0xff, 0x7e --> first ex packet after startup will not be decoded
      if( (c & 0x00ff) == 0x007e && (c_minus1 & 0x00ff) == 0x00ff && (c_minus2 & 0x00ff) == 0x00fe )
      {
        c_minus3 &= ~0x100;
        c_minus2 &= ~0x100;
        c_minus1 &= ~0x100;
        c        &= ~0x100;
      }
      c_minus3 = c_minus2;
      c_minus2 = c_minus1;
      c_minus1 = c;
      return c_minus3;
    }
    return 0;
  }

#else

// ATMega
/////////

#include <avr/io.h>
#include <avr/interrupt.h>

// HARDWARE SERIAL
//////////////////
void TxJetiExAtMegaSerial::Init() // pins are unsued for hardware version
{
  // init UART-registers
  UCSRA = 0x00;
  UCSRB = _BV(UCSZ2) | _BV(RXEN) /* | _BV(TXEN) */;         // 9 Bit, RX enable, Tx disable
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1) | _BV(UPM0) | _BV(UPM1) ; // 9-bit data, 2 stop bits, odd parity

  // wormfood.net/avrbaudcalc.php 
#if F_CPU == 16000000L  // for the 16 MHz clock on most Arduino boards
  UBRRH = 0x00;
  UBRRL = 0x66; // 9800 Bit/s
#elif F_CPU == 8000000L   // for the 8 MHz internal clock (Pro Mini 3.3 Volt) 
  UBRRH = 0x00;
  UBRRL = 0x32; // 9800 Bit/s
#else
  #error Unsupported clock speed
#endif  

  // TX and RX pins goes high, when disabled
  pinMode( 0, INPUT_PULLUP );
  pinMode( 1, INPUT_PULLUP );
}


// Interrupt driven transmission
////////////////////////////////
TxJetiExHardwareSerialInt * _pInstance = 0;   // instance pointer to find the serial object from ISR

// static function for port object creation
TxJetiExSerial * TxJetiExSerial::CreatePort( int comPort )
{
  return new TxJetiExHardwareSerialInt();
} 

void TxJetiExHardwareSerialInt::Init()
{
  TxJetiExAtMegaSerial::Init();

  // init rx ring buffer 
  m_rxHeadPtr = m_rxBuf;
  m_rxTailPtr = m_rxBuf;
  m_rxNumChar = 0;

  _pInstance  = this; // there is a single instance only

  // enable receiver
  uint8_t ucsrb = UCSRB; 
  ucsrb        &= ~( (1<<TXEN) | (1<<TXCIE) ); // disable transmitter and tx interrupt when there is nothing more to send
  ucsrb        |=    (1<<RXEN) | (1<<RXCIE);   // enable receiver with interrupt
  UCSRB        = ucsrb;
}

// Read key from Jeti box
uint16_t TxJetiExHardwareSerialInt::Getchar(void)
{
  uint16_t c = 0;

  if( m_rxNumChar ) // atomic operation
  {
    cli();
    c = *(_pInstance->m_rxTailPtr);
    m_rxNumChar--; 
    m_rxTailPtr = IncBufPtr( m_rxTailPtr, m_rxBuf, RX_RINGBUF_SIZE );
    sei();
  }
  return c;
}


// increment buffer pointer (todo: use templates for 8 and 16 bit versions of pointers)
volatile uint16_t * TxJetiExHardwareSerialInt::IncBufPtr( volatile uint16_t * ptr, volatile uint16_t * pRingBuf, size_t bufSize )
{
  ptr++;
  if( ptr >= ( pRingBuf + bufSize ) )
    return pRingBuf; // wrap around
  else
    return ptr;
}

// ISR - receiver buffer full
ISR( USART_RX_vect )
{
  // uint8_t status = UCSR0A;
  uint16_t bit8 = (UCSRB & _BV(RXB8)) ? 0x0100 : 0x0000;   
  *(_pInstance->m_rxHeadPtr) = bit8 | UDR;  // write data to buffer
  _pInstance->m_rxNumChar++;                // increase number of characters in buffer
  _pInstance->m_rxHeadPtr = _pInstance->IncBufPtr( _pInstance->m_rxHeadPtr, _pInstance->m_rxBuf, _pInstance->RX_RINGBUF_SIZE );    // increase ringbuf pointer
}

#endif // CORE_TEENSY 
