/*

  TxJetiExBLESend.h - Send a value to BLE stream 

  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.94   11/16/2017  created

  Based on BLE_uart from Neil Kolban and Evandro Copercini

**************************************************************/

#ifndef TXJETIEXBLESEND_H
#define TXJETIEXBLESEND_H

// #define TXJETIEX_DECODE_DEBUG // dump raw data bufer and decoded values to Serial

#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include <bt.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

typedef enum
{
    Minus14dbm = 0,
    Minus11dbm = 1,
    Minus8dbm  = 2,
    Minus5dbm  = 3,
    Minus2dbm  = 4,
    Plus1dbm   = 5,
    Plus4dbm   = 6,
    Plus7dbm   = 7,
}
TXJETIEX_BLE_POWER_LEVEL;

class SimpleRingBuf // not thread_safe !
{
public:
  SimpleRingBuf( size_t nBytes );

  bool GetByte( uint8_t * pByte );
  bool PutByte( uint8_t c );

  size_t GetFreeBuffer() { return (m_bufSize - m_nChar); }
  void   Flush() { m_headPtr = m_tailPtr = m_pBuf; m_nChar = 0;  }

protected:
  uint8_t * m_pBuf; 
  uint8_t * m_headPtr;
  uint8_t * m_tailPtr;
  uint16_t  m_nChar;
  uint16_t  m_bufSize;
};


class TxJetiExBLESend : public Stream
{
  friend class TxJetiExBLESendServerCallbacks;
  friend class TxJetiExBLESendCharacteristicCallbacks;
public:
  TxJetiExBLESend();

  void init( size_t bufSize, TXJETIEX_BLE_POWER_LEVEL pwrLevel = Minus14dbm );
  bool isConnected() { return m_bDeviceConnected; }
  int  getTxDefaultPower();

  size_t getFreeBuffer() { if( m_pRingBuffer == NULL ) return 0;  return m_pRingBuffer->GetFreeBuffer(); }
  void   doSend();

  virtual size_t write(uint8_t);
  virtual int available() { return 0; }
  virtual int read() { return 0; }
  virtual int peek() { return 0; }
  virtual void flush() {  if( m_pRingBuffer != NULL ) m_pRingBuffer->Flush();  }

protected: 
  BLEServer         * m_pServer;
  BLECharacteristic * m_pCharacteristic;
  volatile bool       m_bDeviceConnected;
  bool                m_bOldDeviceConnected;
  unsigned long       m_tiScheduleAdvertising;

  static const char SERVICE_UUID[];
  static const char CHARACTERISTIC_UUID_RX[];
  static const char CHARACTERISTIC_UUID_TX[];

  SimpleRingBuf * m_pRingBuffer;
};

#endif 