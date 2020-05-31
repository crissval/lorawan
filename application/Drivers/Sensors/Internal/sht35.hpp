/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* Driver SHT35 for ConnecSenS Device *********************/
/* Datasheet : http://bit.ly/2mgllap                      */
/**********************************************************/
#pragma once

#include "sensor_internal.hpp"



class SHT35 : public SensorInternal
{
public:
  static const char *TYPE_NAME;


private:
  // Alert
  typedef struct AlertLimits
  {
    float humidityHighSet;
    float temperatureHighSet;
    float humidityHighClear;
    float temperatureHighClear;
    float humidityLowClear;
    float temperatureLowClear;
    float humidityLowSet;
    float temperatureLowSet;
  }
  AlertLimits;

  // Status-Register
  typedef union Status
  {
    uint16_t u16;
    struct
    {
      uint16_t CrcStatus     : 1; // write data checksum status
      uint16_t CmdStatus     : 1; // command status
      uint16_t Reserve0      : 2; // reserved
      uint16_t ResetDetected : 1; // system reset detected
      uint16_t Reserve1      : 5; // reserved
      uint16_t T_Alert       : 1; // temperature tracking alert
      uint16_t RH_Alert      : 1; // humidity tracking alert
      uint16_t Reserve2      : 1; // reserved
      uint16_t HeaterStatus  : 1; // heater status
      uint16_t Reserve3      : 1; // reserved
      uint16_t AlertPending  : 1; // alert pending status
    }
    bit;
  }
  Status;


  typedef enum Command
  {
    CMD_READ_SERIALNBR  = 0x3780, // read serial number
    CMD_READ_STATUS     = 0xF32D, // read status register
    CMD_CLEAR_STATUS    = 0x3041, // clear status register
    CMD_HEATER_ENABLE   = 0x306D, // enabled heater
    CMD_HEATER_DISABLE  = 0x3066, // disable heater
    CMD_SOFT_RESET      = 0x30A2, // soft reset
    CMD_MEAS_CLOCKSTR_H = 0x2C06, // measurement: clock stretching, high repeatability
    CMD_MEAS_CLOCKSTR_M = 0x2C0D, // measurement: clock stretching, medium repeatability
    CMD_MEAS_CLOCKSTR_L = 0x2C10, // measurement: clock stretching, low repeatability
    CMD_MEAS_POLLING_H  = 0x2400, // measurement: polling, high repeatability
    CMD_MEAS_POLLING_M  = 0x240B, // measurement: polling, medium repeatability
    CMD_MEAS_POLLING_L  = 0x2416, // measurement: polling, low repeatability
    CMD_MEAS_PERI_05_H  = 0x2032, // measurement: periodic 0.5 mps, high repeatability
    CMD_MEAS_PERI_05_M  = 0x2024, // measurement: periodic 0.5 mps, medium repeatability
    CMD_MEAS_PERI_05_L  = 0x202F, // measurement: periodic 0.5 mps, low repeatability
    CMD_MEAS_PERI_1_H   = 0x2130, // measurement: periodic 1 mps, high repeatability
    CMD_MEAS_PERI_1_M   = 0x2126, // measurement: periodic 1 mps, medium repeatability
    CMD_MEAS_PERI_1_L   = 0x212D, // measurement: periodic 1 mps, low repeatability
    CMD_MEAS_PERI_2_H   = 0x2236, // measurement: periodic 2 mps, high repeatability
    CMD_MEAS_PERI_2_M   = 0x2220, // measurement: periodic 2 mps, medium repeatability
    CMD_MEAS_PERI_2_L   = 0x222B, // measurement: periodic 2 mps, low repeatability
    CMD_MEAS_PERI_4_H   = 0x2334, // measurement: periodic 4 mps, high repeatability
    CMD_MEAS_PERI_4_M   = 0x2322, // measurement: periodic 4 mps, medium repeatability
    CMD_MEAS_PERI_4_L   = 0x2329, // measurement: periodic 4 mps, low repeatability
    CMD_MEAS_PERI_10_H  = 0x2737, // measurement: periodic 10 mps, high repeatability
    CMD_MEAS_PERI_10_M  = 0x2721, // measurement: periodic 10 mps, medium repeatability
    CMD_MEAS_PERI_10_L  = 0x272A, // measurement: periodic 10 mps, low repeatability
    CMD_FETCH_DATA      = 0xE000, // readout measurements for periodic mode
    CMD_R_AL_LIM_LS     = 0xE102, // read alert limits, low set
    CMD_R_AL_LIM_LC     = 0xE109, // read alert limits, low clear
    CMD_R_AL_LIM_HS     = 0xE11F, // read alert limits, high set
    CMD_R_AL_LIM_HC     = 0xE114, // read alert limits, high clear
    CMD_W_AL_LIM_HS     = 0x611D, // write alert limits, high set
    CMD_W_AL_LIM_HC     = 0x6116, // write alert limits, high clear
    CMD_W_AL_LIM_LC     = 0x610B, // write alert limits, low clear
    CMD_W_AL_LIM_LS     = 0x6100, // write alert limits, low set
    CMD_NO_SLEEP        = 0x303E
  }
  Command;

  // Frequency
  typedef enum MeasurementFrequency
  {
    FREQUENCY_HZ5,  //  0.5 measurements per seconds
    FREQUENCY_1HZ,  //  1.0 measurements per seconds
    FREQUENCY_2HZ,  //  2.0 measurements per seconds
    FREQUENCY_4HZ,  //  4.0 measurements per seconds
    FREQUENCY_10HZ  // 10.0 measurements per seconds
  }
  MeasurementFrequency;

  // Measurement Repeatability
  typedef enum MeasurementRepeatability
  {
    REPEATAB_LOW,    // high repeatability
    REPEATAB_MEDIUM, // medium repeatability
    REPEATAB_HIGH    // low repeatability
  }
  MeasurementRepeatability;

  // Measurement Mode
  typedef enum MeasurementMode
  {
    MODE_CLKSTRETCH, // clock stretching
    MODE_POLLING     // polling
  }
  MeasurementMode;


public:
  SHT35();
  static Sensor *getNewInstance();
  const char    *type();

  uint32_t    serialNumber();
  const char *uniqueId();


private:
  bool openSpecific();
  bool readSpecific();
  bool configAlertSpecific();
  bool jsonAlarmSpecific(                 const JsonObject&       json,
					  CNSSInt::Interruptions *pvAlarmInts);
  bool writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame        *pvFrame);

  const char **csvHeaderValues();
  int32_t      csvDataSpecific(char *ps_data, uint32_t size);

  bool     interFrameDelay();
  bool     readSerialNumber();
  bool     readPeriodic();
  bool     readPolling();
  bool     readStatus(Status *pvStatus);
  bool     getTempAndHumiClkStretch();
  bool     getTempAndHumiPolling();
  bool     startPeriodicMeasurement();
  bool     enableHeater();
  bool     disableHeater();
  bool     softReset();
  bool     setAlertLimits(     const AlertLimits *pvThs);
  bool     getAlertLimits(     AlertLimits       *pvThs);
  bool     clearAllAlertFlags();
  uint8_t  calcCRC(            uint8_t *data, uint8_t nBytes);
  bool     writeCommand(       Command command);
  bool     read2NBytesAndCRC(  uint16_t *pu16Data, uint8_t nb);
  bool     write2BytesAndCRC(  Command command, uint16_t value);
  bool     writeAlertLimitData(Command command, float  humidity,   float  temperature);
  bool     readAlertLimitData( Command command, float *pfHumidity, float *pfTemperature);
  float    calcTemperature(    uint16_t rawValue);
  float    calcHumidity(       uint16_t rawValue);
  uint16_t calcRawTemperature( float    temperature);
  uint16_t calcRawHumidity(    float    humidity);


private:
  uint32_t    _serialNumber;
  float       _temperature;
  float       _humidity;
  AlertLimits _thresholds;

  static const char   *_CSV_HEADER_VALUES[];

  static const AlertLimits _DISABLE_THRESHOLDS;
  static const Command     _FREQUENCY_REPEATABILITY_TO_COMMAND[15];
};

