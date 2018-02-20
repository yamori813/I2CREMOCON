/* I2CREMOCON Test probram on Arduino */

#include <Wire.h>

#define I2CADDR 4

void setup()
{
  Wire.begin();
}

void loop()
{
  // Panasonic fluorescent ceiling light 2CH(AEHA)
  Wire.beginTransmission(I2CADDR);
  Wire.write(0xa1);
  Wire.write(0x34);
  Wire.write(0x4a);
  Wire.write(0x90);
  Wire.write(0x0c);
  Wire.write(0x9c);
  Wire.endTransmission();
  delay(5000);

  // AppleRemote menu(NEC)
  Wire.beginTransmission(I2CADDR);
  Wire.write(0x82);
  Wire.write(0x77);
  Wire.write(0xe1);
  Wire.write(0xc0);
  Wire.write(0xe9);
  Wire.endTransmission();
  delay(5000);

  // Sony CD play
  Wire.beginTransmission(I2CADDR);
  Wire.write(0x33);
  Wire.write(0x4d);
  Wire.write(0x10);
  Wire.endTransmission();
  delay(5000);

}

