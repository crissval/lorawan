/**
 * @file  logger.c
 * @brief Implement a logging system.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#include <stdarg.h>
#include <string.h>
#include "logger.h"
#include "board.h"
#include "utils.h"
#include "rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFAULT_LOG_LEVEL
#define DEFAULT_LOG_LEVEL  LOG_INFO
#endif

#ifndef LOGGER_WORKING_BUFFER_SIZE
#define LOGGER_WORKING_BUFFER_SIZE  1500
#elif LOGGER_WORKING_BUFFER_SIZE < 100
#error "LOGGER_WORKING_BUFFER_SIZE must be at least 100."
#endif

#ifndef LOGGER_FIELD_SEP_CHAR
#define LOGGER_FIELD_SEP_CHAR  '|'
#endif


  static const char *logger_log_level_to_str[] =
  {
      "     ",
      "TRACE",
      "DEBUG",
      "INFO ",
      "WARN ",
      "ERROR",
      "FATAL",
      "     "
  };


  static char                  logger_working_buffer[LOGGER_WORKING_BUFFER_SIZE];
  static LoggingConfiguration *pv_logger_config           = NULL;
  static Logger               *pv_loggers_list            = NULL;
  static uint8_t               logger_default_level       = DEFAULT_LOG_LEVEL;
  //static uint32_t              logger_log_counter         = 1;  // Start at 1 so that we can detect counter overflows in the logs.
  static LoggerBackend        *pv_logger_backends         = NULL;



  /**
   * Initialise the logging system.
   *
   * @note the init() function of the configuration is called.
   *
   * MUST be called before any logger is initialised.
   * If not then all the loggers initialised previously to calling this function
   * will use the default configuration, set up at compile time.
   *
   * @param[in] p_config the configuration to use.
   *                     Can be NULL, in which case the default configuration
   *                     set at compile time will be used.
   */
  void logger_init(LoggingConfiguration *p_config)
  {
    if(p_config->pf_init) { p_config->pf_init(); }
    pv_logger_config = p_config;

    if(p_config->default_log_level != LOG_DEFAULT)
    {
      logger_default_level = p_config->default_log_level;
    }
  }


  /**
   * Initialise a new logger and add it to the logger list.
   *
   * @pre the logger's name MUST already have been set. The name CANNOT be NULL or EMPTY.
   *      There MUST be NO OTHER logger WITH the SAME NAME.
   *
   * @param[in] pv_logger the logger. MUST be NOT NULL.
   */
  void logger_init_logger(Logger *pv_logger)
  {
    uint8_t level = LOG_DEFAULT;

    // Get log level from configuration
    if(pv_logger_config && pv_logger_config->pf_get_log_level)
    {
      level = pv_logger_config->pf_get_log_level(pv_logger->ps_name);
    }

    // Initialise the logger.
    pv_logger->name_len = strlen(pv_logger->ps_name);
    pv_logger->level    = level;

    // Add it to the loggers list.
    pv_logger->p_next   = pv_loggers_list;
    pv_loggers_list     = pv_logger;
  }

  /**
   * Register a logger back-end.
   *
   * @note This function do not check if the back-end already is registered or not.
   * @note the back-ends are called in the order they have been registered.
   *
   * @param[in] pv_backend the back-end to register. MUST be NOT NULL.
   */
  void logger_register_backend(LoggerBackend *pv_backend)
  {
    LoggerBackend *p;

    pv_backend->pv_next = NULL;

    if(pv_logger_backends)
    {
      // Look for the last backend of the list
      for(p = pv_logger_backends; p->pv_next; p = p->pv_next) ;

      // Append to the list
      p->pv_next = pv_backend;
    }
    else { pv_logger_backends = pv_backend; }
  }


  /**
   * Set the default debug level.
   *
   * @param[in] level the default debug level to use.
   */
  void logger_set_default_level(LogLevel level)
  {
    logger_default_level = level;
  }

  /**
   * Set the default log level using a level name.
   * The name comparison is not case sensitive.
   *
   * @param[in] ps_level the level's name.
   *
   * @return true  if the default level has been set to the new value.
   * @return false if the level name is unknown.
   * @return false if ps_level is NULL.
   */
  bool logger_set_default_level_using_string(const char *ps_level)
  {
    LogLevel level = DEFAULT_LOG_LEVEL;
    bool     res   = false;

    if(!ps_level || !*ps_level) { goto exit; }

    if     (strcasecmp(ps_level, "TRACE")   == 0) { level = LOG_TRACE; }
    else if(strcasecmp(ps_level, "DEBUG")   == 0) { level = LOG_DEBUG; }
    else if(strcasecmp(ps_level, "INFO")    == 0) { level = LOG_INFO;  }
    else if(strcasecmp(ps_level, "WARN")    == 0) { level = LOG_WARN;  }
    else if(strcasecmp(ps_level, "WARNING") == 0) { level = LOG_WARN;  }
    else if(strcasecmp(ps_level, "ERROR")   == 0) { level = LOG_ERROR; }
    else if(strcasecmp(ps_level, "FATAL")   == 0) { level = LOG_FATAL; }
    else if(strcasecmp(ps_level, "OFF")     == 0) { level = LOG_OFF;   }

    if(level != DEFAULT_LOG_LEVEL)
    {
      logger_set_default_level(level);
      res = true;
    }

    exit:
    return res;
  }

  /**
   * Set the default log level using a level name and it's length.
   * The name comparison is not case sensitive.
   *
   * With this function you can use non null terminated strings as name.
   *
   * @param[in] ps_level the level's name.
   * @param[in] len      the name's length.
   *
   * @return true  if the default level has been set to the new value.
   * @return false if the level name is unknown.
   * @return false if ps_level is NULL.
   */
  bool logger_set_default_level_using_string_and_len(const char *ps_level, uint32_t len)
  {
    LogLevel level = DEFAULT_LOG_LEVEL;
    bool     res   = false;

    if(!ps_level || !*ps_level) { goto exit; }

    if     (strncasecmp(ps_level, "TRACE",   len) == 0) { level = LOG_TRACE; }
    else if(strncasecmp(ps_level, "DEBUG",   len) == 0) { level = LOG_DEBUG; }
    else if(strncasecmp(ps_level, "INFO",    len) == 0) { level = LOG_INFO;  }
    else if(strncasecmp(ps_level, "WARN",    len) == 0) { level = LOG_WARN;  }
    else if(strncasecmp(ps_level, "WARNING", len) == 0) { level = LOG_WARN;  }
    else if(strncasecmp(ps_level, "ERROR",   len) == 0) { level = LOG_ERROR; }
    else if(strncasecmp(ps_level, "FATAL",   len) == 0) { level = LOG_FATAL; }
    else if(strncasecmp(ps_level, "OFF",     len) == 0) { level = LOG_OFF;   }

    if(level != DEFAULT_LOG_LEVEL)
    {
      logger_set_default_level(level);
      res = true;
    }

    exit:
    return res;
  }


  /**
   * Log a message.
   *
   * @param[in] pv_logger the logger. MUST be NOT NULL and MUST have been INITIALISED.
   * @param[in] level     the level of this log entry.
   * @param[in] ps_file   the file name. MUST be NOT NULL and NOT be EMPTY.
   * @param[in] line      the line number.
   * @param[in] ps_func   the function's name. MUST be NOT NULL and NOT be EMPTY.
   * @param[in] msg       the message. Followed by message arguments. All formated in printf format.
   */
  void logger_log(Logger     *pv_logger,
		  LogLevel    level,
		  const char *ps_file,
		  uint32_t    line,
		  const char *ps_func,
		  const char *msg,
		  ...)
  {
    Datetime       ts;
    uint16_t       len;
    char          *ps_value;
    va_list        ap;
    LoggerBackend *pv_backend;
    LogLevel       logger_level;

    if(!pv_logger_backends)                     { goto exit; }
    if(!logger_has_been_initialised(pv_logger)) { logger_init_logger(pv_logger); }

    // Check against log level
    logger_level = (pv_logger->level == LOG_DEFAULT) ? logger_default_level : pv_logger->level;
    if(logger_level > level) { goto exit; }

    // Build log string
    // Write timestamp
    rtc_get_date(&ts);
    sprintf(logger_working_buffer, "%04d-%02d-%02d %02d:%02d:%02d",
	    ts.year, ts.month, ts.day, ts.hours, ts.minutes, ts.seconds);
    len = 19;
    logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;

    // Write log level
    strcpy(&logger_working_buffer[len], logger_log_level_to_str[level]);
    len += 5;
    logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;

    // Write logger name
    strcpy(&logger_working_buffer[len], pv_logger->ps_name);
    len += pv_logger->name_len;
    logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;

    // Write the line number
    len += strn_uint_to_string(&logger_working_buffer[len], line, 10, true) - &logger_working_buffer[len];
    logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;


    // Write file infos if we are asked to
    if(pv_logger_config && pv_logger_config->print_file_infos)
    {
      // Extract and print file name
      ps_value = strrchr(ps_file, '/');
      if(!ps_value) { ps_value = strrchr(ps_file, '\\'); }  // In case of compiled with Windows
      ps_value = ps_value ? ps_value + 1 : (char *)ps_file;
      strlcpy(&logger_working_buffer[len], ps_value, sizeof(logger_working_buffer) - len);
      len += strlen(&logger_working_buffer[len]);
      logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;

      // Write the line number
      ps_value = strn_uint_to_string(&logger_working_buffer[len], line, sizeof(logger_working_buffer) - len, true);
      len     += ps_value - &logger_working_buffer[len];
      logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;

      // Write the function's name
      strlcpy(&logger_working_buffer[len], ps_func, sizeof(logger_working_buffer) - len);
      len += strlen(&logger_working_buffer[len]);
      logger_working_buffer[len++] = LOGGER_FIELD_SEP_CHAR;
    }

    // Write the message
    va_start(ap, msg);
    vsnprintf(&logger_working_buffer[len], sizeof(logger_working_buffer) - len, msg, ap);
    va_end(ap);

    // Append end of line if we need to
    len += strlen(&logger_working_buffer[len]);
    if(logger_working_buffer[len - 1] != '\n' && len < sizeof(logger_working_buffer) - 3)
    {
      // Append end of line compatible with Windows
      logger_working_buffer[len++] = '\r';
      logger_working_buffer[len++] = '\n';
      logger_working_buffer[len]   = '\0';  // So that we have a valid string
    }

    // Write log line to the back-ends
    for(pv_backend = pv_logger_backends; pv_backend; pv_backend = pv_backend->pv_next)
    {
      pv_backend->pf_write_log(pv_backend, (const uint8_t *)logger_working_buffer, len);
    }

    exit:
    return;
  }


#ifdef __cplusplus
}
#endif
