/* project k0uchpad. Q30 Keyboard with nRF51822
   Coded by TinLethax 2021/08/05 +7
*/

#ifndef BLE_TOUCH
#define BLE_TOUCH

#include <Arduino.h>

#include <BLECharacteristic.h>
#include <BLEHIDReportReferenceDescriptor.h>
#include <BLEHID.h>

class BLETouch : public BLEHID
{
  public:
    BLETouch();

    void MaxTchCntReport();

    void KeyReport(
      uint8_t key1,
      uint8_t key2,
      uint8_t key3,
      uint8_t key4,
      uint8_t key5,
      uint8_t key6);

    void TouchReport(
      uint8_t Finger1_present,
      uint8_t Finger1_pressure,
      uint8_t Finger1_XLSB,
      uint8_t Finger1_XMSB,
      uint8_t Finger1_YLSB,
      uint8_t Finger1_YMSB,

      uint8_t Finger2_present,
      uint8_t Finger2_pressure,
      uint8_t Finger2_XLSB,
      uint8_t Finger2_XMSB,
      uint8_t Finger2_YLSB,
      uint8_t Finger2_YMSB) ;

  protected:
    virtual void setReportId(unsigned char reportId);
    virtual unsigned char numAttributes();
    virtual BLELocalAttribute** attributes();

  private:
    BLECharacteristic                 _reportCharacteristic;
    BLECharacteristic                 _reportCharacteristic2;
    //BLECharacteristic                 _reportCharacteristic3;
    BLEHIDReportReferenceDescriptor   _reportReferenceDescriptor;
    BLEHIDReportReferenceDescriptor   _reportReferenceDescriptor2;
    //BLEHIDReportReferenceDescriptor   _reportReferenceDescriptor3;
};

#endif
