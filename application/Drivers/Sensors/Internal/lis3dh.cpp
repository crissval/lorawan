
#include <string.h>
#include "lis3dh.hpp"
#include "connecsens.hpp"
#include "board.h"

#define LIS3DH_I2C_ADDRESS  0x32

/**************************** REGISTER MAP ****************************/
#define LIS3DH_REG_STATUS_REG_AUX                	0x07
#define LIS3DH_REG_OUT_ADC1_L                    	0x08
#define LIS3DH_REG_OUT_ADC1_H                           0x09
#define LIS3DH_REG_OUT_ADC2_L                           0x0A
#define LIS3DH_REG_OUT_ADC2_H                    	0x0B
#define LIS3DH_REG_OUT_ADC3_L                    	0x0C
#define LIS3DH_REG_OUT_ADC3_H                    	0x0D
#define LIS3DH_REG_WHO_AM_I                             0x0F
#define LIS3DH_REG_CTRL_REG0                            0x1E
#define LIS3DH_REG_TEMP_CFG_REG                  	0x1F
#define LIS3DH_REG_CTRL_REG1                            0x20
#define LIS3DH_REG_CTRL_REG2                            0x21
#define LIS3DH_REG_CTRL_REG3                            0x22
#define LIS3DH_REG_CTRL_REG4                            0x23
#define LIS3DH_REG_CTRL_REG5                            0x24
#define LIS3DH_REG_CTRL_REG6                            0x25
#define LIS3DH_REG_REFERENCE                            0x26
#define LIS3DH_REG_STATUS_REG                    	0x27
#define LIS3DH_REG_OUT_X_L                              0x28
#define LIS3DH_REG_OUT_X_H                              0x29
#define LIS3DH_REG_OUT_Y_L                              0x2A
#define LIS3DH_REG_OUT_Y_H                              0x2B
#define LIS3DH_REG_OUT_Z_L                              0x2C
#define LIS3DH_REG_OUT_Z_H                              0x2D
#define LIS3DH_REG_FIFO_CTRL_REG                   	0x2E
#define LIS3DH_REG_FIFO_SRC_REG                   	0x2F
#define LIS3DH_REG_INT1_CFG                             0x30
#define LIS3DH_REG_INT1_SOURCE                          0x31
#define LIS3DH_REG_INT1_THS                             0x32
#define LIS3DH_REG_INT1_DURATION                   	0x33
#define LIS3DH_REG_INT2_CFG                             0x34
#define LIS3DH_REG_INT2_SOURCE                          0x35
#define LIS3DH_REG_INT2_THS                             0x36
#define LIS3DH_REG_INT2_DURATION                   	0x37
#define LIS3DH_REG_CLICK_CFG                            0x38
#define LIS3DH_REG_CLICK_SRC                            0x39
#define LIS3DH_REG_CLICK_THS                            0x3A
#define LIS3DH_REG_TIME_LIMIT                           0x3B
#define LIS3DH_REG_TIME_LATENCY                    	0x3C
#define LIS3DH_REG_TIME_WINDOW                          0x3D
#define LIS3DH_REG_ACT_THS                   		0x3E
#define LIS3DH_REG_ACT_DUR                   		0x3F

#define LIS2DH_REG_ADDRESS_AUTO_INCREMENT  0x80

/*************************** CTRL_REG1 MASK **************************/
#define LIS3DH_CTRL_REG1_ALL_AXIS              			0x07
#define LIS3DH_CTRL_REG1_X_AXIS                			0x01
#define LIS3DH_CTRL_REG1_Y_AXIS                			0x02
#define LIS3DH_CTRL_REG1_Z_AXIS                			0x04
#define LIS3DH_CTRL_REG1_LOW_POWER             			0x08
#define LIS3DH_CTRL_REG1_POWER_DOWN                             0x00
#define LIS3DH_CTRL_REG1_1HZ              		     	0x10
#define LIS3DH_CTRL_REG1_10HZ                 			0x20
#define LIS3DH_CTRL_REG1_25HZ                  			0x30
#define LIS3DH_CTRL_REG1_50HZ                  			0x40
#define LIS3DH_CTRL_REG1_100HZ                 			0x50
#define LIS3DH_CTRL_REG1_200HZ                 			0x60
#define LIS3DH_CTRL_REG1_400HZ                 			0x70
#define LIS3DH_CTRL_REG1_LP_1K6HZ      				0x80
#define LIS3DH_CTRL_REG1_N_1K25_LP_5K  				0x90

#define LIS3DH_CTRL_REG2_HPM_NORMAL     0x00
#define LIS3DH_CTRL_REG2_HPM_REFERENCE  0x40
#define LIS3DH_CTRL_REG2_HPM_AUTORESET  0xC0
#define LIS3DH_CTRL_REG2_HPCF_00        0x00
#define LIS3DH_CTRL_REG2_HPCF_01        0x10
#define LIS3DH_CTRL_REG2_HPCF_10        0x20
#define LIS3DH_CTRL_REG2_HPCF_11        0x30
#define LIS3DH_CTRL_REG2_FDS            0x08
#define LIS3DH_CTRL_REG2_HPCLICK        0x04
#define LIS3DH_CTRL_REG2_HPIA2          0x02
#define LIS3DH_CTRL_REG2_HPIA1          0x01


/*************************** CTRL_REG3 MASK **************************/
#define LIS3DH_CTRL_REG3_I1_NONE                                0x00
#define LIS3DH_CTRL_REG3_I1_CLICK              			0x80
#define LIS3DH_CTRL_REG3_I1_IA1               			0x40
#define LIS3DH_CTRL_REG3_I1_IA2                			0x20
#define LIS3DH_CTRL_REG3_I1_ZYXDA              			0x10
#define LIS3DH_CTRL_REG3_I1_321DA              			0x08
#define LIS3DH_CTRL_REG3_I1_WTM                			0x04
#define LIS3DH_CTRL_REG3_I1_OVERRUN    				0x02

/*************************** CTRL_REG4 MASK **************************/
#define LIS3DH_CTRL_REG4_BLOCK                 			0x80
#define LIS3DH_CTRL_REG4_BLE                   			0x40
#define LIS3DH_CTRL_REG4_BLE_LITTLE_ENDIAN                      0x00
#define LIS3DH_CTRL_REG4_BLE_BIG_ENDIAN                         LIS3DH_CTRL_REG4_BLE
#define LIS3DH_CTRL_REG4_2G_SCALE              			0x00
#define LIS3DH_CTRL_REG4_4G_SCALE              			0x10
#define LIS3DH_CTRL_REG4_8G_SCALE              			0x20
#define LIS3DH_CTRL_REG4_16G_SCALE     				0x30
#define LIS3DH_CTRL_REG4_HIGHRES               			0x08

/*************************** CTRL_REG5 MASK **************************/
#define LIS3DH_CTRL_REG5_BOOT                  			0x80
#define LIS3DH_CTRL_REG5_FIFO_EN               			0x40
#define LIS3DH_CTRL_REG5_FIFO_DISABLE                           0x00
#define LIS3DH_CTRL_REG5_FIFO_ENABLE                            LIS3DH_CTRL_REG5_FIFO_EN
#define LIS3DH_CTRL_REG5_LIR_INT1              			0x08
#define LIS3DH_CTRL_REG5_D4D_INT1              			0x04

/*************************** CTRL_REG6 MASK **************************/
#define LIS3DH_CTRL_REG6_I2_NONE                                0x00
#define LIS3DH_CTRL_REG6_I2_CLICK    				0x80
#define LIS3DH_CTRL_REG6_I2_IA1              			0x40
#define LIS3DH_CTRL_REG6_I2_IA2               			0x20
#define LIS3DH_CTRL_REG6_I2_BOOT    				0x10
#define LIS3DH_CTRL_REG6_I2_ACT    				0x08
#define LIS3DH_CTRL_REG6_INT_POLARITY          			0x02
#define LIS3DH_CTRL_REG6_INT_ACTIVE_HIGH                        0x00
#define LIS3DH_CTRL_REG6_INT_ACTIVE_LOW                         LIS3DH_CTRL_REG6_INT_POLARITY

/*************************** STATUS_REG MASK *************************/
#define LIS3DH_STATUS_REG_ZYXOR                			0x80
#define LIS3DH_STATUS_REG_ZOR                  			0x40
#define LIS3DH_STATUS_REG_YOR                 			0x20
#define LIS3DH_STATUS_REG_XOR                  			0x10
#define LIS3DH_STATUS_REG_ZYXDA                			0x08
#define LIS3DH_STATUS_REG_ZDA                  			0x04
#define LIS3DH_STATUS_REG_YDA                  			0x02
#define LIS3DH_STATUS_REG_XDA                  			0x01

/**************************** INT1_CFG MASK **************************/
#define LIS3DH_INT1_CFG_AOI                    			0x80
#define LIS3DH_INT1_CFG_6D                    			0x40
#define LIS3DH_INT1_CFG_OR_EVENTS                               0x00
#define LIS3DH_INT1_CFG_OR_6DIRS_MVT                            (LIS3DH_INT1_CFG_6D)
#define LIS3DH_INT1_CFG_AND_EVENTS                              (LIS3DH_INT1_CFG_AOI)
#define LIS3DH_INT1_CFG_AND_6DIRS_POS                           (LIS3DH_INT1_CFG_AOI | LIS3DH_INT1_CFG_6D)
#define LIS3DH_INT1_CFG_ZHIE             			0x20
#define LIS3DH_INT1_CFG_ZLIE    				0x10
#define LIS3DH_INT1_CFG_YHIE              			0x08
#define LIS3DH_INT1_CFG_YLIE   					0x04
#define LIS3DH_INT1_CFG_XHIE              			0x02
#define LIS3DH_INT1_CFG_XLIE    				0x01

/**************************** INT1_SRC MASK **************************/
#define LIS3DH_INT1_SRC_IA                     			0x40
#define LIS3DH_INT1_SRC_ZH                     			0x20
#define LIS3DH_INT1_SRC_ZL                     			0x10
#define LIS3DH_INT1_SRC_YH             					0x08
#define LIS3DH_INT1_SRC_YL                    			0x04
#define LIS3DH_INT1_SRC_XH            					0x02
#define LIS3DH_INT1_SRC_XL                     			0x01

/**************************** INT2_CFG MASK **************************/
#define LIS3DH_INT2_CFG_AOI                    			0x80
#define LIS3DH_INT2_CFG_6D                    			0x40
#define LIS3DH_INT2_CFG_ZHIE             				0x20
#define LIS3DH_INT2_CFG_ZLIE    						0x10
#define LIS3DH_INT2_CFG_YHIE              				0x08
#define LIS3DH_INT2_CFG_YLIE   							0x04
#define LIS3DH_INT2_CFG_XHIE              				0x02
#define LIS3DH_INT2_CFG_XLIE    						0x01

/**************************** INT2_SRC MASK **************************/
#define LIS3DH_INT2_SRC_IA                     			0x40
#define LIS3DH_INT2_SRC_ZH                     			0x20
#define LIS3DH_INT2_SRC_ZL                    			0x10
#define LIS3DH_INT2_SRC_YH             					0x08
#define LIS3DH_INT2_SRC_YL                    			0x04
#define LIS3DH_INT2_SRC_XH            					0x02
#define LIS3DH_INT2_SRC_XL                     			0x01

/*************************** CLICK_CFG MASK **************************/
#define LIS3DH_CLICK_CFG_Z_DOUBLE              			0x20
#define LIS3DH_CLICK_CFG_Z_SINGLE              			0x10
#define LIS3DH_CLICK_CFG_Y_DOUBLE              			0x08
#define LIS3DH_CLICK_CFG_Y_SINGLE            			0x04
#define LIS3DH_CLICK_CFG_X_DOUBLE              			0x02
#define LIS3DH_CLICK_CFG_X_SINGLE            			0x01

/*************************** CLICK_SRC MASK **************************/
#define LIS3DH_CLICK_SRC_IA                    			0x40
#define LIS3DH_CLICK_SRC_DOUBLE_CLICK          			0x20
#define LIS3DH_CLICK_SRC_SINGLE_CLICK        			0x10
#define LIS3DH_CLICK_SRC_SIGN          				0x08
#define LIS3DH_CLICK_SRC_Z                    			0x04
#define LIS3DH_LIS3DH_CLICK_SRC_Y            			0x02
#define LIS3DH_LIS3DH_CLICK_SRC_X             			0x01


#define DEFAULT_SCALE  SCALE_2G


/**
 * The CTRL configuration when no alarm is used.
 */
const uint8_t LIS3DH::_ctrlNoAlarm[8] =
{
    0x10,   // CTRL_REG0: disable SDO/SA0 pull-up
    0x00,   // TEMP_CFG_REG: no ADC, no temperature sensor.
    LIS3DH_CTRL_REG1_POWER_DOWN | LIS3DH_CTRL_REG1_ALL_AXIS,
    LIS3DH_CTRL_REG2_HPM_NORMAL,
    LIS3DH_CTRL_REG3_I1_NONE,
    LIS3DH_CTRL_REG4_BLOCK      | LIS3DH_CTRL_REG4_BLE_LITTLE_ENDIAN | LIS3DH_CTRL_REG4_HIGHRES,
    LIS3DH_CTRL_REG5_FIFO_DISABLE,
    LIS3DH_CTRL_REG6_I2_NONE
};

/**
 * The CTRL configuration when the alarm is used.
 */
const uint8_t LIS3DH::_ctrlAlarm[8] =
{
    0x10,   // CTRL_REG0: disable SDO/SA0 pull-up
    0x00,   // TEMP_CFG_REG: no ADC, no temperature sensor.
    LIS3DH_CTRL_REG1_25HZ         | LIS3DH_CTRL_REG1_ALL_AXIS,
    LIS3DH_CTRL_REG2_HPM_NORMAL   | LIS3DH_CTRL_REG2_HPCF_00           | LIS3DH_CTRL_REG2_HPIA1,
    LIS3DH_CTRL_REG3_I1_IA1,
    LIS3DH_CTRL_REG4_BLOCK        | LIS3DH_CTRL_REG4_BLE_LITTLE_ENDIAN | LIS3DH_CTRL_REG4_HIGHRES,
    LIS3DH_CTRL_REG5_FIFO_DISABLE | LIS3DH_CTRL_REG5_LIR_INT1,
    LIS3DH_CTRL_REG6_I2_NONE      | LIS3DH_CTRL_REG6_INT_ACTIVE_HIGH
};

/**
 * Get THS Step using Scale.
 */
const uint8_t LIS3DH::_scaleToTHSStep[4] =  { 16, 32, 62, 186 };

const char *LIS3DH::TYPE_NAME = "LIS3DH";

const char *LIS3DH::_CSV_HEADER_VALUES[] =
{
    "xAccelerationG", "yAccelerationG", "zAccelerationG", NULL
};


LIS3DH::LIS3DH() : SensorInternal(LIS3DH_I2C_ADDRESS, LITTLE_ENDIAN)
{
  this->_motionDetection  = false;
  this->_alarmThresholdMg = 0;
  this->_scale            = DEFAULT_SCALE;
  this->_x_accel          = this->_y_accel = this->_z_accel = 0.0;
}

Sensor *LIS3DH::getNewInstance()
{
  return new LIS3DH();
}

const char *LIS3DH::type()
{
  return TYPE_NAME;
}


bool LIS3DH::openSpecific()
{
  uint8_t v;
  uint8_t data[8];

  // Copy the configuration adapted to the situation
  if(this->_motionDetection) { memcpy(data, _ctrlAlarm,   sizeof(data)); }
  else                       { memcpy(data, _ctrlNoAlarm, sizeof(data)); }

  // Set the scale to use
  data[5] |= this->_scale << 4;

  // Configure device
  if(!SensorI2C::openSpecific() || !iamLIS3DH() ||
      !i2cMemWrite(LIS3DH_REG_CTRL_REG0 | LIS2DH_REG_ADDRESS_AUTO_INCREMENT, 1, data, sizeof(data)))
  { goto error_exit; }

  // Set up the alarm
  if(this->_motionDetection)
  {
    i2cReadByteFromRegister(LIS3DH_REG_REFERENCE, &v); // Dummy read to clear high-pass filter
    v       = _scaleToTHSStep[this->_scale];
    data[0] = (this->_alarmThresholdMg  + v / 2) / v; // THS
    data[1] = 0;                                      // Duration
    data[2] = LIS3DH_INT1_CFG_OR_EVENTS | LIS3DH_INT1_CFG_XHIE | LIS3DH_INT1_CFG_YHIE;  // INT1_CFG
    if(!i2cMemWrite(LIS3DH_REG_INT1_THS | LIS2DH_REG_ADDRESS_AUTO_INCREMENT, 1, data, 3)) { goto error_exit; }
  }

  return true;

  error_exit:
  return false;
}

void LIS3DH::closeSpecific()
{
  uint8_t v;

  if(!this->_motionDetection)
  {
    // Read from REFERENCE register before switching to power-down mode
    i2cReadByteFromRegister(LIS3DH_REG_REFERENCE, &v);
    UNUSED(v);

    // Switch to power-down mode.
    i2cWriteByteToRegister( LIS3DH_REG_CTRL_REG1, LIS3DH_CTRL_REG1_POWER_DOWN);
  }

  SensorInternal::closeSpecific();
}

bool LIS3DH::readSpecific()
{
  uint32_t tickStart;
  uint16_t values[3];
  uint8_t  v;

  if(!this->_motionDetection)
  {
    // Set active
    i2cWriteByteToRegister(LIS3DH_REG_CTRL_REG1, LIS3DH_CTRL_REG1_1HZ | LIS3DH_CTRL_REG1_ALL_AXIS);
  }

  // Wait for readings
  tickStart = HAL_GetTick();
  while(1)
  {
    i2cReadByteFromRegister(LIS3DH_REG_STATUS_REG, &v);
    if(v & LIS3DH_STATUS_REG_ZYXDA)      { break; }
    if(HAL_GetTick() - tickStart > 3000) { goto error_exit; }
    HAL_Delay(1);
  }

  // Get values
  if(!i2cMemRead(LIS3DH_REG_OUT_X_L | LIS2DH_REG_ADDRESS_AUTO_INCREMENT, 1,
		 (uint8_t *)values, sizeof(values))) { goto error_exit; }
  this->_x_accel = convertValueToAcceleration(values[0]);
  this->_y_accel = convertValueToAcceleration(values[1]);
  this->_z_accel = convertValueToAcceleration(values[2]);

  return true;

  error_exit:
  return false;
}

float LIS3DH::convertValueToAcceleration(uint16_t v)
{
  float res;
  bool invert = false;

  // 2-complement. If negative then convert to positive and keep track of sign.
  if(v & 0x8000) { invert = true; v = ~v + 1; }

  // The data are 12 bits left aligned; convert to right aligned
  v >>= 4;

  res = (float)(v * (2u << this->_scale)) / 2047.0;  // 2047 because 11 positives bits for 12 bits value
  if(invert) { res = -res; }

  return res;
}

bool LIS3DH::iamLIS3DH()
{
  uint8_t v;
  return i2cReadByteFromRegister(LIS3DH_REG_WHO_AM_I, &v) && v == 0x33;
}


void LIS3DH::readIntRegister()
{
  uint8_t v;
  /*uint8_t stateClickSrc = */  i2cReadByteFromRegister(LIS3DH_REG_CLICK_SRC,   &v);
  /*uint8_t stateInt1Src = */   i2cReadByteFromRegister(LIS3DH_REG_INT1_SOURCE, &v);
  /*uint8_t SEcstateInt1Src = */i2cReadByteFromRegister(LIS3DH_REG_INT1_SOURCE, &v);
  UNUSED(v);
}


bool LIS3DH::jsonAlarmSpecific(const JsonObject& json, CNSSInt::Interruptions *pvAlarmInts)
{
  this->_motionDetection = json["motionDetection"].as<bool>();

  *pvAlarmInts = this->_motionDetection ? CNSSInt::LIS3DH_FLAG : CNSSInt::INT_FLAG_NONE;
  return         this->_motionDetection;
}


bool LIS3DH::writeDataToCNSSRFDataFrameSpecific(CNSSRFDataFrame *pvFrame)
{
  return cnssrf_dt_acceleration_write_3dg_to_frame(
      pvFrame, this->_x_accel, this->_y_accel, this->_z_accel);
}


const char **LIS3DH::csvHeaderValues()
{
  return _CSV_HEADER_VALUES;
}

int32_t LIS3DH::csvDataSpecific(char *ps_data, uint32_t size)
{
  uint32_t len;

  len = snprintf(ps_data, size, "%f%c%f%c%f",
		 this->_x_accel, OUTPUT_DATA_CSV_SEP,
		 this->_y_accel, OUTPUT_DATA_CSV_SEP,
		 this->_z_accel);

  return len >= size ? -1 : (int32_t)len;
}


