/*

  TxJetiExBLESendLoc.h - Send position via BLE 

  -------------------------------------------------------------
  
  Copyright (C) 2017 Bernd Wokoeck
  
  Version history:
  0.94   12/16/2017  created

**************************************************************/

#ifndef TXJETIEXBLESENDLOC_H
#define TXJETIEXBLESENDLOC_H

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

class LocationAndSpeed
{
public:
  LocationAndSpeed() { Clear(); }
  void Clear() { m_data.flags = 0x0004;  m_data.lat = 0L; m_data.lon = 0L; }
  bool IsEmpty() { return m_data.lat != 0 && m_data.lon != 0; }
  void SetPosition(float lat, float lon);
  
  uint8_t * GetBuffer() { return (uint8_t *)&m_data; }
  size_t    GetSize() { return sizeof(m_data); }

  int32_t GetLat() { return m_data.lat; }
  int32_t GetLon() { return m_data.lon; }

protected:
#pragma pack(push, 1 )
  struct location_and_speed
  {
    int16_t flags; // 1=speed, 2=distance, *** 4=location ***, 8=elevation, 0x10=heading, 0x20=rolling, 0x40=time, 0x80/0x100=posStatus, 0x200=speedDistFormat, 0x400/0x800=eleSource, 0x1000=hdgSource, 0x2000/0x4000/0x8000 reserved
    // uint16_t speed;
    // uint8_t  totDist[3];
    int32_t lat;
    int32_t lon;
    // uint8_t  elevation[3];
    // uint16_t heading;
    // uint8_t  rollTime;
    // struct time
  }
  m_data;
#pragma pack(pop) 
};

class TxJetiExBLESendLoc
{
  friend class TxJetiExBLESendServerCallbacks;
  friend class TxJetiExBLESendCharacteristicCallbacks;
public:
  TxJetiExBLESendLoc();

  void Init( TXJETIEX_BLE_POWER_LEVEL pwrLevel = Minus14dbm );
  void Start();
  void Stop();
  bool IsConnected() { return m_bDeviceConnected; }
  int  GetTxDefaultPower();
  void SetPosition(float lat, float lon) { m_loc.SetPosition(lat, lon); }

  void doSend();

protected: 

  LocationAndSpeed    m_loc;

  BLEServer         * m_pServer;
  BLECharacteristic * m_pCharacteristic;
  volatile bool       m_bDeviceConnected;
  bool                m_bOldDeviceConnected;
  bool                m_bActive;  
  unsigned long       m_tiScheduleAdvertising;
  unsigned long       m_tiScheduleNotify;
};

#endif  // TXJETIEXBLESENDLOC_H