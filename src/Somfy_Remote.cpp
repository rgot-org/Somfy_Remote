/*
This library is based on the Arduino sketch by Nickduino: https://github.com/Nickduino/Somfy_Remote
*/

#include "Somfy_Remote.h"

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__)
  #define EEPROM_SIZE 1024
#elif defined(__AVR_ATmega2560__)
  #define EEPROM_SIZE 4096
#else // ESP32 / ESP8266
  #define EEPROM_SIZE 512
#endif

static const uint16_t symbol = 604;

// Broches configurables via begin()
static uint8_t s_gdo0 = 255;

uint8_t currentEppromAddress = 0;

// ---------------------------------------------------------------------------

void SomfyRemote::begin(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs, uint8_t gdo0)
{
  s_gdo0 = gdo0;
  ELECHOUSE_cc1101.setSpiPin(sck, miso, mosi, cs);
  ELECHOUSE_cc1101.setGDO(gdo0, gdo0); // gdo0 utilisé pour les deux (GDO2 non câblé)
}

// Constructor
SomfyRemote::SomfyRemote(String name, uint32_t remoteCode)
{
  _name = name;
  _remoteCode = remoteCode;
  _eepromAddress = getNextEepromAddress();
}

String SomfyRemote::getName()
{
  return _name;
}

uint16_t SomfyRemote::getNextEepromAddress()
{
  uint8_t addr = currentEppromAddress;
  currentEppromAddress += 4;
  return addr;
}

void SomfyRemote::getRollingCode()
{
  if (EEPROM.get(_eepromAddress, _rollingCode) < 1)
  {
    _rollingCode = 1;
  }
}

void SomfyRemote::move(String command)
{
  const uint8_t my   = 0x1;
  const uint8_t up   = 0x2;
  const uint8_t down = 0x4;
  const uint8_t prog = 0x8;

  uint8_t frame[7];

  EEPROM.begin(EEPROM_SIZE);
  getRollingCode();

  command.toUpperCase();

  switch (command[0])
  {
    case 'U': buildFrame(frame, up);   break;
    case 'D': buildFrame(frame, down); break;
    case 'M': buildFrame(frame, my);   break;
    case 'P': buildFrame(frame, prog); break;
    default:  buildFrame(frame, my);   break;
  }

  sendCommand(frame, 2);
  for (int i = 0; i < 2; i++)
  {
    sendCommand(frame, 7);
  }

  EEPROM.commit();
}

void SomfyRemote::buildFrame(uint8_t *frame, uint8_t command)
{
  uint8_t checksum = 0;

  frame[0] = 0xA7;
  frame[1] = command << 4;
  frame[2] = _rollingCode >> 8;
  frame[3] = _rollingCode;
  frame[4] = _remoteCode >> 16;
  frame[5] = _remoteCode >> 8;
  frame[6] = _remoteCode;

  for (uint8_t i = 0; i < 7; i++)
  {
    checksum ^= frame[i] ^ (frame[i] >> 4);
  }
  frame[1] |= (checksum & 0x0F);

  for (uint8_t i = 1; i < 7; i++)
  {
    frame[i] ^= frame[i - 1];
  }

  _rollingCode++;
  EEPROM.put(_eepromAddress, _rollingCode);
}

void SomfyRemote::sendCommand(uint8_t *frame, uint8_t sync)
{
  if (sync == 2)
  {
    // Initialisation CC1101 au premier appel
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SetTx(433.42);

    pinMode(s_gdo0, OUTPUT);

    // Impulsion réveil
    digitalWrite(s_gdo0, HIGH);
    delayMicroseconds(9415);
    digitalWrite(s_gdo0, LOW);
    delayMicroseconds(89565);
  }

  // Synchronisation matérielle
  for (int i = 0; i < sync; i++)
  {
    digitalWrite(s_gdo0, HIGH);
    delayMicroseconds(4 * symbol);
    digitalWrite(s_gdo0, LOW);
    delayMicroseconds(4 * symbol);
  }

  // Synchronisation logicielle
  digitalWrite(s_gdo0, HIGH);
  delayMicroseconds(4550);
  digitalWrite(s_gdo0, LOW);
  delayMicroseconds(symbol);

  // Données (56 bits, encodage Manchester)
  for (uint8_t i = 0; i < 56; i++)
  {
    sendBit((frame[i / 8] >> (7 - (i % 8))) & 1);
  }

  digitalWrite(s_gdo0, LOW);
  delayMicroseconds(30415);
}

void SomfyRemote::sendBit(bool value)
{
  // Manchester Somfy : 1 = LOW→HIGH, 0 = HIGH→LOW
  if (value)
  {
    digitalWrite(s_gdo0, LOW);
    delayMicroseconds(symbol);
    digitalWrite(s_gdo0, HIGH);
    delayMicroseconds(symbol);
  }
  else
  {
    digitalWrite(s_gdo0, HIGH);
    delayMicroseconds(symbol);
    digitalWrite(s_gdo0, LOW);
    delayMicroseconds(symbol);
  }
}
