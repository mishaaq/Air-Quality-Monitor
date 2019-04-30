#include "Arduino.h"
#include "EEPROM.h"
#include <PMS.h>
#include <ZUNO_OLED_1106_I2C.h>
#include <ZUNO_OLED_FONT_NUMB16.h>
#include <ZUNO_OLED_FONT_NUMB24.h>
#include <ZUNO_OLED_IMG_LOGO.h>

#define DEBUG 1

#define PRINT_DEBUG(str)             do { if (DEBUG) Serial.println(str); } while(0)
#define PRINT_DEBUG_VAL(str, val)    do { if (DEBUG) {Serial.print(str);Serial.println(val); } } while(0)


#define LED_PIN       14
#define MAGIC_VALUE   34

#define CHANNEL_PM25        1
#define CHANNEL_PM10        2
#define CHANNEL_TEMPERATURE 3
#define CHANNEL_HUMIDITY    4

#define ZWAVE_CONFIG_WAKEUP_INTERVAL 64

OLED oled;
PMS pms;
struct PMS_DATA readings;

ZUNO_SETUP_CHANNELS(
  ZUNO_SENSOR_MULTILEVEL(
    ZUNO_SENSOR_MULTILEVEL_TYPE_GENERAL_PURPOSE_VALUE,
    SENSOR_MULTILEVEL_SCALE_PARTS_PER_MILLION,
    SENSOR_MULTILEVEL_SIZE_TWO_BYTES,
    SENSOR_MULTILEVEL_PRECISION_ZERO_DECIMALS,
    getPM25
  ),
  ZUNO_SENSOR_MULTILEVEL(
    ZUNO_SENSOR_MULTILEVEL_TYPE_GENERAL_PURPOSE_VALUE,
    SENSOR_MULTILEVEL_SCALE_PARTS_PER_MILLION,
    SENSOR_MULTILEVEL_SIZE_TWO_BYTES,
    SENSOR_MULTILEVEL_PRECISION_ZERO_DECIMALS,
    getPM10
  ),
  ZUNO_SENSOR_MULTILEVEL(
    ZUNO_SENSOR_MULTILEVEL_TYPE_TEMPERATURE,
    SENSOR_MULTILEVEL_SCALE_CELSIUS,
    SENSOR_MULTILEVEL_SIZE_TWO_BYTES,
    SENSOR_MULTILEVEL_PRECISION_ONE_DECIMAL,
    getTemperature
  ),
  ZUNO_SENSOR_MULTILEVEL_HUMIDITY(getHumidity)
);

ZUNO_SETUP_CFGPARAMETER_HANDLER(config_parameter_changed);
//ZUNO_SETUP_SLEEPING_MODE(ZUNO_SLEEPING_MODE_FREQUENTLY_AWAKE);

struct ZWAVE_CONFIG {
  word wakeup_interval; // wakeup interval in seconds
} Config = { 180 };

void setup() {
  // setup ZUNO
#ifdef DEBUG
  Serial.begin(9600);
#endif
  pinMode(LED_PIN, OUTPUT);
  zunoLoadCFGParam(ZWAVE_CONFIG_WAKEUP_INTERVAL, &Config.wakeup_interval);

  // setup PMS
  Serial0.begin(BAUD_RATE);
  pms.passiveMode();

  // setup OLED
  oled.begin();
  oled.clrscr();
  oled.writeData(zuno_img_logo);
  oled.display();

  // setup NZRAM
  byte nz_magic_value;
  NZRAM.get(0x0, &nz_magic_value, sizeof(nz_magic_value));
  if (nz_magic_value == MAGIC_VALUE)
    NZRAM.get(0x1, &readings, sizeof(readings));
  else
    NZRAM.put(0x1, &nz_magic_value, sizeof(nz_magic_value));
}

void config_parameter_changed(byte param, word *value) {
  if (param == ZWAVE_CONFIG_WAKEUP_INTERVAL)
    Config.wakeup_interval = *value;
}

void loop() {

  PRINT_DEBUG("Waking up, wait 30 seconds for stable readings...");
  pms.wakeUp();
  delay(30000);

  PRINT_DEBUG("Send read request...");
  pms.requestRead();

  PRINT_DEBUG("Reading data...");
  if (pms.readUntil(&readings))
  {
    PRINT_DEBUG_VAL("PM 1.0 (ug/m3): ", readings.PM_AE_UG_1_0);
    PRINT_DEBUG_VAL("PM 2.5 (ug/m3): ", readings.PM_AE_UG_2_5);
    PRINT_DEBUG_VAL("PM 10.0 (ug/m3): ", readings.PM_AE_UG_10_0);
    PRINT_DEBUG_VAL("Temperature: ", readings.TEMPERATURE);
    PRINT_DEBUG_VAL("Humidity (%): ", readings.HUMIDITY);
  } else {
    PRINT_DEBUG("No data :(");
  }
  PRINT_DEBUG("Sending PMS to sleep.");
  pms.sleep();
  
  displayReadings();
  
  zunoSendReport(CHANNEL_PM25);
  zunoSendReport(CHANNEL_PM10);
  zunoSendReport(CHANNEL_TEMPERATURE);
  zunoSendReport(CHANNEL_HUMIDITY);

  zunoSetBeamCountWU(Config.wakeup_interval);
  //zunoSendDeviceToSleep();
}

void displayReadings(void) {
  oled.clrscr();

  oled.setFont(0);
  oled.gotoXY(0, 0);
  oled.print("PM 2.5:");

  oled.gotoXY(62, 0);
  oled.print("PM 10:");

  oled.gotoXY(0, 5);
  oled.print("T:");
  oled.gotoXY(70, 5);
  oled.print("H:");

  oled.setFont(zuno_font_numbers24);
  oled.gotoXY(12, 1);
  if (readings.PM_AE_UG_2_5 < 100)
    oled.print("0");
  if (readings.PM_AE_UG_2_5 < 10)
    oled.print("0");
  oled.print(readings.PM_AE_UG_2_5);

  oled.gotoXY(74, 1);
  if (readings.PM_AE_UG_10_0 < 100)
    oled.print("0");
  if (readings.PM_AE_UG_10_0 < 10)
    oled.print("0");
  oled.print(readings.PM_AE_UG_10_0);

  oled.setFont(zuno_font_numbers16);
  oled.gotoXY(12, 6);
  oled.fixPrint(int(readings.TEMPERATURE),1);
  oled.setFont(0);
  oled.print("C");
  oled.gotoXY(80, 6);
  oled.setFont(zuno_font_numbers16);
  oled.print(readings.HUMIDITY/10);
  oled.setFont(0);
  oled.print("%");
  
  oled.display();
}

word getPM25(void) {
  return readings.PM_AE_UG_2_5;
}

word getPM10(void) {
  return readings.PM_AE_UG_10_0;
}

word getTemperature(void) {
  return readings.TEMPERATURE;
}

byte getHumidity(void) {
  return readings.HUMIDITY/10;
}
