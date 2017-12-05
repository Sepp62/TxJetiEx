
/* 
  Jeti Transmitter EX Telemetry C++ Library
  
  TxJetiEx.ino - Example printing telemetry data to Adafruit BLE UART friend in CMD mode
                 Place UART friend switch to CMD mode !!!
                 Use with Teensy 3.x

                 For Teensy: uncomment #define SERIAL_9BIT_SUPPORT in
                 ...\Arduino\hardware\teensy\avr\cores\teensy3\HardwareSerial.h
  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.90   12/05/2017  created

  Needs:
  ArduinoJson - https://github.com/bblanchon/ArduinoJson
  nRF51822-based Adafruit Bluefruit LE modules
              - https://github.com/adafruit/Adafruit_BluefruitLE_nRF51

**************************************************************/

#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_UART.h>

#include <ArduinoJson.h>
#include "TxJetiExDecode.h"

TxJetiDecode jetiDecode;

// bluefruit config
#define FACTORYRESET_ENABLE 0  // needed only once !
#define BLUEFRUIT_UART_MODE_PIN 15
#define BLUEFRUIT_UART_CTS_PIN  14
#define BLUEFRUIT_UART_RTS_PIN  16
// Adafruit Bluefruit LE UART friend at Serial 3
Adafruit_BluefruitLE_UART ble( Serial3, BLUEFRUIT_UART_MODE_PIN, BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

void setup()
{
  // init bluefruit module
  BleInit();

  // receive Jeti data on Serial2
  Serial.begin(115200);
  jetiDecode.Start( TxJetiDecode::SERIAL2 );  // for devices with more than one UART (i.e. Teensy): jetiDecode.Start( TxJetiDecode::SERIAL1..3 );
}

void loop()
{
	TxJetiExPacket * pPacket;

	if ((pPacket = jetiDecode.GetPacket()) != NULL)
	{
		switch (pPacket->GetPacketType())
		{
		case TxJetiExPacket::PACKET_NAME:
			PrintJsonName((TxJetiExPacketName *)pPacket);
			break;
		case TxJetiExPacket::PACKET_LABEL:
			PrintJsonLabel((TxJetiExPacketLabel *)pPacket);
			break;
		case TxJetiExPacket::PACKET_VALUE:
			PrintJsonValue((TxJetiExPacketValue *)pPacket);
			break;
		case TxJetiExPacket::PACKET_ERROR:
			PrintJsonError();
			break;
		}
	}

  // receive test
  ReceiveData();
}

void PrintJsonName(TxJetiExPacketName * pName)
{
	StaticJsonBuffer<150> jsonBuf;
	JsonObject& root = jsonBuf.createObject();
	root["t"] = 0; // type "name"
	root["s"] = pName->GetSerialId();
	root["n"] = pName->GetName();
	SendJson(root);
}

void PrintJsonLabel(TxJetiExPacketLabel * pLabel)
{
	StaticJsonBuffer<150> jsonBuf;
	JsonObject& root = jsonBuf.createObject();
	root["t"] = 1; // type "label"
	root["s"] = pLabel->GetSerialId();
	root["n"] = pLabel->GetName();
	root["i"] = pLabel->GetId();
	root["l"] = pLabel->GetLabel();
	root["u"] = pLabel->GetUnit();
	SendJson(root);
}

void PrintJsonValue(TxJetiExPacketValue * pValue)
{
  // if (skipPacket(pValue->GetId())) // reduce amount of data by skipping values
  //  return;

	StaticJsonBuffer<200> jsonBuf;
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
	if (pValue->GetFloat(&fValue))
	{
		root["v"] = fValue;
	}
	else if (pValue->GetLatitude(&fValue))
	{
		sprintf(buf, "%2.5f", fValue);
		root["v"] = buf;
	}
	else if (pValue->GetLongitude(&fValue))
	{
		sprintf(buf, "%2.5f", fValue);
		root["v"] = buf;
	}
	else if (pValue->GetDate(&day, &month, &year))
	{
		sprintf(buf, "%d.%d.%d", day, month, year);
		root["v"] = buf;
	}
	else if (pValue->GetTime(&hour, &minute, &second))
	{
		sprintf(buf, "%d:%d:%d", hour, minute, second);
		root["v"] = buf;
	}
	else
		root["v"] = pValue->GetRawValue();
	SendJson(root);
}

void PrintJsonError()
{
	StaticJsonBuffer<150> jsonBuf;
	JsonObject& root = jsonBuf.createObject();
	root["t"] = 3; // type "error"
	root["e"] = "error";
	SendJson(root);
}

void SendJson(JsonObject& json)
{
  // wait for connection
  if (ble.isConnected())
  {
    ble.print("AT+BLEUARTTX=");
    json.printTo(ble);
    ble.print("\n");
    
    // check response stastus
    if( !ble.waitForOK() )
      Serial.println( "Failed to send" );
    Serial.print(".");
  }
  // json.printTo( Serial );
}

void ReceiveData()
{
  // todo 
  // Check for incoming characters from Bluefruit
  /*
  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0)
  {
    // no data
    return;
  }
  // Some data was found, its in the buffer
  Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
  ble.waitForOK();
  */
}

void BleInit()
{
  if (ble.begin())
  {
    if( FACTORYRESET_ENABLE )
    {
      // Perform a factory reset to make sure everything is in a known state */
      Serial.println( "Performing a factory reset: " );
      if( !ble.factoryReset() )
        Serial.println( "Couldn't factory reset" );
    }

    // Disable command echo from Bluefruit
    ble.echo(false);
    ble.info();
    // LED Activity command is only supported from 0.6.6
    if (ble.isVersionAtLeast((char*)"0.6.6"))
      ble.sendCommandCheckOK("AT+HWModeLED=MODE");
    // set power level
    ble.sendCommandCheckOK("AT+BLEPOWERLEVEL=-20");
  }
  else
    Serial.print("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?");
}

/*
bool skipPacket(int id)
{
  static char idCnt[32] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };

  if (--id >= 0 && id < 31)
  {
    idCnt[id]++;
    if( ( idCnt[id] % 3 ) == 0 ) // let every 3rd value pass
      return false;
  }

  return true;
}
*/