#ifndef MAX17050_H
#define MAX17050_H

#include <stdint.h>
#include <Wire.h>
#include <arduino.h>
#include <stdint.h>

// I/O
class MAX17050
{
protected:
TwoWire *i2cwire;
void max17050_write_reg(uint8_t reg, uint16_t value);
uint16_t max17050_read_reg(uint8_t reg);
uint8_t max17050_write_verify_reg(uint8_t reg, uint16_t value);
uint8_t max17050_probe();

public:

uint8_t max17050_init();

uint16_t max17050_voltage();
int16_t max17050_current();
uint8_t max17050_soc();
uint16_t max17050_cycles();
uint16_t max17050_fullcap();
uint16_t max17050_remaincap();
};
#endif
