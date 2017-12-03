
/* 
  Jeti Transmitter EX Telemetry C++ Library
  
  TxJetiExBLE.ino - Example printing telemetry data to Bluetooth BLE
                    Use with ESP32
					See basics: https://en.wikipedia.org/wiki/JSON_Streaming

  Needs:
  ESP32_BLE   - delivered with this library
  ArduinoJson - https://github.com/bblanchon/ArduinoJson
  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.94   11/16/2017  created

**************************************************************/

#include <ArduinoJson.h>

#include "TxJetiExDecode.h"
#include "TxJetiExBLESend.h"

TxJetiDecode    jetiDecode;
TxJetiExBLESend bleSend;

void setup()
{
  Serial.begin(115200);
  bleSend.init( 300 ); // give 300 bytes for ring buffer 
  jetiDecode.Start();  // for devices with more than one UART (i.e. Teensy): jetiDecode.Start( TxJetiDecode::SERIAL1..3 );
}

void loop()
{
  TxJetiExPacket * pPacket;

  if( ( pPacket = jetiDecode.GetPacket() ) != NULL ) 
  {
    switch( pPacket->GetPacketType() )
    {
    case TxJetiExPacket::PACKET_NAME:
      PrintJsonName( (TxJetiExPacketName *)pPacket );
      break;
    case TxJetiExPacket::PACKET_LABEL:
      PrintJsonLabel( (TxJetiExPacketLabel *)pPacket );
      break;
    case TxJetiExPacket::PACKET_VALUE:
      PrintJsonValue( (TxJetiExPacketValue *)pPacket );
      break;
    case TxJetiExPacket::PACKET_ALARM:
      PrintJsonAlarm( (TxJetiPacketAlarm *)pPacket );
      break;
    case TxJetiExPacket::PACKET_ERROR:
      PrintJsonError();
      break;
    }
  }
  
  bleSend.doSend();
  ShowDiag();
}

void PrintJsonName( TxJetiExPacketName * pName )
{
  StaticJsonBuffer<150> jsonBuf;
  JsonObject& root = jsonBuf.createObject();
  root["t"] = 0; // type "name"
  root["s"] = pName->GetSerialId();
  root["n"] = pName->GetName();
  SendJson( root );
}

void PrintJsonLabel( TxJetiExPacketLabel * pLabel )
{
  StaticJsonBuffer<150> jsonBuf;
  JsonObject& root = jsonBuf.createObject();
  root["t"] = 1; // type "label"
  root["s"] = pLabel->GetSerialId();
  root["n"] = pLabel->GetName();
  root["i"] = pLabel->GetId();
  root["l"] = pLabel->GetLabel();
  root["u"] = pLabel->GetUnit();
  SendJson( root );
}

void PrintJsonValue( TxJetiExPacketValue * pValue)
{
  StaticJsonBuffer<300> jsonBuf;
  JsonObject& root = jsonBuf.createObject();
  root["t"] = 2; // type "value"
  root["s"] = pValue->GetSerialId();
  root["n"] = pValue->GetName();
  root["i"] = pValue->GetId();
  root["l"] = pValue->GetLabel();
  root["u"] = pValue->GetUnit();
  root["x"] = pValue->GetExType();

  char buf[25];
  float fValue;
  uint8_t day;  uint8_t month;  uint16_t year;
  uint8_t hour; uint8_t minute; uint8_t second;
  if( pValue->GetFloat( &fValue ) )
  {
    root["v"] = fValue;
  }
  else if( pValue->GetLatitude( &fValue ) )
  {
    sprintf( buf, "%2.5f", fValue );
    root["v"] = buf;
  }
  else if( pValue->GetLongitude( &fValue ) )
  {
    sprintf( buf, "%2.5f", fValue );
    root["v"] = buf;
  }
  else if( pValue->GetDate( &day, &month, &year ) )
  {
    sprintf( buf, "%d.%d.%d", day, month, year );
    root["v"] = buf;
  }
  else if( pValue->GetTime( &hour, &minute, &second ) )
  {
    sprintf( buf, "%d:%d:%d", hour, minute, second );
    root["v"] = buf;
  }
  else
    root["v"] = pValue->GetRawValue();
  SendJson( root );
} 

void PrintJsonError()
{
  StaticJsonBuffer<150> jsonBuf;
  JsonObject& root = jsonBuf.createObject();
  root["t"] = 3; // type "error"
  root["e"] = "error";
  SendJson( root );
}

void PrintJsonAlarm( TxJetiPacketAlarm * pAlarm )
{
  StaticJsonBuffer<150> jsonBuf;
  JsonObject& root = jsonBuf.createObject();
  root["t"] = 4; // type "alarm"
  root["c"] = pAlarm->GetCode();
  root["s"] = pAlarm->GetSound();
  SendJson( root );
}

void SendJson( JsonObject& json )
{
  if( json.measureLength() < bleSend.getFreeBuffer() ) // check, if queue has space
    json.printTo( bleSend ); 
  else
    Serial.println( "packet skipped" );

  // json.prettyPrintTo( Serial );
}

const char * GetDataTypeString( uint8_t dataType )
{
  static const char * typeBuf[] = { "6b", "14b", "?", "?", "22b", "DT", "?", "?", "30b", "GPS" };
  return typeBuf[ dataType < 10 ? dataType : 2 ];
}

void ShowDiag()
{
  static unsigned long lastMillis = 0;
  if( millis() > (lastMillis + 2000) )
  {
    Serial.printf( "*** Tx power: %d, Conn: %d ***\n", bleSend.getTxDefaultPower(), bleSend.isConnected() );
    lastMillis = millis();
  }
}

