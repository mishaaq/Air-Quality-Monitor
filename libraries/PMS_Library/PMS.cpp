#include "Arduino.h"
#include "PMS.h"

#define MAKEWORD(high, low) ((word)((((word)(high)) << 8) | ((byte)(low))))

uint8_t sleepCommand[] = { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
uint8_t wakeUpCommand[] = { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
uint8_t activeModeCommand[] = { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
uint8_t passiveModeCommand[] = { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
uint8_t requestReadCommand[] = { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };


PMS::PMS()
{
  _index = 0;
  _mode = MODE_ACTIVE;
}

// Standby mode. For low power consumption and prolong the life of the sensor.
void PMS::sleep()
{
  write(sleepCommand, sizeof(sleepCommand));
}

// Operating mode. Stable data should be got at least 30 seconds after the sensor wakeup from the sleep mode because of the fan's performance.
void PMS::wakeUp()
{
  write(wakeUpCommand, sizeof(wakeUpCommand));
}

// Active mode. Default mode after power up. In this mode sensor would send serial data to the host automatically.
void PMS::activeMode()
{
  write(activeModeCommand, sizeof(activeModeCommand));
  _mode = MODE_ACTIVE;
}

// Passive mode. In this mode sensor would send serial data to the host only for request.
void PMS::passiveMode()
{
  write(passiveModeCommand, sizeof(passiveModeCommand));
  _mode = MODE_PASSIVE;
}

// Request read in Passive Mode.
void PMS::requestRead()
{
  if (_mode == MODE_PASSIVE)
  {
    write(requestReadCommand, sizeof(requestReadCommand));
  }
}

void PMS::write(uint8_t* command, uint8_t size)
{
  uint8_t index = 0;
  while(index < size)
  {
    Serial0.write(command[index]);
    command++;
    index++;
  }
  Serial0.flush();
}

// Non-blocking function for parse response.
bool PMS::read(struct PMS_DATA* data)
{
  _data = data;
  loop();
  
  return _status == STATUS_OK;
}

// Blocking function for parse response. Default timeout is 1s.
bool PMS::readUntil(struct PMS_DATA* data, uint16_t timeout)
{
  _data = data;

  uint32_t start = millis();
  do
  {
    loop();
    if (_status == STATUS_OK) break;
  } while (millis() - start < timeout);

  return _status == STATUS_OK;
}

void PMS::loop()
{
  _status = STATUS_WAITING;
  if (Serial0.available())
  {
    uint8_t ch = Serial0.read();

    switch (_index)
    {
    case 0:
      if (ch != 0x42)
      {
        return;
      }
      _calculatedChecksum = ch;
      break;

    case 1:
      if (ch != 0x4D)
      {
        _index = 0;
        return;
      }
      _calculatedChecksum += ch;
      break;

    case 2:
      _calculatedChecksum += ch;
      _frameLen = ch << 8;
      break;

    case 3:
      _frameLen |= ch;
      // Unsupported sensor, different frame length, transmission error e.t.c.
      if (_frameLen != 2 * 9 + 2 && _frameLen != 2 * 13 + 2)
      {
        _index = 0;
        return;
      }
      _calculatedChecksum += ch;
      break;

    default:
      if (_index == _frameLen + 2)
      {
        _checksum = ch << 8;
      }
      else if (_index == _frameLen + 2 + 1)
      {
        _checksum |= ch;

        if (_calculatedChecksum == _checksum)
        {
          _status = STATUS_OK;

          // Standard Particles, CF=1.
          _data->PM_SP_UG_1_0 = MAKEWORD(_payload[0], _payload[1]);
          _data->PM_SP_UG_2_5 = MAKEWORD(_payload[2], _payload[3]);
          _data->PM_SP_UG_10_0 = MAKEWORD(_payload[4], _payload[5]);

          // Atmospheric Environment.
          _data->PM_AE_UG_1_0 = MAKEWORD(_payload[6], _payload[7]);
          _data->PM_AE_UG_2_5 = MAKEWORD(_payload[8], _payload[9]);
          _data->PM_AE_UG_10_0 = MAKEWORD(_payload[10], _payload[11]);

          _data->TEMPERATURE = MAKEWORD(_payload[20], _payload[21]);
          _data->HUMIDITY = MAKEWORD(_payload[22], _payload[23]);
        }

        _index = 0;
        return;
      }
      else
      {
        _calculatedChecksum += ch;
        uint8_t payloadIndex = _index - 4;

        // Payload is common to all sensors (first 2x6 bytes).
        if (payloadIndex < (byte)sizeof(_payload))
        {
          _payload[payloadIndex] = ch;
        }
      }

      break;
    }

    _index++;
  }
}
