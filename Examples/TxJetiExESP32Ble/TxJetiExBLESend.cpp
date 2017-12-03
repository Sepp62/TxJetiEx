/*

  TxJetiExBLESend.cpp - Send values to BLE stream 

  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.94   11/16/2017  created
                     BLE_ESP issue, see: https://github.com/nkolban/esp32-snippets/issues/209

  Based on BLE_uart from Neil Kolban and Evandro Copercini
  https://github.com/nkolban/esp32-snippets/tree/master/cpp_utils/tests/BLETests/Arduino

  BLE GATT tutorial:
  https://devzone.nordicsemi.com/tutorials/17/

**************************************************************/

#include "TxJetiExBLESend.h"

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
//////////////////////////////////////////
const char TxJetiExBLESend::SERVICE_UUID[]           = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"; // UART service UUID
const char TxJetiExBLESend::CHARACTERISTIC_UUID_RX[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char TxJetiExBLESend::CHARACTERISTIC_UUID_TX[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

#define TXJETIEX_MAX_BLE_PACKETLEN 20 

static TxJetiExBLESend * _pInstance = NULL; // single instance only 

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

TxJetiExBLESend::TxJetiExBLESend() : m_pServer( NULL ), m_pCharacteristic( NULL ), m_bDeviceConnected( false ), m_bOldDeviceConnected( false ), m_tiScheduleAdvertising( 0 ), m_pRingBuffer( NULL )
{
  _pInstance = this;
}

void TxJetiExBLESend::init( size_t bufSize, TXJETIEX_BLE_POWER_LEVEL pwrLevel )
{
  // create ring buffer
  m_pRingBuffer = new SimpleRingBuf( bufSize );

  // Create the BLE Device
  BLEDevice::init("Jeti Tx");

  // Create the BLE Server
  m_pServer = BLEDevice::createServer();
  m_pServer->setCallbacks( new TxJetiExBLESendServerCallbacks() );

  // Create the BLE Service
  BLEService *pService = m_pServer->createService( SERVICE_UUID );

  // Create a BLE Characteristic
  m_pCharacteristic = pService->createCharacteristic( CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY );
  m_pCharacteristic->addDescriptor( new BLE2902() );

  BLECharacteristic *pCharacteristic = pService->createCharacteristic( CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE );
  pCharacteristic->setCallbacks( new TxJetiExBLESendCharacteristicCallbacks() );

  // set TX default power level
  esp_ble_tx_power_set( ESP_BLE_PWR_TYPE_DEFAULT, (esp_power_level_t)pwrLevel /*ESP_PWR_LVL_N14*/ ); // ret == ESP_OK ?

  // Start the service
  pService->start();

  // Start advertising
  m_pServer->getAdvertising()->start();
  // Serial.println("Waiting a client connection to notify...");
}

// send engine - must be called from loop() as often as possible
////////////////////////////////////////////////////////////////
void TxJetiExBLESend::doSend()
{
  // https://punchthrough.com/blog/posts/maximizing-ble-throughput-on-ios-and-android
  if( m_pCharacteristic->isReadyForData()  )
  {
    // Serial.println( "ready" );
    uint8_t buf[ TXJETIEX_MAX_BLE_PACKETLEN ];
    int i = 0;
    while( m_pRingBuffer->GetByte( &buf[i] ) )
    {
      // Serial.println( "got" );
      if( ++i >= TXJETIEX_MAX_BLE_PACKETLEN )
        break;
    }

    if( i > 0 )
    {
      // Serial.printf( "notified: %d\n", i ); 
      m_pCharacteristic->setValue( buf, i );
      m_pCharacteristic->notify();
      delay( 10 ); // todo, das geht besser
    }
  }

  // disconnecting
  if( !m_bDeviceConnected && m_bOldDeviceConnected )
  {
    flush();
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
    m_pServer->startAdvertising(); 
  }

}

// Stream write function
size_t TxJetiExBLESend::write( uint8_t c )
{
  if( m_bDeviceConnected )
  {
    if( m_pRingBuffer->PutByte( c ) ) 
      return 1;
    // else
    //   Serial.println( "buf overflow" ); 
  }

  return 0;
}

int TxJetiExBLESend::getTxDefaultPower()
{
  return (int)esp_ble_tx_power_get( ESP_BLE_PWR_TYPE_DEFAULT );
}

// Simple ring buffer - NOT thread safe
///////////////////////////////////////
SimpleRingBuf::SimpleRingBuf( size_t nBytes ) : m_nChar( 0 )
{
  m_pBuf = new uint8_t[ nBytes ];
  m_bufSize = nBytes;
  m_headPtr = m_tailPtr = m_pBuf;
}

bool SimpleRingBuf::GetByte( uint8_t * pByte )
{
  if( m_nChar )
  {
    *pByte = *(m_tailPtr++);
    m_nChar--; 
    if( m_tailPtr >= ( m_pBuf + m_bufSize ) )
      m_tailPtr = m_pBuf;
    return true;
  }
  return false;
}

bool SimpleRingBuf::PutByte( uint8_t c )
{
  // Serial.printf( "nchar %d, bufSize %d\n", m_nChar, m_bufSize );
  if( m_nChar < m_bufSize )
  {
    *(m_headPtr++) = c;
    m_nChar++;
    if( m_headPtr >= ( m_pBuf + m_bufSize ) )
      m_headPtr = m_pBuf;
    return true;
  }
  return false; // buffer overflow
}

