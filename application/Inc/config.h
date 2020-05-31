/*****************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************/
/* Configuration file for ConnecSenS System **********************************************************************************************************************/
/*****************************************************************************************************************************************************************/
/*****************************************************************************************************************************************************************/


#define CURRENT_YEAR_MIN  2019  // The current year minimum value

#define BATT_VOLTAGE_FULL_V           4.2
#define BATT_VOLTAGE_EMPTY_V          2.5
#define BATT_VOLTAGE_LOW_THRESHOLD_V  3.2  // Volts


#define USE_SENSOR_RAIN_GAUGE_CONTACT
#define USE_SENSOR_SOIL_MOISTURE_WATERMARK_I2C
#define USE_SENSOR_INSITU_AQUATROLL_200_SDI12
#define USE_SENSOR_BATTERY_ADC
#define USE_SENSOR_GILL_MAXIMET_SDI12
#define USE_SENSOR_ALGADE_AERTT_SERIAL
#define USE_SENSOR_TRUEBNER_SMT100_SDI12
#define USE_SENSOR_DIGITAL_INPUT

#define USE_DATALOG_CNSSRF_INDEXES


// TODO: remove these confiuration parameters; are they really usefull?
/* Digital Input Output Configuration ****************************************************************************************************************************/
// EXT_GPIO_1
#define		EXT_GPIO1_MODE				GPIO_MODE_OUTPUT_PP			//[GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_OD,GPIO_MODE_OUTPUT_PP]
#define		EXT_GPIO1_PULL				GPIO_PULLUP					//[GPIO_NOPULL,GPIO_PULLUP,GPIO_PULLDOWN]
#define		EXT_GPIO1_SPEED_FREQ 		GPIO_SPEED_FREQ_VERY_HIGH	//[GPIO_SPEED_FREQ_LOW,GPIO_SPEED_FREQ_MEDIUM,GPIO_SPEED_FREQ_HIGH,GPIO_SPEED_FREQ_VERY_HIGH]
// EXT_GPIO_2
#define		EXT_GPIO2_MODE				GPIO_MODE_OUTPUT_PP			//[GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_OD,GPIO_MODE_OUTPUT_PP]
#define		EXT_GPIO2_PULL				GPIO_NOPULL					//[GPIO_NOPULL,GPIO_PULLUP,GPIO_PULLDOWN]
#define		EXT_GPIO2_SPEED_FREQ 		GPIO_SPEED_FREQ_VERY_HIGH	//[GPIO_SPEED_FREQ_LOW,GPIO_SPEED_FREQ_MEDIUM,GPIO_SPEED_FREQ_HIGH,GPIO_SPEED_FREQ_VERY_HIGH]
// EXT_GPIO_3
#define		EXT_GPIO3_MODE				GPIO_MODE_OUTPUT_PP			//[GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_OD,GPIO_MODE_OUTPUT_PP]
#define		EXT_GPIO3_PULL				GPIO_NOPULL					//[GPIO_NOPULL,GPIO_PULLUP,GPIO_PULLDOWN]
#define		EXT_GPIO3_SPEED_FREQ 		GPIO_SPEED_FREQ_VERY_HIGH	//[GPIO_SPEED_FREQ_LOW,GPIO_SPEED_FREQ_MEDIUM,GPIO_SPEED_FREQ_HIGH,GPIO_SPEED_FREQ_VERY_HIGH]
// EXT_GPIO_4
#define		EXT_GPIO4_MODE				GPIO_MODE_OUTPUT_PP			//[GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_OD,GPIO_MODE_OUTPUT_PP]
#define		EXT_GPIO4_PULL				GPIO_NOPULL					//[GPIO_NOPULL,GPIO_PULLUP,GPIO_PULLDOWN]
#define		EXT_GPIO4_SPEED_FREQ 		GPIO_SPEED_FREQ_VERY_HIGH	//[GPIO_SPEED_FREQ_LOW,GPIO_SPEED_FREQ_MEDIUM,GPIO_SPEED_FREQ_HIGH,GPIO_SPEED_FREQ_VERY_HIGH]
// EXT_GPIO_5
#define		EXT_GPIO5_MODE				GPIO_MODE_OUTPUT_PP			//[GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_OD,GPIO_MODE_OUTPUT_PP]
#define		EXT_GPIO5_PULL				GPIO_NOPULL					//[GPIO_NOPULL,GPIO_PULLUP,GPIO_PULLDOWN]
#define		EXT_GPIO5_SPEED_FREQ 		GPIO_SPEED_FREQ_VERY_HIGH	//[GPIO_SPEED_FREQ_LOW,GPIO_SPEED_FREQ_MEDIUM,GPIO_SPEED_FREQ_HIGH,GPIO_SPEED_FREQ_VERY_HIGH]

/* I2C External Bus Configuration ********************************************************************************************************************************/
#define		EXT_I2C_BUS_NBIT_ADDRESS 	I2C_ADDRESSINGMODE_7BIT		//[I2C_ADDRESSINGMODE_7BIT,I2C_ADDRESSINGMODE_10BIT]

/* SPI External Bus Configuration ********************************************************************************************************************************/
#define		EXT_SPI_BUS_BAUDRATE_PRSC 	SPI_BAUDRATEPRESCALER_64	//[SPI_BAUDRATEPRESCALER_2,SPI_BAUDRATEPRESCALER_4,SPI_BAUDRATEPRESCALER_8,SPI_BAUDRATEPRESCALER_16,SPI_BAUDRATEPRESCALER_32,SPI_BAUDRATEPRESCALER_64,SPI_BAUDRATEPRESCALER_128,SPI_BAUDRATEPRESCALER_256]
#define		EXT_SPI_BUS_POLARITY	 	SPI_POLARITY_HIGH			//[SPI_POLARITY_LOW,SPI_POLARITY_HIGH]
#define		EXT_SPI_BUS_PHASE 			SPI_PHASE_2EDGE				//[SPI_PHASE_1EDGE,SPI_PHASE_2EDGE]
#define 	EXT_SPI_BUS_DATASIZE		SPI_DATASIZE_8BIT			//[SPI_DATASIZE_4BIT,SPI_DATASIZE_5BIT,SPI_DATASIZE_6BIT,SPI_DATASIZE_7BIT,SPI_DATASIZE_8BIT,SPI_DATASIZE_9BIT,SPI_DATASIZE_10BIT,SPI_DATASIZE_11BIT,SPI_DATASIZE_12BIT,SPI_DATASIZE_13BIT,SPI_DATASIZE_14BIT,SPI_DATASIZE_15BIT,SPI_DATASIZE_16BIT]
#define		EXT_SPI_BUS_MSB_LSB_FIRST	SPI_FIRSTBIT_MSB			//[SPI_FIRSTBIT_MSB,SPI_FIRSTBIT_LSB]

/* USART External Bus Configuration ******************************************************************************************************************************/
#define		EXT_UART_BUS_BAUDRATE		9600						//


//==================== Directories and files  ===========
#define ENV_DIRECTORY_NAME		"env"
#define LOG_DIRECTORY_NAME		"log"
#define OUTPUT_DIR                      "output"
#define PRIVATE_DATA_DIRECTORY_NAME     ENV_DIRECTORY_NAME "/private"
#define STATE_FILE_NAME			ENV_DIRECTORY_NAME "/state.json"
#define CONFIG_FILE_NAME		ENV_DIRECTORY_NAME "/config.json"
#define SLEEP_FILE_NAME			ENV_DIRECTORY_NAME "/sleep.json"
#define RTC_PRIVATE_DIR_NAME            PRIVATE_DATA_DIRECTORY_NAME "/rtc"


//==================== Logging ==========================
//#define LOGGER_DISABLE_LEVEL_TRACE


//==================== Sensors ==========================

#define SENSOR_STATE_DIR     PRIVATE_DATA_DIRECTORY_NAME "/sensors"

//===================== Datalog files ===================
#define DATALOG_DIR          ENV_DIRECTORY_NAME "/datalogs"
#define DATALOG_FILE_EXT     ".dlog"

//#define DATALOG_CNSSRF_FILE  DATALOG_DIR "/sensors.cnssrf" DATALOG_FILE_EXT
#define DATALOG_CNSSRF_FILENAME    "sensors.cnssrf" DATALOG_FILE_EXT
#define DATALOG_CNSSRF_NB_RECORDS                                        20000
#define DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME          20
#define DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES  16


//==================== RTC ==========================
#define RTC_CAL_PPM_VALUE_CAP_12PF   (-80)  // ppm. For RTC crystal capacitors of 12 pF.
#define RTC_CAL_PPM_VALUE_CAP_20PF   (-10)  // ppm. For RTC crystal capacitors of 20 pF.
//#define RTC_OUTPUT_CLK_ON_MCO  // Define this to output RTC clock on MCO pin.


//==================== SD Card =====================
#define SDCARD_USE_DMA


// Serial debug
#define DEBUG_SERIAL_BAUDRATE 115200

#define SYSLOG_DIRECTORY_NAME         LOG_DIRECTORY_NAME "/syslog"
#define SYSLOG_FILE_NAME_PREFIX       "sys"
#define SYSLOG_FILE_NAME              SYSLOG_FILE_NAME_PREFIX ".log"
#define SYS_LOG_FILE_SIZE_MAX          10000000  // 10 Mo
#define SYS_LOG_FILES_TOTAL_SIZE_MAX  100000000  // 100 Mo


  //==================== SDI-12 configuration ====================
#define SDI12_MANAGER_NB_INTERFACES_MAX       1
#define SDI12_INTERFACE_NAME                  "sdi12"
#define SDI12_PERIOD_MIN_BETWEEN_COMMANDS_MS  0

#define SDI12_DO_NOT_PROCESS_IN_CALLBACKS

#define SDI12_MGR_ALL_DATA_BUFFER_SIZE     200
#define SDI12_GEN_NB_SENSOR_CMD_DESCS_MAX  20
#define SDI12_GEN_MGR_DO_NOT_PROCESS_IN_CALLBACKS
#define SDI12_GEN_MGR_DELAY_BETWEEN_SENDS_MS  200

  /**
   * The break duration, in milliseconds to use.
   * The specification requires a break of at least 12 ms.
   * Let's take some margin; a couple more milliseconds,
   * their impact will be negligible on the communication speed.
   */
#define SDI12_BREAK_DURATION_MS  20


//=================== RF frames ==================================
#define RF_FRAMES_ADD_GEOPOS_TO_EACH   // Define this to add geograpical position to each RF frame


//=================== CSV output data ============================
#define OUTPUT_DATA_CSV_SEP                ';'
#define OUTPUT_DATA_CSV_SEP_STR            ";"
#define OUTPUT_DATA_CSV_HEADER_PREPEND_SEP '_'
#define OUTPUT_DATA_CSV_DIR                OUTPUT_DIR "/csv"
#define OUTPUT_DATA_CSV_BUFFER_SIZE        1024
#define OUTPUT_DATA_CSV_FILE_EXT           ".csv"
#define OUTPUT_DATA_CSV_EOL_STR            "\r\n"


//=================== Campaign working mode
#define CAMPAIGN_DIRECTORY_NAME         "campaign"
#define CAMPAIGN_RANGE_DIRECTORY_NAME   CAMPAIGN_DIRECTORY_NAME "/" "range"

