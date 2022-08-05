/* project k0uchpad. Q30 Keyboard with nRF51822
   Coded by TinLethax 2021/08/05 +7
*/
 
#ifndef BLE_TOUCH
#define BLE_TOUCH

#include <Arduino.h>

#include <BLECharacteristic.h>
#include <BLEHIDReportReferenceDescriptor.h>
#include <BLEHID.h>

// Digitizer report for Pen side, Eraser Side and barrel buttons
#define DIGIT_PEN_TIP_INRANGE 	0x20 // Pen side is hovering. 
#define DIGIT_PEN_TIP_DOWN		0x21 // Pen side touch the digitizer surface.
#define DIGIT_ERASER_INRANGE	0x22 // Eraser side is hovering.
#define DIGIT_ERASER_DOWN		0x2A // Eraser side touch the digitizer surface.

#define DIGIT_BTN1 				0x04 // First Barrel Button is pressed.
#define DIGIT_BTN2				0x10 // Second Barrel Button is pressed.

class BLETouch : public BLEHID
{
  public:
    BLETouch();

	void MaxTchCntReport();
	
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
    BLEHIDReportReferenceDescriptor   _reportReferenceDescriptor;
    BLEHIDReportReferenceDescriptor   _reportReferenceDescriptor2;
};

#endif
