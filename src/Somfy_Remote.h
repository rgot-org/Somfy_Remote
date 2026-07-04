/*
This library is based on the Arduino sketch by Nickduino: https://github.com/Nickduino/Somfy_Remote
*/

#ifndef SOMFY_REMOTE
#define SOMFY_REMOTE

#include <EEPROM.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

class SomfyRemote
{
private:
  String _name;
  uint32_t _remoteCode;
  uint32_t _rollingCode;
  uint16_t _eepromAddress;

  void buildFrame(uint8_t *frame, uint8_t command);
  void sendCommand(uint8_t *frame, uint8_t sync);
  void sendBit(bool value);
  uint16_t getNextEepromAddress();
  void getRollingCode();

public:
  SomfyRemote(String name, uint32_t remoteCode);
  String getName();
  void move(String command); // UP, DOWN, MY, PROGRAM

  // Doit être appelé une fois dans setup() avant le premier move()
  // sck, miso, mosi, cs : broches SPI
  // gdo0 : broche GDO0 du CC1101 (entrée données TX en mode async)
  static void begin(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs, uint8_t gdo0);
};
#endif
