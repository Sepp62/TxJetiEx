
/* 
  Jeti Transmitter EX Telemetry C++ Library
  
  TxJetiEx.ino - Example printing telemetry data to Serial in JSON format
                 Use with Teensy 3.x

                 For Teensy: uncomment #define SERIAL_9BIT_SUPPORT in
                 ...\Arduino\hardware\teensy\avr\cores\teensy3\HardwareSerial.h
  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.90   12/04/2017  created

  Needs:
  ArduinoJson - https://github.com/bblanchon/ArduinoJson

**************************************************************/

#include <ArduinoJson.h>
#include "TxJetiExDecode.h"

TxJetiDecode jetiDecode;

void setup()
{
  // Resend sensor data on Serial 3
  Serial3.begin(9600);
  Serial3.setTimeout(1000);

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
  json.printTo(Serial3);
  Serial3.print("\n");
}

void ReceiveData()
{
  // receive test with Serial 3 - connect pin 7 and 8 on teensy
	char buf[150];
  StaticJsonBuffer<150> jsonBuffer;
  if( Serial3.available() )
	{
    int len;
		if( ( len = Serial3.readBytesUntil( '\n', buf, sizeof(buf) ) ) )
		{
      buf[len] = '\0';
      // Serial.printf("buf: %s\n", buf );
      JsonObject& root = jsonBuffer.parseObject(buf);
      if( root.success() )
      {
        int type = root["t"];
        // Serial.printf("type: %d\n", type);
        if (type == 2)
        {
          const char *  name = root["n"];
          const char *  label = root["l"];
          const char *  unit = root["u"];
          unsigned long serialId = root["s"];
          int           id = root["i"];
          int           exType = root["x"];
          float         value = root["v"];
          Serial.printf("Serial: %lx\n", serialId );
          Serial.printf("Name: %s\n", name );
          Serial.printf("Id: %d\n", id );
          Serial.printf("Label: %s\n", label );
          Serial.printf("Unit: %s\n", unit );
          Serial.printf("EX type: %d\n", exType );
          if (exType != 5)
          {
            Serial.printf("value: %f\n", value);
          }
          else
          {
            const char * dateTime = root["v"];
            Serial.printf("value: %s\n", dateTime );
          }
          Serial.println();
        }
      }
      else
        Serial.println( "parse error" );
    }
  }
}
