
/* 
  Jeti Transmitter EX Telemetry C++ Library
  
  TxJetiExBLE.ino - Example to send last GPS location via BLE
                    Use with ESP32

  Needs:
  ESP32_BLE   - delivered with this library (see zip)
  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.90   12/09/2017  created

**************************************************************/


#include "TxJetiExDecode.h"
#include "TxJetiExBLESendLoc.h"

// uncomment, when you want to have permanent connectivity
#define TXJETIEXBLE_OFFLINE_MODE

TxJetiDecode       jetiDecode;
TxJetiExBLESendLoc bleSendLoc;

struct gpspos
{
    float lat;
    float lon;
}
_gpspos = { 0, 0 };

enum enState
{
  EN_OFFLINE    = 0,
  EN_WAITONLINE = 1,
  EN_ONLINE     = 2,
}
_enState = EN_OFFLINE;

uint32_t _tiWaitOnline = 0;

void setup()
{
  pinMode(A1, INPUT_PULLUP); // online-button

  Serial.begin(115200);
  bleSendLoc.Init();
  #ifndef  TXJETIEXBLE_OFFLINE_MODE
    bleSendLoc.Start();
  #endif 
  jetiDecode.Start();  // for devices with more than one UART (i.e. Teensy): jetiDecode.Start( TxJetiDecode::SERIAL1..3 );

  // test data 
  // _gpspos.lat = 48.24570f;
  // _gpspos.lon = 11.55616f;
  // bleSendLoc.SetPosition(_gpspos.lat, _gpspos.lon);
}

void loop()
{
  TxJetiExPacket * pPacket;

  if( ( pPacket = jetiDecode.GetPacket() ) != NULL ) 
  {
    switch( pPacket->GetPacketType() )
    {
    case TxJetiExPacket::PACKET_NAME:
      break;
    case TxJetiExPacket::PACKET_LABEL:
      break;
    case TxJetiExPacket::PACKET_VALUE:
      SendValue( (TxJetiExPacketValue *)pPacket );
      break;
    case TxJetiExPacket::PACKET_ERROR:
      break;
    }
  }
  
  bleSendLoc.doSend();
  doConnectionState();
}

void SendValue( TxJetiExPacketValue * pValue)
{
  float fValue = 0.0;
  if( pValue->GetLatitude( &fValue ) && fValue != 0.0f )
    _gpspos.lat = fValue;
  else if( pValue->GetLongitude( &fValue ) && fValue != 0.0f )
    _gpspos.lon = fValue;

  // Serial.printf( "%f %f\n", _gpspos.lat, _gpspos.lon);

  if(_gpspos.lat != 0.0f && _gpspos.lon != 0.0f )
    bleSendLoc.SetPosition(_gpspos.lat, _gpspos.lon );
} 

void doConnectionState()
{
  static unsigned long lastMillis = 0;
  if( millis() > (lastMillis + 2000) )
  {
    #ifdef  TXJETIEXBLE_OFFLINE_MODE
      // check online button
      int bt = digitalRead(A1);

      if (_enState == EN_OFFLINE && bt == 0)
      {
        _enState = EN_WAITONLINE;
        _tiWaitOnline = millis() + 60000; // 1 minute for the central to connect
        bleSendLoc.Start();
      }

      // timeout waiting for connection
      if (_enState == EN_WAITONLINE && _tiWaitOnline && millis() > _tiWaitOnline )
      {
        _enState = EN_OFFLINE;
        _tiWaitOnline = 0;
        bleSendLoc.Stop();
      }

      // waiting for online
      if (_enState == EN_WAITONLINE && bleSendLoc.IsConnected() )
        _enState = EN_ONLINE;

      // diconnected by central
      if (_enState == EN_ONLINE && !bleSendLoc.IsConnected() )
      {
        _enState = EN_OFFLINE;
        bleSendLoc.Stop();
      }
    #endif

    Serial.printf( "Tx power: %d,, Connected: %d, Button: %d\n", bleSendLoc.GetTxDefaultPower(), bleSendLoc.IsConnected(), bt );
    lastMillis = millis();
  }
}

