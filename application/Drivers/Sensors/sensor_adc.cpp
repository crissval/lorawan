/*
 * Base class for sensors using the µC ADC line(s)
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 */

#include <string.h>
#include "sensor_adc.hpp"


#define ADC_CONV_TIMEOUT_MS  1000


const SensorADC::ADCLineDesc SensorADC::_adcDescs[SENSOR_ADC_LINES_COUNT] =
{
    {
	ADC_LINE_ANA1, ADC_LINE_FLAG_ANA1, "ANA1",
	ADC_ANA1_GPIO,
	PASTER2(ADC, ADC_ANA1_ADC_ID), PASTER2(ADC_CHANNEL_, ADC_ANA1_ADC_CHANNEL_ID)
    },
    {
	ADC_LINE_ANA2, ADC_LINE_FLAG_ANA2, "ANA2",
	ADC_ANA2_GPIO,
	PASTER2(ADC, ADC_ANA2_ADC_ID), PASTER2(ADC_CHANNEL_, ADC_ANA2_ADC_CHANNEL_ID)
    },
    {
	ADC_LINE_BATV, ADC_LINE_FLAG_BATV, "NodeBatV",
	ADC_BATV_GPIO,
	PASTER2(ADC, ADC_BATV_ADC_ID), PASTER2(ADC_CHANNEL_, ADC_BATV_ADC_CHANNEL_ID)
    }
};



/**
 * Constructor.
 *
 * @param[in] power       the power configuration required when the node is active.
 * @param[in] powerSleep  the power configuration required when the node is asleep.
 * @param[in] adcLines    the identifier flags of the ADC lines used by this sensor.
 */
SensorADC::SensorADC(Power power, Power powerSleep, ADCLineFlags adcLines) :
    Sensor(power, powerSleep, FEATURE_BASE)
{
  setADCLines(adcLines);
}

/**
 * Constructor.
 *
 * @param[in] power       the power configuration required when the node is active.
 * @param[in] powerSleep  the power configuration required when the node is asleep.
 * @param[in] adcLine     the identifier the ADC line used by this sensor.
 */
SensorADC::SensorADC(Power power, Power powerSleep, ADCLine adcLine) :
    Sensor(power, powerSleep, FEATURE_BASE)
{
  setADCLine(adcLine);
}

/**
 * Common initialisations to all constructors.
 *
 * @param[in] adcLines the identifier flags of the ADC lines used by this sensor
 */
void SensorADC::init(ADCLineFlags adcLines)
{
  uint8_t i;

  this->_adcLines = adcLines;

  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    this->_adcValuesMV[i]    = 0;
    this->_hadcs[i].Instance = NULL;
  }
}

SensorADC::~SensorADC()
{
  // Do nothing
}

/**
 * Set the ADC line(s) to use.
 *
 * @param[in] adcLines the identifier flags of the ADC lines used by this sensor.
 */
void SensorADC::setADCLines(ADCLineFlags adcLines)
{
  init(adcLines);
}

/**
 * Set the ADC line to use.
 *
 * @param[in] adcLine the identifier the ADC line used by this sensor.
 */
void SensorADC::setADCLine(ADCLine adcLine)
{
  uint8_t i;

  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    if(_adcDescs[i].id == adcLine)
    {
      init(_adcDescs[i].idFlag);
      break;
    }
  }
}


/**
 * Return an ADC line description using the line's name.
 *
 * The name comparison is not case sensitive.
 *
 * @param[in] psName the name.
 *
 * @return the line description.
 * @return NULL if no description has been found for the given name.
 */
const SensorADC::ADCLineDesc *SensorADC::lineDescUsingName(const char *psName)
{
  uint8_t i;

  if(!psName || !*psName) { goto error_exit; }

  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    if(strcasecmp(psName, _adcDescs[i].psName) == 0) { return  &_adcDescs[i]; }
  }

  error_exit:
  return NULL;
}


/**
 * Get the ADC handle object for a given ADC line description.
 *
 * @param[in] line   the ADC line identifier.
 * @param[in] create create a handle object if none is found matching in the current list?
 *
 * @return the handle object.
 * @return NULL if no handle object found.
 */
ADC_HandleTypeDef *SensorADC::getADCHandleForLineDesc(const ADCLineDesc *pvDesc, bool create)
{
  uint8_t i;

  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    if(this->_hadcs[i].Instance == pvDesc->pvADC) { return &this->_hadcs[i]; }
  }

  if(create)
  {
    for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
    {
      if(!this->_hadcs[i].Instance) { return &this->_hadcs[i]; }
    }
  }

  return NULL;
}


bool SensorADC::openSpecific()
{
  ADC_MultiModeTypeDef multimode;
  GPIO_InitTypeDef     init;
  ADC_HandleTypeDef    *pvADC;
  uint8_t              i;

  __HAL_RCC_ADC_CLK_ENABLE();

  // Base configuration for all GPIOs
  init.Mode  = GPIO_MODE_ANALOG_ADC_CONTROL;
  init.Pull  = GPIO_NOPULL;
  init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

  // Base configuration for all ADCs
  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    const ADCLineDesc *pvDesc = &_adcDescs[i];
    if(!(pvDesc->idFlag & this->_adcLines)) continue;

    // Set GPIO
    gpio_use_gpio_with_id(pvDesc->gpio);
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(pvDesc->gpio, &init.Pin), &init);

    // Set up the ADC
    pvADC = getADCHandleForLineDesc(pvDesc, true);  // This should never return NULL because the table is big enough.
    if(pvADC->Instance) continue;                   // ADC already has been initialised.
    pvADC->Instance                   = pvDesc->pvADC;
    pvADC->Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    pvADC->Init.Resolution            = ADC_RESOLUTION_12B;
    pvADC->Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    pvADC->Init.ScanConvMode          = ADC_SCAN_DISABLE;
    pvADC->Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    pvADC->Init.LowPowerAutoWait      = DISABLE;
    pvADC->Init.ContinuousConvMode    = DISABLE;
    pvADC->Init.NbrOfConversion       = 1;
    pvADC->Init.DiscontinuousConvMode = DISABLE;
    pvADC->Init.NbrOfDiscConversion   = 1;
    pvADC->Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    pvADC->Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    pvADC->Init.DMAContinuousRequests = DISABLE;
    pvADC->Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    pvADC->Init.OversamplingMode      = DISABLE;
    multimode.Mode                    = ADC_MODE_INDEPENDENT;
    HAL_ADC_Init(                    pvADC);
    HAL_ADCEx_MultiModeConfigChannel(pvADC, &multimode);

    // Start ADC calibration
    HAL_ADCEx_Calibration_Start(pvADC, ADC_SINGLE_ENDED);
  }

  return true;
}

/**
 * Close ADC.
 */
void SensorADC::closeSpecific()
{
  ADC_HandleTypeDef *pvADC;
  uint8_t            i;

  // Base configuration for all ADCs
  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    const ADCLineDesc *pvDesc = &_adcDescs[i];
    if(!(pvDesc->idFlag & this->_adcLines)) continue;

    gpio_free_gpio_with_id(pvDesc->gpio);

    pvADC = getADCHandleForLineDesc(pvDesc, false);
    if(!pvADC || !pvADC->Instance) continue;
    ADC_Disable(                        pvADC);
    HAL_ADC_DeInit(                     pvADC);
    HAL_ADCEx_EnterADCDeepPowerDownMode(pvADC);
    pvADC->Instance = NULL;
  }

  __HAL_RCC_ADC_CLK_DISABLE();
}

/**
 * Read the values.
 */
bool SensorADC::readSpecific()
{
  ADC_HandleTypeDef     *pvADC;
  ADC_ChannelConfTypeDef channelConfig;
  uint8_t                i;
  uint32_t               v;

  // Base configuration for all ADCs
  for(i = 0; i < SENSOR_ADC_LINES_COUNT; i++)
  {
    const ADCLineDesc *pvDesc = &_adcDescs[i];
    if(!(pvDesc->idFlag & this->_adcLines)) continue;

    pvADC = getADCHandleForLineDesc(pvDesc, false);
    if(!pvADC || !pvADC->Instance) continue;

    // Configure the channel
    channelConfig.Channel      = pvDesc->adcChannel;
    channelConfig.Rank         = ADC_REGULAR_RANK_1;
    channelConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    channelConfig.SingleDiff   = ADC_SINGLE_ENDED;
    channelConfig.OffsetNumber = ADC_OFFSET_NONE;
    channelConfig.Offset       = 0;

    // Start conversion
    if( HAL_ADC_ConfigChannel(    pvADC, &channelConfig)      != HAL_OK ||
	HAL_ADC_Start(            pvADC)                      != HAL_OK ||
	HAL_ADC_PollForConversion(pvADC, ADC_CONV_TIMEOUT_MS) != HAL_OK) { goto error_exit; }

    // Compute voltage
    v = HAL_ADC_GetValue(pvADC);
    this->_adcValuesMV[pvDesc->id - 1] = (v * VDDA_VOLTAGE_MV) / 4095;
  }

  return true;

  error_exit:
  return false;
}
