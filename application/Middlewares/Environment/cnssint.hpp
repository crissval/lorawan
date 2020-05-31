/*
 * Header for the ConnecSenS interruptions handler.
 *
 *  Created on: 23 mai 2018
 *      Author: jfuchet
 */
#ifndef ENVIRONMENT_CNSSINT_HPP_
#define ENVIRONMENT_CNSSINT_HPP_

#include "defs.h"
#include "config.h"
#include "json.hpp"
#include "gpio.h"

class CNSSIntClient;  // So that we can use it.


#ifndef CNSSIT_CLIENT_COUNT_MAX
#define CNSSIT_CLIENT_COUNT_MAX 20
#endif

class CNSSInt
{
public:
  /**
   * Defines the interruption identifiers.
   */
  typedef enum IntId
  {
    INT1,
    INT2,
    OPTO1,
    OPTO2,
    LIS3DH,
    LPS25,
    OPT3001,
    SHT35,
    SPI,
    I2C,
    USART,
    SDI12,
    INTERRUPTION_COUNT,  ///< Not an actual identifier, used to know the number of interruptions
    INT_NONE
  }
  IntId;

  /**
   * Defines the interruption flags
   */
  typedef enum IntFlag
  {
    INT_FLAG_NONE     = 0,
    INT1_FLAG         = 1u << INT1,
    INT2_FLAG         = 1u << INT2,
    OPTO1_FLAG        = 1u << OPTO1,
    OPTO2_FLAG        = 1u << OPTO2,
    LIS3DH_FLAG       = 1u << LIS3DH,
    LPS25_FLAG        = 1u << LPS25,
    OPT3001_FLAG      = 1u << OPT3001,
    SHT35_FLAG        = 1u << SHT35,
    SPI_FLAG          = 1u << SPI,
    I2C_FLAG          = 1u << I2C,
    USART_FLAG        = 1u << USART,
    SDI12_FLAG        = 1u << SDI12
  }
  IntFlag;
  typedef uint32_t Interruptions;  ///< The type used to store interruption(s)


private:
  /**
   * Store the informations for an interruption client.
   */
  typedef struct Client
  {
    CNSSIntClient *pvClient;        ///< Pointer to the client.
    Interruptions  sensitivityMask; ///< Or ORed combination of IntFlag the client is sensitive to.
  }
  Client;

  /**
   * Defines an alias for an interruption name
   */
  typedef struct Alias
  {
    const char *psAlias;   ///< The alias
    const char *psIntName; ///< The corresponding interruption name. MUST be NOT NULL.
  }
  Alias;

  /**
   * Used a store an interrupt line configuration
   */
  typedef struct IntConfig
  {
    const char *psName;    ///< The interrupt line's name
    IntId       id;        ///< The interrupt line's identifier.
    IntFlag     flag;      ///< The interrupt line's associated interruption flag
    bool        internal;  ///< Internal or external interruption?
    GPIOId      gpio;      ///< The GPIO the interrupt line is built with.
  }
  IntConfig;

  /**
   * Store the parameters for an interruption
   */
  typedef struct IntParams
  {
    IntFlag   flag;               ///< The flag associated with the interruption.
    uint32_t  debounceMs;         ///< The debounce time, in milliseconds, to use. Set to 0 to turn debouncing off.
    uint32_t  lastActivationTsMs; ///< Last activation timestamp, in milliseconds.
  }
  IntParams;


public:
  static CNSSInt *instance();

  bool setConfiguration(const JsonArray& json);
  bool setDebounce(     IntId            id, uint32_t ms);

  void sleep();

  bool    registerClient(  CNSSIntClient& client, Interruptions sensitivityMask, bool append = true);
  IntFlag registerClient(  CNSSIntClient& client, const char   *psIntName,       bool append = true);
  void    unregisterClient(CNSSIntClient& client);
  bool    processInterruptions();
  void    clearInternalInterruptions();
  void    clearExternalInterruptions();
  void    clearAllInterruptions();

  bool    isSensitiveToExternalInterruption() { return this->_sensitiveToExternalInterruption; }
  bool    isSensitiveToInternalInterruption() { return this->_sensitiveToInternalInterruption; }

  static IntFlag     getFlagUsingName(         const char   *psName);
  static IntId       getIdUsingName(           const char   *psName);
  static const char *getNameUsingFlag(         IntFlag       flag);
  static const char *getNameUsingId(           IntId         id);
  static bool        containsExternalInterrupt(Interruptions ints);
  static bool        containsInternalInterrupt(Interruptions ints);


private:
  CNSSInt();
  void initInterruptLines();
  void clearInterruptions(GPIOId readGPIO, GPIOId clearGPIO, int32_t clearLevel);
  bool setConfigurationForInterruption(      const char *psName, const JsonObject &json);
  static const char *     resolveAlias(      const char *psName);
  static const IntConfig *getConfigUsingName(const char *psName);
  static void             USBIrqHandler(     void);
  static void             dummyIrqHandler(   void);


private:
  Client    _clients[CNSSIT_CLIENT_COUNT_MAX]; ///< The list of clients.
  uint32_t  _nbClients;                        ///< The number of registered clients.
  IntParams _intParams[INTERRUPTION_COUNT];    ///< The interruption parameters, in the same order as their identifiers.
  bool      _sensitiveToExternalInterruption;  ///< Are we sensitive to external interruptions?
  bool      _sensitiveToInternalInterruption;  ///< Are we sensitive to internal interruptions?

  static const Alias     _aliases[];                          ///< The lists of aliases. End of list indicated with a NULL psAlias.
  static const IntConfig _intConfigs[INTERRUPTION_COUNT + 1]; ///< The list of interrupt configuration. There is a configuration with a NULL psName at the end of the table.
  static CNSSInt        *_pvInstance;                         ///< The unique instance of this class.
};



#endif /* ENVIRONMENT_CNSSINT_HPP_ */
