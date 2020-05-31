/**
 * @file  logger.h
 * @brief Header file defining a logger.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <error.h>
#include "defs.h"
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Defines the log levels
   */
  typedef enum LogLevel
  {
    LOG_DEFAULT = 0,
    LOG_TRACE   = 1,
    LOG_DEBUG   = 2,
    LOG_INFO    = 3,
    LOG_WARN    = 4,
    LOG_ERROR   = 5,
    LOG_FATAL   = 6,
    LOG_OFF     = 7
  }
  LogLevel;

  /**
   * Defines the logger type.
   */
  struct Logger;
  typedef struct Logger Logger;
  struct Logger
  {
    const char *ps_name;   ///< The logger's name.
    uint8_t     name_len;  ///< The logger's name length.
    uint8_t     level;     ///< The log level to use.

    Logger     *p_next;    ///< Next logger in the logger list.
  };


  /**
   * Defined the interface that must implement a logging configuration.
   */
  typedef struct LoggingConfiguration
  {
    /**
     * Initialises the logging configuration.
     * It may be a good idea to protect this function from multiple callings.
     *
     * Can be NULL if there is nothing to initialise.
     */
    void (*pf_init)(void);

    /**
     * The default log level to use.
     */
    LogLevel default_log_level;

    /**
     * Print the file informations (file name, line number and function name) in log lines?
     */
    bool print_file_infos;

    /**
     * Return the log level for a logger using its name.
     *
     * This function pointer can be NULL if we only wish to use the default log level.
     *
     * @param[in] ps_name the logger's name.
     *
     * @return the log level.
     * @return the default log level if ps_name is NULL or empty.
     */
    LogLevel (*pf_get_log_level)(const char *ps_name);
  }
  LoggingConfiguration;


  /**
   * Defines the interface that must provide a logger back-end.
   */
  struct LoggerBackend;
  typedef struct LoggerBackend LoggerBackend;
  struct LoggerBackend
  {
    const char *ps_name;  ///< The back-end's name.

    /**
     * Write a string to the log.
     *
     * @param[in] pv_backend the pointer to this back-end. IS NOT NULL.
     * @param[in] pu8_data   the data to write. Is NOT NULL.
     * @param[in] size       the number of data bytes to write.
     */
    void (*pf_write_log)(LoggerBackend *pv_backend, const uint8_t *pu8_data, uint16_t size);

    LoggerBackend *pv_next;  ///< Next back-end in the back-end list. Can be NULL if is last of the list.
  };


#define LOGGER(name)  _##name##__logger_

#define CREATE_LOGGER(name) \
  static Logger _##name##__logger_ = { #name, 0, LOG_DEFAULT, NULL }

#define logger_has_been_initialised(pv_logger) ((pv_logger)->name_len != 0)

#define log_level(name, lvl, msg, ...)   \
      logger_log(&LOGGER(name), LOG_##lvl, __FILE__, __LINE__, _FUNCTION_NAME_, msg _VARGS_(__VA_ARGS__));

#define log_fatal(name, msg, ...)  \
    log_level(name, FATAL, msg _VARGS_(__VA_ARGS__)); \
    fatal_error_with_msg(  msg _VARGS_(__VA_ARGS__))

#define log_error(name, msg, ...)  log_level(name, ERROR, msg _VARGS_(__VA_ARGS__))

#define log_warn( name, msg, ...)  log_level(name, WARN,  msg _VARGS_(__VA_ARGS__))

#ifdef  LOGGER_DISABLE_LEVEL_INFO
#define log_info(name, msg, ...)  /* Nothing */
#else
#define log_info(name, msg, ...)   log_level(name, INFO,  msg _VARGS_(__VA_ARGS__))
#endif

#ifdef  LOGGER_DISABLE_LEVEL_DEBUG
#define log_debug(name, msg, ...)  /* Nothing */
#else
#define log_debug(name, msg, ...)  log_level(name, DEBUG, msg _VARGS_(__VA_ARGS__))
#endif

#ifdef  LOGGER_DISABLE_LEVEL_TRACE
#define log_trace(name, msg, ...)  /* Nothing */
#else
#define log_trace(name, msg, ...)  log_level(name, TRACE, msg _VARGS_(__VA_ARGS__))
#endif

#define log_nstr(p_str, len)  logger_nstr(p_str, len)


  extern void logger_init(            LoggingConfiguration *p_config);
  extern void logger_init_logger(     Logger               *p_logger);
  extern void logger_register_backend(LoggerBackend        *pv_backend);

  extern void logger_set_default_level(                     LogLevel    level);
  extern bool logger_set_default_level_using_string(        const char *ps_level);
  extern bool logger_set_default_level_using_string_and_len(const char *ps_level, uint32_t len);

  extern void logger_log(Logger     *pv_logger,
			 LogLevel    level,
			 const char *ps_file,
			 uint32_t    line,
			 const char *ps_func,
			 const char *msg,
			 ...);


#ifdef __cplusplus
}
#endif
#endif /* __LOGGER_H__ */
