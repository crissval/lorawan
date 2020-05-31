/**
 * Class for SPI communication.
 *
 * @file spi.hpp
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2019
 */
#ifndef _SPI_H_
#define _SPI_H_
#include "board.h"
#include "json.hpp"
#include "extensionport.h"

class SPI
{
  public:
  /**
   * Define the SPI sensor configuration options.
   */
  typedef enum Option
  {
    OPTION_NONE,
    OPTION_MASTER           = 1u  << 0,  ///< The node is the bus master.
    OPTION_SHIFT_MSB_FIRST  = 1u  << 1,  ///< MSB is shifted out first.
    OPTION_SHIFT_LSB_FIRST  = 0,         ///< LSB is shifted out first.
    OPTION_CPHA_0           = 0,         ///< Use CPHA = 0.
    OPTION_CPHA_1           = 1u  << 2,  ///< Use CPHA = 1.
    OPTION_CPOL_0           = 0,         ///< Use CPOL = 0.
    OPTION_CPOL_1           = 1u  << 3,  ///< Use CPOL = 1.
    OPTION_MODE_0           = OPTION_CPOL_0 | OPTION_CPHA_0,  ///< Use SPI mode 0.
    OPTION_MODE_1           = OPTION_CPOL_0 | OPTION_CPHA_1,  ///< Use SPI mode 1.
    OPTION_MODE_2           = OPTION_CPOL_1 | OPTION_CPHA_0,  ///< Use SPI mode 2.
    OPTION_MODE_3           = OPTION_CPOL_1 | OPTION_CPHA_1,  ///< Use SPI mode 3.
    OPTION_FULL_DUPLEX      = 0,         ///< Both MOSI and MISO are used simultaneously.
    OPTION_HALF_DUPLEX      = 1u  << 4,  ///< Rx and Tx use the same data line; cannot Rx and Tx at the same time.
    OPTION_SIMPLEX          = 1u  << 5,  ///< Can only Tx or Rx, use a single data line.
    OPTION_USE_MOSI         = 1u  << 6,  ///< Use the MOSI signal/pin.
    OPTION_USE_MISO         = 1u  << 7,  ///< Use the MISO signal/pin.
    OPTION_DATA_SIZE_4BITS  = 0x3 << 8,  ///< Data bytes are 4 bits long.
    OPTION_DATA_SIZE_5BITS  = 0x4 << 8,  ///< Data bytes are 5 bits long.
    OPTION_DATA_SIZE_6BITS  = 0x5 << 8,  ///< Data bytes are 6 bits long.
    OPTION_DATA_SIZE_7BITS  = 0x6 << 8,  ///< Data bytes are 7 bits long.
    OPTION_DATA_SIZE_8BITS  = 0x7 << 8,  ///< Data bytes are 8 bits long.
    OPTION_DATA_SIZE_9BITS  = 0x8 << 8,  ///< Data bytes are 9 bits long.
    OPTION_DATA_SIZE_10BITS = 0x9 << 8,  ///< Data bytes are 10 bits long.
    OPTION_DATA_SIZE_11BITS = 0xA << 8,  ///< Data bytes are 11 bits long.
    OPTION_DATA_SIZE_12BITS = 0xB << 8,  ///< Data bytes are 12 bits long.
    OPTION_DATA_SIZE_13BITS = 0xC << 8,  ///< Data bytes are 13 bits long.
    OPTION_DATA_SIZE_14BITS = 0xD << 8,  ///< Data bytes are 14 bits long.
    OPTION_DATA_SIZE_15BITS = 0xE << 8,  ///< Data bytes are 15 bits long.
    OPTION_DATA_SIZE_16BITS = 0xF << 8   ///< Data bytes are 16 bits long.
  }
  Option;
  typedef uint16_t Options;  ///< Type used to store a ORed combination of Option values.
#define SENSOR_SPI_OPTION_DATA_SIZE_MASK  0x0F00  ///< The mask to use to get the data size from the options.
#define SENSOR_SPI_OPTION_DATA_SIZE_POS   8       ///< The position, in bits, of the data size value in the options.

  /**
   * Define the flag identifiers for the default slave selects.
   * Can be extended in sub-classes to define new slave select signals.
   */
  typedef enum SSIdFlag
  {
    SS_IDFLAG_NONE    = 0,        ///< No SS.
    SS_IDFLAG_DEFAULT = 1u << 0,  ///< Default SPI SS.
    /**
     * The base flag identifier to use to define new SS flag identifiers in sub-classes.
     * First identifier is SS_IDFLAG_SUBCLASS.
     * Second identifier is( SS_IDFLAG_SUBCLASS << 1).
     * Third identifier is( SS_IDFLAG_SUBCLASS << 2).
     * And so on...
     */
    SS_IDFLAG_SUBCLASS = 1u << 1
  }
  SSIdFlag;

  /**
   * Define basic slaves identifiers.
   * Sub-classes can define new ones.
   *
   * Their value is made of a ORed combination of #SSIdFlag values.
   * It is thus possible to control a single or multiple SS GPIOs using a single identifier.
   * The identifier can be used to describe and control and address decoder.
   * For example we could use 2 GPIOs to control 3 slaves using an address decoder.
   * Each #SSIdFlag would represent one of the GPIOs.
   * Say we have these SS flags defined:
   * <ul>
   * 	<li>SS_IDFLAG_BIT0 = SS_IDFLAG_SUBCLASS</li>
   * 	<li>SS_IDFLAG_BIT1 = SS_IDFLAG_SUBCLASS &lt;&lt; 1</li>
   * </ul>
   * Then we could define the following identifiers:
   * <ul>
   * 	<li>SS_IS_SLAVE1 = SS_IDFLAG_BIT0</li>
   * 	<li>SS_IS_SLAVE2 = SS_IDFLAG_BIT1</li>
   * 	<li>SS_IS_SLAVE3 = SS_IDFLAG_BIT1 | SS_IDFLAG_BIT0</li>
   * </ul>
   * #SS_IDFLAG_NONE is used to de-select all slaves.
   */
  typedef enum SSId
  {
    SS_ID_NONE    = SS_IDFLAG_NONE,     ///< No slave to select.
    SS_ID_DEFAULT = SS_IDFLAG_DEFAULT   ///< Select the slave activated by the default SS signal.
  }
  SSId;

  /**
   * Store information about a slave select (SS) signal (GPIO pin).
   */
  typedef struct SSInfos
  {
    const char *psConfigKeyBaseName;  ///< The base name for the SS JSON configuration keys.
    SSIdFlag   idFlag;                ///< The SS flag identifier.
    bool       activeLow;             ///< Is the SS signal active when low (true) or high (false).
  }
  SSInfos;


private:
  /**
   * Store information on a slave select (SS) signal.
   */
  typedef struct SSConfig
  {
    const SSInfos    *pvInfos;  ///< The SS informations.
    const ExtPortPin *pvPin;    ///< The extension port's pin to use as SS signal.
  }
  SSConfig;


public:
  SPI(uint32_t       sckFreqHz,
      Options        options,
      const SSInfos *ptSSInfos = DEFAULT_SS_INFOS);
  ~SPI();

  bool initUsingJSON(   const JsonObject& json);
  bool initUsingOptions(uint32_t sckFreq = 0, Options  options = OPTION_NONE);
  bool open();
  void close();

  void selectSlave( SSId id = SS_ID_DEFAULT);
  bool write(       const uint8_t *pu8Data, uint16_t size, uint32_t timeoutMs);
  bool read(        uint8_t       *pu8Data, uint16_t nb,   uint32_t timeoutMs);
  bool writeAndRead(const uint8_t *pu8TxData,
		    uint8_t       *pu8RxData,
		    uint16_t       size,
		    uint32_t       timeoutMs);

  bool switchMOSIToGPIOInput(bool       asGPIO = true);
  bool switchMISOToGPIOInput(bool       asGPIO = true);
  bool waitForMOSItoGo(      LogicLevel level, uint32_t timeoutMs);
  bool waitForMISOtoGo(      LogicLevel level, uint32_t timeoutMs);


public:
  static const SSInfos DEFAULT_SS_INFOS[2];  ///< The slave select infos for the default slave select signal.


private:
  const char       *psLogName;        ///< The log name to use.
  SPI_HandleTypeDef _spi;             ///< STM32 HAL SPI object.
  uint32_t          _sckFreqHz;       ///< The SCK signal frequency, in Hz.
  Options           _options;         ///< The configuration options.
  SSConfig         *_ptSSConfigs;     ///< The slave select configuration(s).
  uint8_t           _ssConfigsCount;  ///< The number of slave select configurations. Can be 0 if no slave select is used.
};

#endif // _SPI_H_
