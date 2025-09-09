#ifndef SHT30_H
#define SHT30_H

#include <Wire.h>

class SHT30 {
public:
  SHT30();
  bool begin(TwoWire *wire);
  bool read();
  float getTemperature();
  float getHumidity();

private:
  TwoWire *_wire;
  float _temperature;
  float _humidity;
};

#endif