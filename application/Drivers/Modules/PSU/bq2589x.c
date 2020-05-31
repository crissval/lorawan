/*
 * Driver for the BQ2589X family battery chargers?
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#include "bq2589x.h"
#include "board.h"
#include "gpio.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifndef BATT_VOLTAGE_FULL_V
#error "You MUST specify BATT_VOLTAGE_FULL_V voltage."
#endif
#ifndef BATT_VOLTAGE_EMPTY_V
#error "You MUST specify BATT_VOLTAGE_EMPTY_V voltage."
#endif


#define BATT_VOLTAGE_FULL_MV   ((uint16_t)(BATT_VOLTAGE_FULL_V  * 1000.0))
#define BATT_VOLTAGE_EMPTY_MV  ((uint16_t)(BATT_VOLTAGE_EMPTY_V * 1000.0))


#define BQ2589X_I2C_ADDRESS     0xD4
#define BQ2589X_I2C_TIMEOUT_MS  1000

#define BQ2589X_END_OF_ADC_CONVERSION_TIMEOUT_MS  1000


#define BQ2589X_SYS_SETTLE_WAIT_MS  100


  /**
   * The charger terminate current in mA.
   */
#ifndef BATT_CHARGER_TERMINATE_CURRENT_MA
#define BATT_CHARGER_TERMINATE_CURRENT_MA  256
#endif
  /**
   * The charger input resistance compensation configuration.
   */
#ifndef BATT_CHARGER_IR_COMP
#define BATT_CHARGER_IR_COMP     BQ2589X_IR_COMP_DEFAULT
#endif
#ifndef BATT_CHARGER_BOOST_MODE
#define BATT_CHARGER_BOOST_MODE  BQ2589X_BOOST_DEFAULT
#endif


  /* Registers */
#define BQ2589X_REG_INPUT_CURR      0x00
#define BQ2589X_REG_VINDPM          0x01
#define BQ2589X_REG_CFG1            0x02
#define BQ2589X_REG_CFG2            0x03
#define BQ2589X_REG_CHG_CURR        0x04
#define BQ2589X_REG_PRE_CHG_CURR    0x05
#define BQ2589X_REG_CHG_VOLT        0x06
#define BQ2589X_REG_TIMER           0x07
#define BQ2589X_REG_IR_COMP         0x08
#define BQ2589X_REG_FORCE           0x09
#define BQ2589X_REG_BOOST_MODE      0x0A
#define BQ2589X_REG_STATUS          0x0B /* Read-only */
#define BQ2589X_REG_FAULT           0x0C /* Read-only */
#define BQ2589X_REG_VINDPM_THRESH   0x0D
#define BQ2589X_REG_ADC_BATT_VOLT   0x0E /* Read-only */
#define BQ2589X_REG_ADC_SYS_VOLT    0x0F /* Read-only */
#define BQ2589X_REG_ADC_TS          0x10 /* Read-only */
#define BQ2589X_REG_ADC_VBUS_VOLT   0x11 /* Read-only */
#define BQ2589X_REG_ADC_CHG_CURR    0x12 /* Read-only */
#define BQ2589X_REG_ADC_INPUT_CURR  0x13 /* Read-only */
#define BQ2589X_REG_ID              0x14

  /* REG00 : input current register bit definitions */
#define BQ2589X_INPUT_CURR_EN_HIZ  (1<<7)
#define BQ2589X_INPUT_CURR_EN_ILIM (1<<6)
  /* REG02 : first configuration register bit definitions */
#define BQ2589X_CFG1_CONV_START    (1<<7)
#define BQ2589X_CFG1_ICO_EN        (1<<4)
#define BQ2589X_CFG1_AUTO_DPDM_EN  (1<<0)
  /* REG03 : second configuration register bit definitions */
#define BQ2589X_CFG2_SYS_MIN       (7<<1)
#define BQ2589X_CFG2_CHG_CONFIG    (1<<4)
#define BQ2589X_CFG2_OTG_CONFIG    (1<<5)
#define BQ2589X_CFG2_WD_RST        (1<<6)
  /* REG08 : IR compensation definitions */
#define BQ2589X_IR_BAT_COMP_140MOHM (7 << 5)
#define BQ2589X_IR_BAT_COMP_120MOHM (6 << 5)
#define BQ2589X_IR_BAT_COMP_100MOHM (5 << 5)
#define BQ2589X_IR_BAT_COMP_80MOHM  (4 << 5)
#define BQ2589X_IR_BAT_COMP_60MOHM  (3 << 5)
#define BQ2589X_IR_BAT_COMP_40MOHM  (2 << 5)
#define BQ2589X_IR_BAT_COMP_20MOHM  (1 << 5)
#define BQ2589X_IR_BAT_COMP_0MOHM   (0 << 5)
#define BQ2589X_IR_VCLAMP_224MV     (7 << 2)
#define BQ2589X_IR_VCLAMP_192MV     (6 << 2)
#define BQ2589X_IR_VCLAMP_160MV     (5 << 2)
#define BQ2589X_IR_VCLAMP_128MV     (4 << 2)
#define BQ2589X_IR_VCLAMP_96MV      (3 << 2)
#define BQ2589X_IR_VCLAMP_64MV      (2 << 2)
#define BQ2589X_IR_VCLAMP_32MV      (1 << 2)
#define BQ2589X_IR_VCLAMP_0MV       (0 << 2)
#define BQ2589X_IR_TREG_120C        (3 << 0)
#define BQ2589X_IR_TREG_100C        (2 << 0)
#define BQ2589X_IR_TREG_80C         (1 << 0)
#define BQ2589X_IR_TREG_60C         (0 << 0)
#define BQ2589X_IR_COMP_DEFAULT (BQ2589X_IR_TREG_120C | BQ2589X_IR_VCLAMP_0MV |\
		BQ2589X_IR_BAT_COMP_0MOHM)
#define BQ2589X_TERM_CURRENT_LIMIT_DEFAULT 256
  /* 5V VBUS Boost settings */
#define BQ2589X_BOOSTV_MV(mv)       (((((mv) - 4550)/64) & 0xF) << 4)
#define BQ2589X_BOOSTV_DEFAULT      BQ2589X_BOOSTV_MV(4998)
#define BQ2589X_BOOST_LIM_500MA     0x00
#define BQ2589X_BOOST_LIM_750MA     0x01
#define BQ2589X_BOOST_LIM_1200MA    0x02
#define BQ2589X_BOOST_LIM_1400MA    0x03
#define BQ2589X_BOOST_LIM_1650MA    0x04
#define BQ2589X_BOOST_LIM_1875MA    0x05
#define BQ2589X_BOOST_LIM_2150MA    0x06
#define BQ2589X_BOOST_LIM_2450MA    0x07
#define BQ2589X_BOOST_LIM_DEFAULT   BQ2589X_BOOST_LIM_1400MA
#define BQ2589X_BOOST_DEFAULT       (BQ2589X_BOOST_LIM_DEFAULT |\
		BQ2589X_BOOSTV_DEFAULT)
  /* REG14: Device ID, reset and ICO status */
#define BQ2589X_DEVICE_ID_MASK      0x38
#define BQ25890_DEVICE_ID           0x18
#define BQ25892_DEVICE_ID           0x00
#define BQ25895_DEVICE_ID           0x38
#define BQ2589X_ID_ICO_OPTIMIZED    0x40
  /* Variant-specific configuration */
#if   defined(CONFIG_CHARGER_BQ25890)
#define BQ2589X_DEVICE_ID    BQ25890_DEVICE_ID
#define BQ2589X_ADDR         (0x6A << 1)
#elif defined(CONFIG_CHARGER_BQ25895)
#define BQ2589X_DEVICE_ID    BQ25895_DEVICE_ID
#define BQ2589X_ADDR         (0x6A << 1)
#elif defined(CONFIG_CHARGER_BQ25892)
#define BQ2589X_DEVICE_ID    BQ25892_DEVICE_ID
#define BQ2589X_ADDR         (0x6B << 1)
#endif



  static I2C_HandleTypeDef   _bq2589x_i2c;            ///< The object to use I2C interface.
  static BQ2589XChargeStatus _bq2589x_charge_status;  ///< The battery charging status.
  static uint16_t            _bq2589x_vbatt_mv;       ///< The battery voltage, in millivolts.
#ifdef BQ2589X_USE_VSYS
  static uint16_t            _bq2589x_vsys_mv;        ///< Vsys, in millivolts.
#endif
#ifdef BQ2589X_USE_VBUS
  static uint16_t            _bq2589x_vbus_mv;        ///< Vbus, in millivolts.
#endif
#ifdef BQ2589X_USE_ICHG
  static uint16_t            _bq2589x_ichg_ma;        ///< Battery charge current, in mA.
#endif
#ifdef BQ2589X_USE_IBUS_LIMIT
  static uint16_t            _bq2589x_ibus_limit_ma;  ///< The input current limit, in mA.
#endif

  static bool bq2589x_set_terminate_current_ma(uint16_t ma);
  static bool bq25890_read_charging_status(    void);

  static bool bq2589x_read_register( uint8_t reg, uint8_t *pu8_value);
  static bool bq2589x_write_register(uint8_t reg, uint8_t  value);


  bool bq2589x_init(void)
  {
    GPIO_InitTypeDef init;
    uint8_t          u8;
    bool             res;

    // Configure GPIOs
    init.Mode  = GPIO_MODE_AF_OD;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    // Reset I2C module and enable clock
    PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _FORCE_RESET)();
    PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _RELEASE_RESET)();
    PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _CLK_ENABLE)();

    // Configure GPIO
    gpio_use_gpios_with_ids(I2C_INTERNAL_SDA_GPIO, I2C_INTERNAL_SCL_GPIO);
    init.Alternate = I2C_INTERNAL_SDA_AF;
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(I2C_INTERNAL_SDA_GPIO, &init.Pin), &init);
    init.Alternate = I2C_INTERNAL_SCL_AF;
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(I2C_INTERNAL_SCL_GPIO, &init.Pin), &init);

    // Configure I2C module stuff
    _bq2589x_i2c.Instance              = PASTER2(I2C, I2C_INTERNAL_ID);
    _bq2589x_i2c.Init.Timing           = I2C_INTERNAL_TIMING;
    _bq2589x_i2c.Init.OwnAddress1      = 0;
    _bq2589x_i2c.Init.AddressingMode   = EXT_I2C_BUS_NBIT_ADDRESS;
    _bq2589x_i2c.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    _bq2589x_i2c.Init.OwnAddress2      = 0;
    _bq2589x_i2c.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    _bq2589x_i2c.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
    _bq2589x_i2c.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
    HAL_I2CEx_ConfigAnalogFilter( &_bq2589x_i2c, I2C_INTERNAL_FILTER_ANALOG);
    HAL_I2CEx_ConfigDigitalFilter(&_bq2589x_i2c, I2C_INTERNAL_FILTER_DIGITAL);
    if(!(res = (HAL_I2C_Init(&_bq2589x_i2c) == HAL_OK))) { goto exit; }

    // Chip configuration
    bq2589x_read_register( BQ2589X_REG_TIMER,      &u8);
    bq2589x_write_register(BQ2589X_REG_TIMER,      0b11001101);
    bq2589x_set_terminate_current_ma(BATT_CHARGER_TERMINATE_CURRENT_MA);
    bq2589x_write_register(BQ2589X_REG_IR_COMP,    BATT_CHARGER_IR_COMP);
    bq2589x_write_register(BQ2589X_REG_BOOST_MODE, BATT_CHARGER_BOOST_MODE);
    bq2589x_read_register( BQ2589X_REG_FAULT,      &u8);
    bq25890_read_charging_status();

    exit:
    return res;
  }

  void bq2589x_deinit(void)
  {
    HAL_I2C_DeInit( &_bq2589x_i2c);
    PASTER3(__HAL_RCC_I2C, I2C_INTERNAL_ID, _CLK_DISABLE)();
    gpio_free_gpios_with_ids(I2C_INTERNAL_SDA_GPIO, I2C_INTERNAL_SCL_GPIO);
  }


  /**
   * Read the charging status.
   *
   * @post _bq2589x_charge_status is updated.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool bq25890_read_charging_status(void)
  {
    uint8_t u8;
    bool    res;

    _bq2589x_charge_status = (res = bq2589x_read_register(BQ2589X_REG_STATUS, &u8)) ?
	(BQ2589XChargeStatus)((u8 >> 3) & 0x03) :
	BQ2589X_NOT_CHARGING;

    return res;
  }

  /**
   * Read values from the charger.
   *
   * @param[in] values the ORed combination of the values to read.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool bq2589x_read(BQ2589XValues values)
  {
    uint32_t tref;
    uint8_t  value, cfg2_bak;
    bool     res;
    bool     sys_min_changed = false;

    // Clear variables
    _bq2589x_vbatt_mv = 0;
#ifdef BQ2589X_USE_VSYS
    _bq2589x_vsys_mv  = 0;
#endif
#ifdef BQ2589X_USE_VBUS
    _bq2589x_vbus_mv  = 0;
#endif
#ifdef BQ2589X_USE_ICHG
    _bq2589x_ichg_ma  = 0;
#endif
#ifdef BQ2589X_USE_IBUS_LIMIT
    _bq2589x_ibus_limit_ma = 0;
#endif

    if(values == BQ2589X_VALUE_NONE) { goto exit; }

    // If read battery voltage then we need to use a workaround.
    // I think that there is a bug in the chip because it cannot read battery voltage if
    // it is lower than SYS_MIN.
    // This information has been added in an errata note.
    // That's why I think it's a bug, and it does not make sense.
    // So before reading battery voltage the SYS voltage is lowered at it's minimum: 3V.
    // This is enough for the system to run and it allows us to read battery voltages down to that value.
    // We can do nothing for lower battery voltages...
    if(values & BQ2589X_VALUE_VBATT)
    {
      // Get current SYS_MIN value
      if(!bq2589x_read_register(BQ2589X_REG_CFG2, &cfg2_bak)) { goto error_exit; }

      // Lower SYS_MIN to minimum value.
      if(!bq2589x_write_register(BQ2589X_REG_CFG2, (cfg2_bak & ~BQ2589X_CFG2_SYS_MIN))) { goto error_exit; }
      sys_min_changed = true;

      // Wait for SYS voltage to settle if it has to.
      board_delay_ms(BQ2589X_SYS_SETTLE_WAIT_MS);
    }

    // Trigger an ADC conversion
    if(!bq2589x_read_register( BQ2589X_REG_CFG1, &value))                           { goto error_exit; }
    if(!bq2589x_write_register(BQ2589X_REG_CFG1,  value | BQ2589X_CFG1_CONV_START)) { goto error_exit; }

    // Wait for end of conversion
    tref = board_ms_now();
    while(1)
    {
      if(!bq2589x_read_register(BQ2589X_REG_CFG1, &value))                 { goto error_exit; }
      if((value & BQ2589X_CFG1_CONV_START) == 0)                           { break;           }
      if(board_is_timeout(tref, BQ2589X_END_OF_ADC_CONVERSION_TIMEOUT_MS)) { goto error_exit; }
      board_delay_ms(10);
    };

    if(values & BQ2589X_VALUE_VBATT)
    {
      // Get battery voltage
      if(!bq2589x_read_register(BQ2589X_REG_ADC_BATT_VOLT,  &value)) { goto error_exit; }
      _bq2589x_vbatt_mv = 2304 + ((uint16_t)(value & 0x7F)) * 20;

      // Read charging status
      if(!bq25890_read_charging_status()) { goto error_exit; }
    }

#ifdef BQ2589X_USE_VSYS
    if(values & BQ2589X_VALUE_VSYS)
    {
      // Read Vsys
      if(!bq2589x_read_register(BQ2589X_REG_ADC_SYS_VOLT,  &value)) { goto error_exit; }
      _bq2589x_vsys_mv = 2304 + ((uint16_t)(value & 0x7F)) * 20;
    }
#endif

#ifdef BQ2589X_USE_VBUS
    if(values & BQ2589X_VALUE_VBUS)
    {
      // Read Vsys
      if(!bq2589x_read_register(BQ2589X_REG_ADC_VBUS_VOLT,  &value)) { goto error_exit; }
      _bq2589x_vbus_mv = 2600 + ((uint16_t)(value & 0x7F)) * 100;
    }
#endif

#ifdef BQ2589X_USE_ICHG
    if(values & BQ2589X_VALUE_ICHG)
    {
      // Read Vsys
      if(!bq2589x_read_register(BQ2589X_REG_ADC_CHG_CURR,  &value)) { goto error_exit; }
      _bq2589x_ichg_ma = ((uint16_t)(value & 0x7F)) * 50;
    }
#endif

#ifdef BQ2589X_USE_IBUS_LIMIT
    if(values & BQ2589X_VALUE_IBUS_LIMIT)
    {
      // Read Vsys
      if(!bq2589x_read_register(BQ2589X_REG_ADC_INPUT_CURR,  &value)) { goto error_exit; }
      _bq2589x_ibus_limit_ma = 100 + ((uint16_t)(value & 0x3F)) * 50;
    }
#endif

    exit:
    res = true;
    goto finally;

    error_exit:
    res = false;

    finally:
    // If SYS_MIN has been modified then set the backed up value
    if(sys_min_changed)
    {
      // Restore whole CFG2 register value
      res &= bq2589x_write_register(BQ2589X_REG_CFG2, cfg2_bak);

      // Wait for SYS voltage to settle if it has to.
      board_delay_ms(BQ2589X_SYS_SETTLE_WAIT_MS);
    }

    return res;
  }


  /**
   * Return the battery voltage, in millivolts.
   *
   * @return the voltage.
   * @return 0 if no battery is connected.
   * @return 0 if the voltage could not be read.
   */
  uint16_t bq2589x_vbat_mv(void)
  {
    return _bq2589x_vbatt_mv;
  }

  /**
   * Return the battery voltage, in Volts.
   *
   * @return the voltage.
   * @return 0 if no battery is connected.
   * @return 0 if the voltage could not be read.
   */
  float bq2589x_vbat_v(void)
  {
    return ((float)_bq2589x_vbatt_mv) / 1000.0;
  }

  /**
   * Set the battery voltage.
   *
   * @param[in] mv the battery voltage, in milliVolts.
   */
  void bq2589x_set_vbat_mv(uint16_t mv)
  {
    _bq2589x_vbatt_mv = mv;
  }

  /**
   * Return the battery level between in a given range.
   *
   * @param[in] min_value   the value to return when the battery is empty.
   * @param[in] max_value   the value to return when the battery is full.
   * @param[in] error_value the value to return when we failed to read the battery level.
   *
   * @return the battery level, in the given value range.
   */
  uint32_t bq2589x_vbat_level(uint32_t min_value, uint32_t max_value, uint32_t error_value)
  {
    uint32_t v = error_value;

    if(_bq2589x_vbatt_mv)
    {
      v  = (_bq2589x_vbatt_mv >= BATT_VOLTAGE_EMPTY_MV) ? _bq2589x_vbatt_mv - BATT_VOLTAGE_EMPTY_MV : 0;
      v  = (v * (max_value - min_value)) / (BATT_VOLTAGE_FULL_MV - BATT_VOLTAGE_EMPTY_MV);
      v += min_value;
    }

    return v;
  }

#ifdef BQ2589X_USE_VSYS
  /**
   * Return Vsys voltage, in millivolts.
   *
   * @return the voltage.
   * @return 0 if the voltage could not be read.
   */
  uint16_t bq2589x_vsys_mv(void) { return _bq2589x_vsys_mv; }

  /**
   * Return Vsys voltage, in Volts.
   *
   * @return the voltage.
   * @return 0 if the voltage could not be read.
   */
  float bq2589x_vsys_v(void) { return ((float)_bq2589x_vsys_mv) / 1000.0; }
#endif

#ifdef BQ2589X_USE_VBUS
  /**
   * Return Vbus voltage, in millivolts.
   *
   * @return the voltage.
   * @return 0 if the voltage could not be read.
   */
  uint16_t bq2589x_vbus_mv(void) { return _bq2589x_vbus_mv; }

  /**
   * Return Vbus voltage, in Volts.
   *
   * @return the voltage.
   * @return 0 if the voltage could not be read.
   */
  float bq2589x_vbus_v(void) { return ((float)_bq2589x_vbus_mv) / 1000.0; }
#endif

#ifdef BQ2589X_USE_ICHG
  /**
   * Return the battery charge current, in mA.
   *
   * @return the current.
   * @return 0 if the current could not be read.
   */
  uint16_t bq2589x_ichg_ma(void) { return _bq2589x_ichg_ma; }

  /**
   * Return the battery charge current, in A.
   *
   * @return the current.
   * @return 0 if the current could not be read.
   */
  float bq2589x_ichg_a(void) { return ((float)_bq2589x_ichg_ma) / 1000.0; }
#endif

#ifdef BQ2589X_USE_IBUS_LIMIT
  /**
   * Return current input current limit, in mA.
   *
   * @return the current.
   * @return 0 if the current could not be read.
   */
  uint16_t bq2589x_ibus_limit_ma(void) { return _bq2589x_ibus_limit_ma; }

  /**
   * Return current input current limit, in A.
   *
   * @return the current.
   * @return 0 if the current could not be read.
   */
  float bq2589x_ibus_limit_a(void) { return ((float)_bq2589x_ibus_limit_ma) / 1000.0; }
#endif

  /**
   * Return the battery charge status.
   *
   * @return the charge status.
   * @return BQ2589X_NOT_CHARGING is no battery is connected.
   * @return BQ2589X_NOT_CHARGING if the status could not be read.
   */
  BQ2589XChargeStatus bq2589x_charge_status(void)
  {
    return _bq2589x_charge_status;
  }

  /**
   * Indicate if the battery is charging or not.
   *
   * @return true  if the battery is charging.
   * @return false if it is not.
   * @return false if the battery is not connected.
   * @return false if failed to read charging status.
   */
  bool bq2589x_is_charging(void)
  {
    return _bq2589x_charge_status != BQ2589X_NOT_CHARGING;
  }


  static bool bq2589x_set_terminate_current_ma(uint16_t ma)
  {
    uint8_t reg_val;
    uint8_t val = (ma - 64) / 64;

    if(!bq2589x_read_register(BQ2589X_REG_PRE_CHG_CURR, &reg_val)) { return false; }
    reg_val = (reg_val & ~0xf) | (val & 0xf);

    return bq2589x_write_register(BQ2589X_REG_PRE_CHG_CURR, reg_val);
  }


  /**
   * Read a register's value from the chip.
   *
   * @param[in]  reg       the register's address.
   * @param[out] pu8_value where the value is written to. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool bq2589x_read_register( uint8_t reg, uint8_t *pu8_value)
  {
    return HAL_I2C_Mem_Read(&_bq2589x_i2c, BQ2589X_I2C_ADDRESS,
			    reg, 1, pu8_value, 1,
			    BQ2589X_I2C_TIMEOUT_MS) == HAL_OK;
  }

  /**
   * Write a value to one of the chip's registers.
   *
   * @param[in] reg   the register's address.
   * @param[in] value the value to write.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool bq2589x_write_register(uint8_t reg, uint8_t value)
  {
    return HAL_I2C_Mem_Write(&_bq2589x_i2c, BQ2589X_I2C_ADDRESS,
			     reg, 1, &value, 1,
			     BQ2589X_I2C_TIMEOUT_MS) == HAL_OK;
  }


#ifdef __cplusplus
}
#endif
