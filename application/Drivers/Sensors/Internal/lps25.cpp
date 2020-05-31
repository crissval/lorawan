#include <string.h>
#include "lps25.hpp"
#include "board.h"

#define LPS25_I2C_ADDRESS	0b10111000

#define LPS25_REG_ADDRESS_AUTO_INCREMENT  0x80

#define LPS25_REG_REF_P_XL	0x08
#define LPS25_REG_REF_P_L	0x09
#define LPS25_REG_REF_P_H	0x0A
#define LPS25_REG_WHO_AM_I	0x0F
#define LPS25_REG_RES_CONF	0x10
#define LPS25_REG_CTRL1		0x20
#define LPS25_REG_CTRL2		0x21
#define LPS25_REG_CTRL3		0x22
#define LPS25_REG_CTRL4		0x23
#define LPS25_REG_INT_CFG	0x24
#define LPS25_REG_INT_SOURCE	0x25
#define LPS25_REG_STATUS	0x27
#define LPS25_REG_PRESS_OUT_XL 	0x28
#define LPS25_REG_PRESS_OUT_L	0x29
#define LPS25_REG_PRESS_OUT_H	0x2A
#define LPS25_REG_TEMP_OUT_L	0x2B
#define LPS25_REG_TEMP_OUT_H	0x2C
#define LPS25_REG_FIFO_CTRL	0x2E
#define LPS25_REG_FIFO_STATUS	0x2F
#define LPS25_REG_THS_P_L	0x30
#define LPS25_REG_THS_P_H	0x31
#define LPS25_REG_RPDS_L	0x39
#define LPS25_REG_RPDS_H	0x3A

#define I2C_TIMEOUT_MS  1000

#define cad(var) ((var ^ -1) + 1)


const char *LPS25::TYPE_NAME = "LPS25";

const char *LPS25::_CSV_HEADER_VALUES[] = { "pressureHPa", NULL };



LPS25::LPS25() : SensorInternal(LPS25_I2C_ADDRESS, LITTLE_ENDIAN)
{
  // Do nothing
}


Sensor *LPS25::getNewInstance()
{
  return new LPS25();
}

const char *LPS25::type()
{
  return TYPE_NAME;
}


bool LPS25::readSpecific()
{
  uint32_t tickStart;
  uint8_t  temp        = 0xFF;
  uint32_t rawPressure = 0;

  /* Reference absolue */
  //  2. Turn on the pressure sensor analog front end in single shot mode  � WriteByte(CTRL_REG1_ADDR = 0x84); // @0x20 = 0x84
  //  3. Run one-shot measurement (temperature and pressure), the set bit will be reset by the sensor itself after execution (self-clearing bit) � WriteByte(CTRL_REG2_ADDR = 0x01); // @0x21 = 0x01
  if(!i2cWrite2BytesToRegister(LPS25_REG_RPDS_L | LPS25_REG_ADDRESS_AUTO_INCREMENT, 0) ||
      !i2cWriteByteToRegister( LPS25_REG_CTRL1, 0x86)                                  ||
      !i2cWriteByteToRegister( LPS25_REG_CTRL2, 0x01)) { goto error_exit; }

  tickStart = HAL_GetTick();
  while(1)
  {
    //  4. Wait until the measurement is completed � ReadByte(CTRL_REG2_ADDR = 0x00); // @0x21 = 0x00
    i2cReadByteFromRegister(LPS25_REG_CTRL2, &temp);
    if(!(temp & 0x01))                   { break;           }
    if(HAL_GetTick() - tickStart > 3000) { goto error_exit; }
    HAL_Delay(1);
  }

  //  5. Read the temperature measurement (2 bytes to read)  � Read((u8*)pu8, TEMP_OUT_ADDR, 2); // @0x2B(OUT_L)~0x2C(OUT_H)
  //� Temp_Reg_s16 = ((u16) pu8[1]<<8) | pu8[0];
  // make a SIGNED 16 bit variable  � Temperature_DegC = 42.5 + Temp_Reg_s16 / 480; // offset and scale
  //  6. Read the pressure measurement
  //� Read((u8*)pu8, PRESS_OUT_ADDR, 3); // @0x28(OUT_XL)~0x29(OUT_L)~0x2A(OUT_H)
  //� Pressure_Reg_s32 = ((u32)pu8[2]<<16)|((u32)pu8[1]<<8)|pu8[0];
  // make a SIGNED 32 bit � Pressure_mb = Pressure_Reg_s32 / 4096; // scale
  if(!i2cRead3BytesFromRegister(LPS25_REG_PRESS_OUT_XL | LPS25_REG_ADDRESS_AUTO_INCREMENT,
				&rawPressure)) { goto error_exit; }
  this->pressure = (float)rawPressure / 4096.0f;

  return true;

  error_exit:
  return false;
}

bool LPS25::configAlertSpecific()
{
  uint8_t  u8;
  uint16_t THSP;
  uint32_t PressRef;

  // Calcul des seuils
  // Le seuil est donn�e pour la valeur haute et basse
  // C'est la diff�rence entre la pression de r�f�rence et la pression mesur�e
  // qui est compar�e en valeur absolue au seuil
  THSP     = ((this->HighThreshold - this->LowThreshold) / 2) * 16;
  PressRef = ((this->HighThreshold - this->LowThreshold) / 2 + this->LowThreshold) * 16;

  return open() &&
      i2cWriteByteToRegister(  LPS25_REG_CTRL2,   0)    &&
      i2cWrite2BytesToRegister(LPS25_REG_RPDS_L  | LPS25_REG_ADDRESS_AUTO_INCREMENT, PressRef) &&
      i2cWrite2BytesToRegister(LPS25_REG_THS_P_L | LPS25_REG_ADDRESS_AUTO_INCREMENT, THSP)     &&
      i2cWriteByteToRegister(  LPS25_REG_INT_CFG, 0x03) &&
      i2cWriteByteToRegister(  LPS25_REG_CTRL1,   0x98) &&
      i2cWriteByteToRegister(  LPS25_REG_CTRL3,   0x03) &&
      i2cReadByteFromRegister( LPS25_REG_INT_SOURCE, &u8);
}

bool LPS25::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  this->HighThreshold = json["highThresholdHPa"];
  this->LowThreshold  = json["lowThresholdHPa"];

  bool noAlarm = this->LowThreshold == 0.0 &&   this->HighThreshold == 0.0;
  bool alarmOk = false;
  if(!noAlarm) { alarmOk = this->HighThreshold > this->LowThreshold; }

  *pvAlarmInts = alarmOk ? CNSSInt::LPS25_FLAG : CNSSInt::INT_FLAG_NONE;
  return noAlarm || alarmOk;
}


bool LPS25::WhoAmI()
{
  uint8_t v;

  return i2cReadByteFromRegister(LPS25_REG_WHO_AM_I, &v) && v == 0xBD;
}


bool LPS25::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_pressure_write_atmo_hpa_to_frame(pvFrame, this->pressure);
}



const char **LPS25::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}


int32_t LPS25::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%.3f", this->pressure);

  return len >= size ? -1 : (int32_t)len;
}



