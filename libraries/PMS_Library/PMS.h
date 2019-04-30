#ifndef PMS_H
#define PMS_H

#include "Stream.h"

#define SINGLE_RESPONSE_TIME 1000
#define TOTAL_RESPONSE_TIME 10000
#define STEADY_RESPONSE_TIME 30000

#define BAUD_RATE 9600

struct PMS_DATA
{
  // Standard Particles, CF=1
  uint16_t PM_SP_UG_1_0;
  uint16_t PM_SP_UG_2_5;
  uint16_t PM_SP_UG_10_0;

  // Atmospheric environment
  uint16_t PM_AE_UG_1_0;
  uint16_t PM_AE_UG_2_5;
  uint16_t PM_AE_UG_10_0;

  uint16_t TEMPERATURE;
  uint16_t HUMIDITY;
};

enum PMS_STATUS { STATUS_WAITING, STATUS_OK };
enum PMS_MODE { MODE_ACTIVE, MODE_PASSIVE };

class PMS
{
public:

  PMS();
  void sleep();
  void wakeUp();
  void activeMode();
  void passiveMode();

  void requestRead();
  bool read(struct PMS_DATA* data);
  bool readUntil(struct PMS_DATA* data, uint16_t timeout = SINGLE_RESPONSE_TIME);

  struct PMS_DATA* _data;

private:

  uint8_t _payload[24];
//  struct PMS_DATA* _data;
  PMS_STATUS _status;
  PMS_MODE _mode;

  uint8_t _index;
  uint16_t _frameLen;
  uint16_t _checksum;
  uint16_t _calculatedChecksum;

  void loop();
  void write(uint8_t* cmd, uint8_t size);
};

#endif