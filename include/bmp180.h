#ifndef BMP180_H
#define BMP180_H

#include <Arduino.h>
#include <Wire.h>

// BMP180 I2C address
#define BMP180_ADDR 0x77

// BMP180 register addresses
#define BMP180_REG_CONTROL 0xF4
#define BMP180_REG_RESULT 0xF6
#define BMP180_REG_CAL_AC1 0xAA
#define BMP180_REG_CAL_AC2 0xAC
#define BMP180_REG_CAL_AC3 0xAE
#define BMP180_REG_CAL_AC4 0xB0
#define BMP180_REG_CAL_AC5 0xB2
#define BMP180_REG_CAL_AC6 0xB4
#define BMP180_REG_CAL_B1 0xB6
#define BMP180_REG_CAL_B2 0xB8
#define BMP180_REG_CAL_MB 0xBA
#define BMP180_REG_CAL_MC 0xBC
#define BMP180_REG_CAL_MD 0xBE

// BMP180 commands
#define BMP180_CMD_TEMP 0x2E
#define BMP180_CMD_PRESS0 0x34  // OSS = 0
#define BMP180_CMD_PRESS1 0x74  // OSS = 1
#define BMP180_CMD_PRESS2 0xB4  // OSS = 2
#define BMP180_CMD_PRESS3 0xF4  // OSS = 3

// Oversampling settings
#define BMP180_OSS_ULTRALOWPOWER 0
#define BMP180_OSS_STANDARD 1
#define BMP180_OSS_HIGHRES 2
#define BMP180_OSS_ULTRAHIGHRES 3

class BMP180 {
private:
    TwoWire* _wire;
    uint8_t _oss;  // Oversampling setting
    
    // Calibration coefficients
    int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
    uint16_t ac4, ac5, ac6;
    
    // Private methods
    bool readCalibrationData();
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister8(uint8_t reg);
    uint16_t readRegister16(uint8_t reg);
    int32_t readRawTemperature();
    int32_t readRawPressure();
    int32_t computeB5(int32_t UT);

public:
    // Constructor
    BMP180();
    bool begin(TwoWire* wire, uint8_t oss = BMP180_OSS_ULTRAHIGHRES);
    bool isConnected();
    
    float readTemperature();
    float readPressure();
    float readAltitude(float seaLevelPressure = 101325.0);
    
    // Raw data access
    int32_t getRawTemperature();
    int32_t getRawPressure();
};

#endif