/*

  TxJetiExBLESendLoc.cpp - Send a GPS position via BLE

  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.94   12/09/2017  created

**************************************************************/

#include "TxJetiExBLESendLoc.h"

static TxJetiExBLESendLoc * _pInstance = NULL; // single instance only 

class TxJetiExBLESendServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer)    { _pInstance->m_bDeviceConnected = true;  };
    void onDisconnect(BLEServer* pServer) { _pInstance->m_bDeviceConnected = false; }
};

class TxJetiExBLESendCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0)
      {
        /*
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]); */
      }
    }
};

TxJetiExBLESendLoc::TxJetiExBLESendLoc() : 
  m_pServer( NULL ), m_pCharacteristic( NULL ), m_bDeviceConnected( false ), m_bOldDeviceConnected( false ),
  m_bActive(false), m_tiScheduleAdvertising( 0 ), m_tiScheduleNotify( 0 )
{
  _pInstance = this;
}

void TxJetiExBLESendLoc::Init(TXJETIEX_BLE_POWER_LEVEL pwrLevel)
{
  // Create the BLE Device
  BLEDevice::init("Jeti Tx Loc");

  // Create the BLE Server
  m_pServer = BLEDevice::createServer();
  m_pServer->setCallbacks(new TxJetiExBLESendServerCallbacks());

  // Create the BLE Service
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.location_and_navigation.xml
  BLEService * pService = m_pServer->createService((uint16_t)ESP_GATT_UUID_LOCATION_AND_NAVIGATION_SVC); // 0x1819 

  // Create a BLE Characteristic
  // 0x2a6a ln feature mask - https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.ln_feature.xml
  BLECharacteristic * pCharacteristicFeature = pService->createCharacteristic((uint16_t)0x2a6a, BLECharacteristic::PROPERTY_READ); // feature mask
  uint32_t mask = 4L; // "position" present
  pCharacteristicFeature->setValue((uint8_t*)&mask, 4);

  // 0x2a67 location - https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.location_and_speed.xml
  m_pCharacteristic = pService->createCharacteristic((uint16_t)0x2a67, BLECharacteristic::PROPERTY_NOTIFY); // position
  m_pCharacteristic->addDescriptor(new BLE2902());

  m_pCharacteristic->setCallbacks(new TxJetiExBLESendCharacteristicCallbacks());

  // get informed about connect/disconnect
  // set TX default power level
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, (esp_power_level_t)pwrLevel /*ESP_PWR_LVL_N14*/); // ret == ESP_OK ?

  // start service
  pService->start();
}

void TxJetiExBLESendLoc::Start()
{
  // Start 
  m_pServer->getAdvertising()->start();
  m_bActive = true;
  Serial.println("Started advertising, waiting a client connection to notify...");
}

void TxJetiExBLESendLoc::Stop()
{
  // Stop everything 
  m_pServer->getAdvertising()->stop();
  m_bActive = false;
  Serial.println("Stopped advertising and service...");
}


// send engine - must be called from loop() as often as possible
////////////////////////////////////////////////////////////////
void TxJetiExBLESendLoc::doSend()
{
  if (millis() > m_tiScheduleNotify)
  {
    if (m_bDeviceConnected)
    {
      // Serial.printf( "%ld %ld\n", m_loc.GetLat(), m_loc.GetLon() );
      m_pCharacteristic->setValue( m_loc.GetBuffer(), m_loc.GetSize() );
      if (m_bActive)
        m_pCharacteristic->notify();
    }
    m_tiScheduleNotify = millis() + 1000; // update every second
  }

  // disconnecting
  if( !m_bDeviceConnected && m_bOldDeviceConnected )
  {
    m_tiScheduleAdvertising = millis() + 500;
    m_bOldDeviceConnected = m_bDeviceConnected;
  }
  // connecting
  if( m_bDeviceConnected && !m_bOldDeviceConnected )
  {
    m_tiScheduleAdvertising = 0;
    m_bOldDeviceConnected = m_bDeviceConnected;
  }
  // start advertising
  if( m_tiScheduleAdvertising && millis() > m_tiScheduleAdvertising )
  {
    m_tiScheduleAdvertising = 0;
    if( m_bActive )
      m_pServer->startAdvertising(); 
  }
}

int TxJetiExBLESendLoc::GetTxDefaultPower()
{
  return (int)esp_ble_tx_power_get( ESP_BLE_PWR_TYPE_DEFAULT );
}

void LocationAndSpeed::SetPosition(float lat, float lon)
{
  m_data.flags = 0x0084; // "position present" and "position status=OK"
  m_data.lat = (int32_t)(lat * 10000000.0f);
  m_data.lon = (int32_t)(lon * 10000000.0f);
}
