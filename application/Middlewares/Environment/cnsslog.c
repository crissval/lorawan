/*
 * Initialisation of the ConnecSenS log sub-system.
 *
 * @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 * @date   2019
 */

#include <string.h>
#include "cnsslog.h"
#include "config.h"
#include "logger.h"
#include "usart.h"
#include "sdcard.h"
#include "utils.h"


#ifdef __cplusplus
extern "C" {
#endif


  /**
   * The logging configuration to use.
   */
  static LoggingConfiguration _cnsslog_config =
  {
      NULL,         // pf_init
      LOG_DEFAULT,  // default_log_level
      false,        // print_file_infos
      NULL          // pf_get_log_level
  };

#define SYSLOG_FILE  SYSLOG_DIRECTORY_NAME "/" SYSLOG_FILE_NAME

  /**
   * Defines the file logger backend.
   */
  typedef struct CNSSLoggerBackendFile
  {
    LoggerBackend base;            ///< The logger back-end interface.
    File          file;            ///< The file object.
    bool          file_is_opened;  ///< Indicate if the file is opened or not.
  }
  CNSSLoggerBackendFile;

  static void cnsslog_backend_file_write(LoggerBackend *pv_backend,
					 const uint8_t *pu8_data,
					 uint16_t       size);
  static void cnsslog_sd_log_file_roll(  void);

  static CNSSLoggerBackendFile _cnsslog_backend_file;

  /**
   * Defines the logger serial backend.
   */
  typedef struct CNSSLoggerBackendSerial
  {
    LoggerBackend base;      ///< The logger back-end interface.
    USART        *pv_usart;  ///< The USART used by the back-end.
  }
  CNSSLoggerBackendSerial;

  static void cnsslog_backend_serial_write(LoggerBackend *pv_backend,
					   const uint8_t *pu8_data,
					   uint16_t       size);

  static CNSSLoggerBackendSerial _cnsslog_backend_serial;  ///< The logger back-end used for serial logging.


  /**
   * Initialise logging system.
   */
  void cnsslog_init(void)
  {
    // Init logging system
    logger_init(&_cnsslog_config);

    // Set up file logging
    _cnsslog_backend_file.base.ps_name      = SYSLOG_FILE;
    _cnsslog_backend_file.base.pf_write_log = cnsslog_backend_file_write;
    _cnsslog_backend_file.file_is_opened    = false;
    logger_register_backend((LoggerBackend *)&_cnsslog_backend_file);
  }

  /**
   * Put the logging system to sleep.
   */
  void cnsslog_sleep(void)
  {
    if(_cnsslog_backend_serial.pv_usart)
    {
      usart_sleep(_cnsslog_backend_serial.pv_usart);
    }

    sdcard_sleep();
  }

  /**
   * Wake the logging system up from sleep.
   */
  void cnsslog_wakeup(void)
  {
    sdcard_wakeup();

    if(_cnsslog_backend_serial.pv_usart)
    {
      usart_wakeup(_cnsslog_backend_serial.pv_usart);
    }
  }


  /**
   * Write log data to a file log back-end.
   *
   * @param[in] pv_backend the file back-end to use. Actual type MUST be CNSSFileLoggerBackend.
   * @param[in] pu8_data   the data to write. MUST be NOT NULL.
   * @param[in] size       the number of data bytes to write.
   */
  static void cnsslog_backend_file_write(LoggerBackend *pv_backend,
					 const uint8_t *pu8_data,
					 uint16_t       size)
  {
    CNSSLoggerBackendFile *pv_file_backend = (CNSSLoggerBackendFile *)pv_backend;

    // Open log file if not already done.
    if( !pv_file_backend->file_is_opened &&
	!sdcard_fopen(&pv_file_backend->file, SYSLOG_FILE, FILE_APPEND | FILE_WRITE)) { goto exit; }

    // Write log entry
    sdcard_fwrite(  &pv_file_backend->file, pu8_data, size);
    sdcard_fsync(   &pv_file_backend->file);  // XXX: Maybe do not flush each time; use periodic flush?

    // Check if we need to roll the file
    if(sdcard_fsize(&pv_file_backend->file) >= SYS_LOG_FILE_SIZE_MAX)
    {
      sdcard_fclose(&pv_file_backend->file);
      cnsslog_sd_log_file_roll();
    }

    exit:
    return;
  }

  /**
   * Roll the log file written on the SD card.
   *
   * @pre Close all file descriptors on the log file before calling this function.
   */
  static void cnsslog_sd_log_file_roll(void)
  {
    File        file;
    Dir         dir;
    FileInfo    info;
    const char *pc;
    char        buffer[100];
    uint32_t    v;
    uint32_t    idMin      = UINT32_MAX;
    uint32_t    idMax      = 0;
    uint32_t    total_size = 0;

    // Go through the log files
    sdcard_ffind_first(&dir, &info, SYSLOG_DIRECTORY_NAME, SYSLOG_FILE_NAME ".*");
    while(info.fname[0])
    {
      // Count the file size to the total size
      if(strlcpy(buffer, SYSLOG_DIRECTORY_NAME "/", sizeof(buffer)) < sizeof(buffer) &&
	  strlcat(buffer, info.fname,                sizeof(buffer)) < sizeof(buffer) &&
	  sdcard_fopen(&file, buffer, FILE_OPEN | FILE_READ))
      {
	total_size += sdcard_fsize(&file);
	sdcard_fclose(             &file);
      }

      // Get the maximum and minimum identifiers
      if((pc = strrchr(info.fname, '.')) && (v  = str_nosign_to_uint_with_default(++pc, 0)))
      {
	if(v < idMin) { idMin = v; }
	if(v > idMax) { idMax = v; }
      }

      sdcard_ffind_next(&dir, &info);
    }

    // If total file size is above limit then remove file with the lowest id.
    if(total_size >= SYS_LOG_FILES_TOTAL_SIZE_MAX && idMin != UINT32_MAX)
    {
      // Build the name for the file to remove (the one with the smallest id).
      if(snprintf(buffer, sizeof(buffer),
		  SYSLOG_DIRECTORY_NAME "/" SYSLOG_FILE_NAME ".%06u", (unsigned int)idMin)
	  < (int32_t)sizeof(buffer))
      {
	// Remove the file
	sdcard_remove(buffer);  // Do not care if it fails
      }
    }

    // Build the name for the next rolled syslog file
    if(snprintf(buffer, sizeof(buffer),
		SYSLOG_DIRECTORY_NAME "/" SYSLOG_FILE_NAME ".%06u", (unsigned int)(idMax + 1))
	>= (int32_t)sizeof(buffer)) { goto exit; }

    // Rename the current syslog file as the next rolled syslog file.
    sdcard_rename(SYSLOG_DIRECTORY_NAME "/" SYSLOG_FILE_NAME, buffer);

    exit:
    return;
  }



  /**
   * Enable, or not, the serial logging.
   *
   * @param[in] ps_usart_name the name of the usart to use for logging.
   *                          If NULL or empty then do not use serial logging.
   *                          If we cannot find an USART with the given name so serial logging is not enabled.
   */
  void cnsslog_enable_serial_logging(const char *ps_usart_name)
  {
    _cnsslog_backend_serial.pv_usart = usart_get_by_name(ps_usart_name);

    if(_cnsslog_backend_serial.pv_usart)
    {
      // Open serial interface
      if(usart_open(_cnsslog_backend_serial.pv_usart,
		    "logger",
		    DEBUG_SERIAL_BAUDRATE,
		    USART_PARAM_WORD_LEN_8BITS | USART_PARAM_STOP_BITS_1 |
		    USART_PARAM_PARITY_NONE    | USART_PARAM_MODE_TX))
      {
	// Set up logger back-end object
	_cnsslog_backend_serial.base.ps_name      = usart_name(_cnsslog_backend_serial.pv_usart);
	_cnsslog_backend_serial.base.pf_write_log = cnsslog_backend_serial_write;

	// Register logger back-end object
	logger_register_backend((LoggerBackend *)&_cnsslog_backend_serial);
      }
    }
  }

  /**
   * Write log data to a serial log back-end.
   *
   * @param[in] pv_backend the serial back-end to use. Actual type MUST be CNSSSerialLoggerBackend.
   * @param[in] pu8_data   the data to write. MUST be NOT NULL.
   * @param[in] size       the number of data bytes to write.
   */
  static void cnsslog_backend_serial_write(LoggerBackend *pv_backend,
					   const uint8_t *pu8_data,
					   uint16_t       size)
  {
    CNSSLoggerBackendSerial *pv_serial_backend = (CNSSLoggerBackendSerial *)pv_backend;

    usart_write(pv_serial_backend->pv_usart, pu8_data, size, 200);
  }

#ifdef __cplusplus
}
#endif
