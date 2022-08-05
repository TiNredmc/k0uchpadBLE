// MAX17050 Driver. 
// Ported from https://android.googlesource.com/kernel/msm/+/android-msm-dory-3.10-kitkat-wear/drivers/power/max17050_battery.c
// Ported by TinLethax 2022/05/12 +7

#include "MAX17050.h"

// Device address (7-bit)
#define MAX17050_DEV_ADDR	0x36 //7-bit address.

#define CMD_STATUS	0x00 // Chip status.
#define CMD_VCELL	0x09 // Cell voltage.
#define CMD_CURRNT	0x0A // Current draw.
#define CMD_SOC		0x0D // State of charge (Battery percentage).
#define CMD_REMCAP	0x0F // Remaining Capacity.
#define CMD_FULLCAP	0x10 // Full capacity.
#define CMD_CYCLES	0x17 // Battery cycle.
#define CMD_CONF	0x1D // COnfig register.

#define CMD_VERSION  0x21 // Fuel gauge version

void MAX17050::max17050_write_reg(uint8_t reg, uint16_t value){
	i2cwire->beginTransmission(MAX17050_DEV_ADDR);
	i2cwire->write(reg);
	i2cwire->write((uint8_t)value);
	i2cwire->write((uint8_t)(value >> 8));
	i2cwire->endTransmission();
}

uint16_t MAX17050::max17050_read_reg(uint8_t reg){
	i2cwire->beginTransmission(MAX17050_DEV_ADDR);
	i2cwire->write(reg);
	i2cwire->endTransmission();
	
	i2cwire->requestFrom(MAX17050_DEV_ADDR, 2);
	return i2cwire->read() | (i2cwire->read() << 8);
}

uint8_t MAX17050::max17050_write_verify_reg(uint8_t reg, uint16_t value){
	uint16_t read;
	
	max17050_write_reg(reg, value);
	
	i2cwire->requestFrom(MAX17050_DEV_ADDR, 2);
	read = i2cwire->read() | (i2cwire->read() << 8);
	
	if(read != value)
		return 1;
	
	return 0;
	
}

uint8_t MAX17050::max17050_probe(){
  uint8_t returnValue = 1;

  i2cwire->beginTransmission(MAX17050_DEV_ADDR);
  uint8_t error = i2cwire->endTransmission(true);

  if (error == 0)
  {
    returnValue = true;
  }

  return returnValue;
}

uint8_t MAX17050::max17050_init(){
	// Probe for chip
  if(max17050_probe())
	  return 1;
  // Check device version
  if(max17050_read_reg(CMD_VERSION) == 0)
    return 1;

  return 0;
}

uint16_t MAX17050::max17050_voltage(){
	
	return (max17050_read_reg(CMD_VCELL) >> 3) * 625; // left shift by 3 equal to dividing with 8.
}

int16_t MAX17050::max17050_current(){
	int16_t real_current = 0;
	
	/*real_current = (int16_t)max17050_read_reg(CMD_CURRNT);
	real_current *= 1562500 / 10000;
	real_current *= -1;
	
	return real_current;*/
	
	return ( ((int16_t)max17050_read_reg(CMD_CURRNT)) * 1562500 / 10000) * -1;
}

uint16_t MAX17050::max17050_cycles(){
	
	return max17050_read_reg(CMD_CYCLES) / 100;
}

uint8_t MAX17050::max17050_soc(){
  
  return (uint8_t)(max17050_read_reg(CMD_SOC) >> 8);
}

uint16_t MAX17050::max17050_fullcap(){// Full capacity 
	
	return max17050_read_reg(CMD_FULLCAP) / 10000;
	
}

uint16_t MAX17050::max17050_remaincap(){// remain capacity
	
	return max17050_read_reg(CMD_REMCAP) / 10000;
}
