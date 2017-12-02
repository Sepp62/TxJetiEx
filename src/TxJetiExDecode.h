/* 
  Jeti Transmitter EX Telemetry C++ Library
  
  TxJetiDecode - Transmitter telemetry EX protocol decoder
  -------------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.90   11/02/2017  Created
  0.91   11/12/2017  Enumeration of names and labels

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

#ifndef TXJETIEXDECODE_H
#define TXJETIEXDECODE_H

// #define TXJETIEX_DECODE_DEBUG // dump raw data bufer and decoded values to Serial

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include "TxJetiExSerial.h"

class TxJetiExPacket
{
public:
  TxJetiExPacket() : m_packetType( PACKET_NONE ) {}

  enum enPacketType
  {
    PACKET_NONE  = 0,
    PACKET_NAME  = 1,
    PACKET_LABEL = 2,
    PACKET_VALUE = 3,
    PACKET_ALARM = 4,
    PACKET_ERROR = 5,
  }
  EN_PACKET_TYPE;

  // Jeti data types
  enum enDataType
  {
    TYPE_6b   = 0, // int6_t  Data type 6b (-31 �31)
    TYPE_14b  = 1, // int14_t Data type 14b (-8191 �8191)
    TYPE_22b  = 4, // int22_t Data type 22b (-2097151 �2097151)
    TYPE_DT   = 5, // int22_t Special data type � time and date
    TYPE_30b  = 8, // int30_t Data type 30b (-536870911 �536870911) 
    TYPE_GPS  = 9, // int30_t Special data type � GPS coordinates:  lo/hi minute - lo/hi degree. 
  }
  EN_DATA_TYPE;

  uint8_t GetPacketType(){ return m_packetType; } // enPacketType

protected: 
  uint8_t m_packetType; // enPacketType

  union
  {
    uint32_t vInt;
    uint8_t  vBytes[4];
  } i2b; // integer to bytes and vice versa

  static const char * m_strUnknown; // "?"
};

class TxJetiExPacketError : public TxJetiExPacket
{
public:
  TxJetiExPacketError() { m_packetType = PACKET_ERROR; }
};

class TxJetiExPacketLabel;
class TxJetiExPacketName : public TxJetiExPacket
{
  friend class TxJetiExPacketValue;
  friend class TxJetiExPacketLabel;
  friend class TxJetiDecode;
public:
  TxJetiExPacketName() : m_serialId( 0 ), m_pstrName( 0 ), m_pNext( 0 ), m_pFirstLabel( 0 ) { m_packetType = PACKET_NAME; }

  uint32_t GetSerialId(){ return m_serialId; };
  const char * GetName(){ if( m_pstrName ) return m_pstrName; return m_strUnknown; }

protected:
  uint32_t m_serialId;
  char *   m_pstrName;
  
  TxJetiExPacketName  * m_pNext;
  TxJetiExPacketLabel * m_pFirstLabel;
};

class TxJetiExPacketLabel : public TxJetiExPacket
{
  friend class TxJetiExPacketValue;
  friend class TxJetiDecode;
public:
  TxJetiExPacketLabel() : m_id( 0 ), m_serialId( 0 ), m_pstrLabel( 0 ), m_pstrUnit( 0 ), m_pNext( 0 ), m_pName( 0 ) { m_packetType = PACKET_LABEL; }

  uint8_t  GetId(){ return m_id; }   
  uint32_t GetSerialId(){ return m_serialId; };

  const char * GetName()  { if( m_pName )     return m_pName->GetName();    return m_strUnknown; }
  const char * GetLabel() { if( m_pstrLabel ) return m_pstrLabel;           return m_strUnknown; }
  const char * GetUnit()  { if( m_pstrUnit )  return m_pstrUnit;            return m_strUnknown; }

protected:
  uint8_t  m_id;
  uint32_t m_serialId;
  char *   m_pstrLabel;
  char *   m_pstrUnit;

  TxJetiExPacketLabel * m_pNext;
  TxJetiExPacketName  * m_pName;
};

class TxJetiExPacketValue : public TxJetiExPacket
{
  friend class TxJetiDecode;
public:
  TxJetiExPacketValue() : m_id( 0 ), m_pLabel( 0 ) { m_packetType = PACKET_VALUE; }

  uint8_t  GetId(){ return m_id; }   
  uint32_t GetSerialId(){ return m_serialId; };
  uint8_t  GetExType(){ return m_exType; };
  uint32_t GetRawValue(){ return m_value; };

  const char * GetName()  { if( m_pLabel ) return m_pLabel->GetName();  return m_strUnknown; }
  const char * GetLabel() { if( m_pLabel ) return m_pLabel->GetLabel(); return m_strUnknown; }
  const char * GetUnit()  { if( m_pLabel ) return m_pLabel->GetUnit();  return m_strUnknown; }
  
  bool  GetFloat( float * pValue );
  bool  GetLatitude( float * pLatitude );
  bool  GetLongitude( float * pLongitude );
  bool  GetDate( uint8_t * pDay,  uint8_t * pMonth,  uint16_t * pYear );
  bool  GetTime( uint8_t * pHour, uint8_t * pMinute, uint8_t * pSecond );

  bool  IsValueComplete(); // check if label or unit name is missing to eventually call "TxJetiExDecode::CompleteValue(...)" with previoulsy store sensor data

protected:
  bool GetGPS( bool * pbLongitude, float * pCoord );
  bool IsNumeric();

  uint8_t     m_id;
  uint32_t    m_serialId;
  int32_t     m_value;
  uint8_t     m_exType;   // enDataType
  uint8_t     m_exponent; // 0, 1=10E-1, 2=10E-2

  TxJetiExPacketLabel * m_pLabel;
};

class TxJetiPacketAlarm: public TxJetiExPacket
{
  friend class TxJetiDecode;
public:
  TxJetiPacketAlarm() : m_bSound( 0 ), m_code( 0 ) {}
  bool    GetSound(){ return m_bSound; }
  uint8_t GetCode(){ return m_code; }
protected:
  uint8_t m_bSound;
  uint8_t m_code;
};

class TxJetiDecode
{
public:
  TxJetiDecode() : m_state( WAIT_STARTOFPACKET ), m_bIsData( false), m_nPacketLen( 0 ), m_nBytes( 0 ), m_pSensorList( 0 ) {}

  enum enComPort
  {
    DEFAULTPORT = 0x00,
    SERIAL1     = 0x01,
    SERIAL2     = 0x02,
    SERIAL3     = 0x03,
  };

  void             Start( enComPort comPort = DEFAULTPORT );
  TxJetiExPacket * GetPacket(); 

  // add eventually missing label, unit and name from persisted data. Check "!TxJetiExPacketValue::>IsValueComplete()" if this is necessary 
  bool CompleteValue( TxJetiExPacketValue * pValue, const char * pstrName, const char * pstrLabel, const char * pstrUnit );

  // name and label enumeration (i.e. for persistence)
  TxJetiExPacketName  * GetFirstName() { return m_pSensorList; }
  TxJetiExPacketName  * GetNextName( TxJetiExPacketName * pName ){ if( pName ) return pName->m_pNext; return NULL; }
  TxJetiExPacketLabel * GetFirstLabel( TxJetiExPacketName * pName ){ if( pName ) return pName->m_pFirstLabel; return NULL; }
  TxJetiExPacketLabel * GetNextLabel( TxJetiExPacketLabel * pLabel ){ if( pLabel ) return pLabel->m_pNext; return NULL; };

protected:

  enum enPacketState
  {
    WAIT_STARTOFPACKET = 0,
    WAIT_EX_BYTE,
    WAIT_LEN,
    WAIT_ENDOFEXPACKET,
    WAIT_ENDOFALARM,
    WAIT_NEXTVALUE,
  };

  // serial interface
  TxJetiExSerial * m_pSerial;

  // packet state
  uint8_t m_state;

  // data buffer handling
  bool    m_bIsData;       // true: EX data, false: text
  uint8_t m_nPacketLen;    // length of EX data packet
  uint8_t m_nBytes;        // current byte counter
  uint8_t m_exBuffer[32];  // EX data buffer

  // EX decoder
  TxJetiExPacket * DecodeName();
  TxJetiExPacket * DecodeLabel();
  TxJetiExPacket * DecodeValue();

  // data output
  TxJetiExPacketName * m_pSensorList;
  TxJetiExPacketValue  m_value;
  TxJetiPacketAlarm    m_alarm;
  TxJetiExPacketError  m_error;

  // sensor helpers
  char * NewName();
  char * NewUnit();
  char * NewString( const char * pStr );
  TxJetiExPacketName  * FindName( uint32_t serialId );
  TxJetiExPacketLabel * FindLabel( uint32_t serialId, uint8_t id );
  void AppendName( TxJetiExPacketName * pName );
  void AppendLabel( TxJetiExPacketLabel * pLabel );

  // Helpers
  bool    crcCheck();
  uint8_t update_crc (uint8_t crc, uint8_t crc_seed);

  // debugging
  #ifdef TXJETIEX_DECODE_DEBUG
  void DumpSerial( int numChar, uint16_t sChar );
  void DumpBuffer( uint8_t * buffer, uint8_t nChars );
  void DumpOutput( TxJetiExPacket * pPacket );
  const char * GetDataTypeString( uint8_t dataType );
  #else
  void DumpSerial( int numChar, uint16_t sChar ){}
  void DumpBuffer( uint8_t * buffer, uint8_t nChars ){}
  void DumpOutput( TxJetiExPacket * pPacket ){}
  #endif
};

#endif // TXJETIEXDECODE_H
