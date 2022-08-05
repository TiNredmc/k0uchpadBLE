/* project k0uchpad. Q30 Keyboard with nRF51822
   Coded by TinLethax 2021/08/05 +7
*/

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEHIDPeripheral.h>
#include "BLETouch.h"
#include "MAX17050.h"
/*  Update the min_conn_interval and max_conn_interval in BLEPeripheral-library-folder\src\nRF51822.cpp
    gap_conn_params.min_conn_interval = 20;  // in 1.25ms units
    gap_conn_params.max_conn_interval = 40;  // in 1.25ms unit
*/

#include <Wire.h>

/* I2C 7bit address of S3501 */
#define S3501_ADDR  0x20

/* Register map*/
#define RMI4_F01  0x01 // Device control
#define RMI4_F12  0x12 // 2D Touch sensor (newer gen of Synaptics use this instead of F11).
#define RMI4_F1A  0x1A // Button 
#define RMI4_F54  0x54 // Test report
#define RMI4_F55  0x55 // TX, RX configuration

#define SYNA_INT 10 // P5 (Chip pin is pin 3) as Synaptics Interrupt (but use for polling).
#define SYNA_RST  9 // P6 (Chip pin is pin 4) as Synaptics Reset pin.
#define LED_stat 4 // P8 (Chip pin is pin 10) as LED backlight / Status LED.
#define KB_INT  13  // P9 (Chip pin is pin 13) as STM8L Keyboard scanner Interrupt.

// Save current page
uint8_t current_page = 0;
// Address of each function.
//Page 0
uint8_t F01_addr = 0;
uint8_t F12_addr = 0;
//static uint8_t F34_addr = 0;// Firmware stuffs.
// Page 1
//static uint8_t F54_addr = 0;
// Page 2
uint8_t F1A_addr = 0;
// Page 3
//static uint8_t F55_addr = 0;

// 2D sensor params of F12
uint8_t F12_maxfinger = 0;// store maximum finger supported by the chip
uint8_t F12_report_addr = 0;// Store ACTUAL HID data report address of F12.

// General purpose array for reading various report (Query, Control, HID data).
// Share same variable to save RAM
uint8_t report[90];

// for fuel gauge status
uint8_t gauge_failed = 0;// if probing is failed, this will set to 1 and will avoid atempting to read from fuel gauge.

// for heartbeat led
unsigned long prev_mil = 0;

// define pins (varies per shield/board)
#define BLE_REQ   6
#define BLE_RDY   7
#define BLE_RST   5

BLEHIDPeripheral bleHID = BLEHIDPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
BLETouch HIDd;

// Thanks https://github.com/kriswiner/nRF52832DevBoard/blob/master/BMP280_nRF52.ino https://developer.apple.com/forums/thread/77866
// for the battery report example.
// Battery Service
BLEService batteryService = BLEService("180F");
BLEUnsignedCharCharacteristic battlevelCharacteristic = BLEUnsignedCharCharacteristic("2A19", BLERead | BLENotify); // battery level is uint8_t
BLEDescriptor battlevelDescriptor = BLEDescriptor("2901", "Battery Level 0 - 100");
MAX17050 FuelGauge;

// Set page of S3501
void s3501_setPage(uint8_t page_num) {
  if (page_num != current_page) {
    current_page = page_num;
    Wire.beginTransmission(S3501_ADDR);// Begin tx

    Wire.write(0xFF);// Write lower half byte which is dummy
    Wire.write(page_num);// Write upper half which is page number.

    Wire.endTransmission();// End tx
  }
}

// Read data from S3501
void s3501_read(uint8_t addr, uint8_t *data, uint8_t len) {

  Wire.beginTransmission(S3501_ADDR);// Begin tx

  Wire.write(addr);// Write register address to S3501.
  Wire.endTransmission();// End tx

  Wire.requestFrom(S3501_ADDR, len);// Read request.
  while (Wire.available() && len--) {
    *data++ = Wire.read();
  }
 
}

// Write data to S3501
void s3501_write(uint8_t addr, uint8_t *data, uint8_t len) {

  Wire.beginTransmission(S3501_ADDR);// Begin tx
  Wire.write(addr);

  while (len--)
    Wire.write(*data++);

  Wire.endTransmission();// End tx
}

// Query process. Call once every power up or reset to locate all RMI4 function address.
// Static uint8_t variables F01_addr and F12_addr for further use of page address of F01 and F12.
// return 0 when no error.
uint8_t s3501_query() {
  uint8_t s3501_desc[6] = {0};

  // Scan for base address of each function.
  for (uint8_t p = 0; p < 4; p++) {
    s3501_setPage(p);// set page
    for (uint8_t i = 0xE9; i > 0xD0; i -= 6) {

      s3501_read(i, s3501_desc, 6);
      // Detect RMI4 function
      switch (s3501_desc[5]) {
        case RMI4_F01: // Function 0x01, RMI device control.
          F01_addr = i;// save F01 address.
          break;

        case RMI4_F12: // Function 0x12, 2-D sensor.
          F12_addr = i;// save F12 address.
          F12_report_addr = s3501_desc[3];// save the F12 data report address. This will be the actual address that we read from (mine is 0x06).
          break;

        case RMI4_F1A: // Function 0x1A
          F1A_addr = i;
          break;

        /*case 0x54: // Function 0x54
          F54_addr = i;
          break;*/

        default:
          break;

      }
    }
  }
  // Sanity check
  if (F01_addr == 0)
    return 1;

  if (F12_addr == 0)
    return 2;

  s3501_setPage(0);// Set to Page 0
  s3501_read(F01_addr, report, 6);// Retrieve Description Table of F01
  s3501_read(report[0], report, 4);// Read from Query register of F01

  if (report[0] != 1) { // Check for manufacturer id (always 1)
    return 3;
  }

  return 0;

}

// Get HID report data (X,Y coordinates and other stuffs).
// Assuming that it has 2 fingers report (I guess).
void s3501_HIDreport() {
  // HID report (Capable upto 10 fingers, But due to the suraface area.
  // It doesn't make any sense to report more than 2 fingers but the data is there).
  // That's why the data report size is 88 bytes, 8*10 + 8 byte.

  // Finger 1
  // report[0] -> Object present (0x01 == Finger)
  // report[1] -> ABS_X LSB
  // report[2] -> ABS_X MSB
  // report[3] -> ABS_Y LSB
  // report[4] -> ABS_Y MSB
  // report[5] -> pressure
  // report[6] -> ABS_MT_TOUCH_MINOR
  // report[7] -> ABS_MT_TOUCH_MAJOR

  // Finger 2
  // report[8] -> Object present (0x01 == Finger)
  // report[9] -> ABS_X LSB
  // report[10] -> ABS_X MSB
  // report[11] -> ABS_Y LSB
  // report[12] -> ABS_Y MSB
  // report[13] -> pressure
  // report[14] -> ABS_MT_TOUCH_MINOR
  // report[15] -> ABS_MT_TOUCH_MAJOR

  //.
  //.
  //.
  // Finger 10

  // report[16] to report[85] are all at 0x00

  // report[86] -> BTN_TOUCH single finger == 1, 2 fingers == 3
  // report[87] is 0x01

  s3501_setPage(0);// Set page 0 to get data from F12.
  s3501_read(F12_report_addr-1, report, 1);// Read from Data register of F12
  s3501_read(F12_report_addr, report, 88);// Read from Data register of F12
}

uint8_t err = 0;
void setup() {
  pinMode(SYNA_INT, INPUT_PULLUP);
  pinMode(SYNA_RST, OUTPUT);
  pinMode(LED_stat, OUTPUT);

  digitalWrite(SYNA_RST, HIGH);

  digitalWrite(LED_stat, HIGH);
  delay(100);
  digitalWrite(LED_stat, LOW);
  delay(100);

  Wire.setPins(2, 1);// set SDA, SCL pin (module pin : P4 and P3)
  Wire.setClock(100000);// Set I2C speed to 100kHz
  Wire.begin();

  err = s3501_query();
  if (err != 0) { // LED blink when detect probing error
    // Delay lenght between quick blink indicates error number
    // 1sec = error 1 = F01 not found
    // 2sec = error 2 = F12 not found
    // 3sec = error 3 = Manufacturer report not found
    while (1) {
      digitalWrite(LED_stat, HIGH);
      delay(5);
      digitalWrite(LED_stat, LOW);
      delay(err * 1000);
    }
  }

  // fuel gauge probing
//  gauge_failed = 1;
//  if (FuelGauge.max17050_init()) {
//    // slow blink to indicates fuel gauge probing error and then continue
//    digitalWrite(LED_stat, HIGH);
//    delay(500);
//    digitalWrite(LED_stat, LOW);
//    delay(500);
//    digitalWrite(LED_stat, HIGH);
//    delay(500);
//    digitalWrite(LED_stat, LOW);
//    delay(500);
//    gauge_failed = 1;
//  } else {
//    gauge_failed = 0;
//  }

  // Battery service
  bleHID.setAdvertisedServiceUuid(batteryService.uuid());
  bleHID.addAttribute(batteryService);
  bleHID.addAttribute(battlevelCharacteristic);
  bleHID.addAttribute(battlevelDescriptor);

  // clears bond data on every boot
  bleHID.clearBondStoreData();
  // Set Bluetooth name
  bleHID.setDeviceName("k0uchpad BLE");
  // Set local name as HID
  bleHID.setLocalName("k0uchpad BLE");
  // Add HID device (BLETouch)
  bleHID.addHID(HIDd);
  // Init the Bluetooth HID
  bleHID.begin();

}

void loop() {
  BLECentral central = bleHID.central();
  if (central) {
    // central connected to peripheral
    //digitalWrite(LED_stat, HIGH);
    if (central.connected())
      HIDd.MaxTchCntReport();
    while (central.connected()) {
      // Report touch data
      if(digitalRead(SYNA_INT) == 0)
        s3501_HIDreport();
      HIDd.TouchReport(
        // Finger 1
        report[0],// Finger 1 present
        report[5],// Pressure
        report[1],// X LSB
        report[2],// X MSB
        report[3],// Y LSB
        report[4],// Y MSB

        // Finger 2
        report[8],// Finger 2 present
        report[13],// Pressure
        report[9],// X LSB
        report[10],// X MSB
        report[11],// Y LSB
        report[12]// Y MSB
      );// Send Bluetooth HID report

      //delay(9);// delay 1ms, throttle down the crazy polling rate.

      // Heartbeat LED.
      if ((millis() - prev_mil) > 1000) {
        prev_mil = millis();
        digitalWrite(LED_stat, HIGH);
        delay(1);
        digitalWrite(LED_stat, LOW);
        // Report battery level every 2 seconds.
        battlevelCharacteristic.setValue(100);
      }

    }// while centril is connected.

    bleHID.clearBondStoreData();

    if ((millis() - prev_mil) > 1000) {
      prev_mil = millis();
      digitalWrite(LED_stat, HIGH);
      delay(1);
      digitalWrite(LED_stat, LOW);
    }
  }

}
