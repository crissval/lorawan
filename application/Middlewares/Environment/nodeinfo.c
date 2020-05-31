/*
 * Provide informations about the node
 *
 *  @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */
#include <string.h>
#include <stdio.h>
#include "nodeinfo.h"
#include "sdcard.h"
#include "utils.h"
#include "version.h"


#ifdef __cplusplus
extern "C" {
#endif

#define NODEINFO_DIR                     "proc"
#define NODEINFO_MAIN_BOARD_FILENAME     "board_main"
#define NODEINFO_MAIN_BOARD_FILE         NODEINFO_DIR "/" NODEINFO_MAIN_BOARD_FILENAME
#define NODEINFO_SENSORS_BOARD_FILENAME  "board_sensors"
#define NODEINFO_SENSORS_BOARD_FILE      NODEINFO_DIR "/" NODEINFO_SENSORS_BOARD_FILENAME
#define NODEINFO_APP_VERSION_FILENAME    "app_version"
#define NODEINFO_APP_VERSION_FILE        NODEINFO_DIR "/" NODEINFO_APP_VERSION_FILENAME

#define NODEINFO_KEY_VALUE_SEP_CHAR  ':'

#define NODEINFO_RTC_CAP_12PF_FILE  NODEINFO_DIR "/rtc_cap_12pf"
#define NODEINFO_RTC_CAP_20PF_FILE  NODEINFO_DIR "/rtc_cap_20pf"

#define NODEINFO_BAT_R26R27_FILE    NODEINFO_DIR "/has_r26r27_battdiv_2"


  static NodeInfo _nodeinfo_infos;
  static bool     _nodeinfo_has_been_initialised = false;
  static bool     _nodeinfo_infos_have_been_read = false;


  static bool nodeinfo_read();
  static bool nodeinfo_read_file(const char *ps_filename,
				 bool      (*pf_set_value)(const char *ps_key, const char *ps_value));

  static bool nodeinfo_set_string_value(char *ps_dest, const char *ps_value, uint32_t size);
#define nodeinfo_main_board_set_string_value(name, ps_value) \
  nodeinfo_set_string_value(_nodeinfo_infos.main_board.name, \
			    (ps_value),                      \
			    sizeof(_nodeinfo_infos.main_board.name))
#define nodeinfo_sensors_board_set_string_value(name, ps_value) \
  nodeinfo_set_string_value(_nodeinfo_infos.sensors_board.name, \
			    (ps_value),                         \
			    sizeof(_nodeinfo_infos.sensors_board.name))

  static bool nodeinfo_set_main_board_value(   const char *ps_key, const char *ps_value);
  static bool nodeinfo_set_sensors_board_value(const char *ps_key, const char *ps_value);
  static bool nodeinfo_check_application_infos();

  static void nodeinfo_get_rtc_cap(void);
  static void nodeinfo_get_batt_r26r27(void);


  /**
   * Initialises the node's information system.
   */
  void nodeinfo_init()
  {
    if(!_nodeinfo_has_been_initialised &&
	(sdcard_exists(NODEINFO_DIR) || sdcard_mkdir(NODEINFO_DIR)))
    {
      nodeinfo_check_application_infos();
      _nodeinfo_has_been_initialised = true;
    }
  }


  /**
   * Get the node's informations.
   *
   * Never return a NULL value; if the informations cannot be read then default values are returned.
   *
   * @returns the node informations.
   */
  const NodeInfo *nodeinfo_infos()
  {
    if(!_nodeinfo_infos_have_been_read) { _nodeinfo_infos_have_been_read = nodeinfo_read(); }

    return (const NodeInfo *)&_nodeinfo_infos;
  }

  /**
   * Read the node's informations.
   *
   * In case of error it will read as many informations as possible.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool nodeinfo_read()
  {
    bool res = true;

    res &= nodeinfo_read_file(NODEINFO_MAIN_BOARD_FILE,    nodeinfo_set_main_board_value);
    res &= nodeinfo_read_file(NODEINFO_SENSORS_BOARD_FILE, nodeinfo_set_sensors_board_value);
    nodeinfo_get_rtc_cap();
    nodeinfo_get_batt_r26r27();

    return res;
  }

  /**
   * Read informations from a file and set the values in the information object.
   *
   * @param[in] ps_filename  the name of the file to read to get the informations.
   * @param[in] pf_set_value the function used to set the values in the information objects.
   *                         MUST be NOT NULL.
   *                         the function takes in the key and the value that are NOT NULL
   *                         and NOT EMPTY. And it returns true if everything went well and false
   *                         otherwise.
   *
   * @return true  on success
   * @return false otherwise.
   */
  static bool nodeinfo_read_file(const char *ps_filename,
				 bool      (*pf_set_value)(const char *ps_key, const char *ps_value))
  {
    File file;
    char line[64], *pc, *ps_key, *ps_value;
    bool ok;
    bool res            = true;
    bool last_line_read = false;

    if(sdcard_fopen(&file, ps_filename, FILE_OPEN | FILE_READ))
    {
      while(!last_line_read)
      {
	// Read a line from the file
	if((last_line_read = !sdcard_read_line(&file, line, sizeof(line), true, &ok)))
	{
	  // If error then do not process line; break from the loop
	  res &= ok;
	  if(!ok) { break; }
	}

	// Get the key and the value and trim them.
	if(!(pc  = strchr(line, NODEINFO_KEY_VALUE_SEP_CHAR)) || pc == line) continue;
	ps_value = pc + 1;
	pc--;
	if(!(ps_key = str_trim(line, &pc)) || !(ps_value = str_trim(ps_value, NULL))) continue;
	pc[1] = '\0';  // Make the trimmed key a string

	// Write the value to the info object
	res &= (*pf_set_value)(ps_key, ps_value);
      };
      sdcard_fclose(&file);
    }
    else { res = false; }

    return res;
  }

  /**
   * Set a value in the main board part of the information object.
   *
   * @param[in] ps_key   the key. MUST be NOT NULL.
   * @param[in] ps_value the value. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool nodeinfo_set_main_board_value(const char *ps_key, const char *ps_value)
  {
    bool res = true;

    if(strcmp(ps_key, "SN") == 0)
    { res &= nodeinfo_main_board_set_string_value(serial_number, ps_value); }

    return res;
  }

  /**
   * Set a value in the sensors board part of the information object.
   *
   * @param[in] ps_key   the key. MUST be NOT NULL.
   * @param[in] ps_value the value. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool nodeinfo_set_sensors_board_value(const char *ps_key, const char *ps_value)
  {
    bool res = true;

    if(strcmp(ps_key, "SN") == 0)
    { res &= nodeinfo_sensors_board_set_string_value(serial_number, ps_value); }

    return res;
  }




  /**
   * Write a string value to a a buffer.
   *
   * @param[out] ps_dest  where the value is written to. Set to empty string in case of error.
   *                      MUST be NOT NULL.
   * @param[in]  ps_value the value to write. Can be NULL, then write an empty string.
   * @param[in]  size     the size of the ps_dest buffer. MUST be > 0.
   *
   * @return true  on success.
   * @return false if the ps_dest buffer was too small to receive the whole value.
   */
  static bool nodeinfo_set_string_value(char *ps_dest, const char *ps_value, uint32_t size)
  {
    bool res;

    if(!ps_value || !*ps_value)                            { *ps_dest = '\0'; return true; }
    if(!(res = (strlcpy(ps_dest, ps_value, size) < size))) { *ps_dest = '\0';              }

    return res;
  }


  /**
   * Check that the application's software informations reported by
   * the 'proc' entry are correct. And change if they are not.
   *
   * @return true  on success.
   * @return false in case of error.
   */
  static bool nodeinfo_check_application_infos()
  {
    File     file;
    uint8_t  buffer[64];
    uint32_t size;
    bool     res = true;

    // Check software version
    if(sdcard_fopen(&file, NODEINFO_APP_VERSION_FILE, FILE_OPEN | FILE_READ))
    {
      size = sdcard_fread_at_most(&file, buffer, sizeof(buffer), NULL);
      sdcard_fclose(&file);
      if(!size || size != strlen(VERSION_STR) || memcmp(buffer, VERSION_STR, size) != 0)
      {    goto update_application_version; }
    }
    else { goto update_application_version; }
    goto check_2;

    update_application_version:
    if(sdcard_fopen(&file, NODEINFO_APP_VERSION_FILE, FILE_TRUNCATE | FILE_WRITE))
    {
      res &= sdcard_fwrite_string(&file, VERSION_STR);
      sdcard_fclose(&file);
    }
    else { res = false; }

    check_2:
    // No more check to perform

    return res;
  }


  /**
   * Get the RTC capacitors' value from 'proc' directory.
   *
   * @post rtc_cap value is updated in the node information object.
   */
  static void nodeinfo_get_rtc_cap(void)
  {
    // Set default value
    _nodeinfo_infos.main_board.rtc_cap = NODEINFO_RTC_CAP_20PF;

    // Set value
    if(     sdcard_exists(NODEINFO_RTC_CAP_20PF_FILE))
    {
      _nodeinfo_infos.main_board.rtc_cap = NODEINFO_RTC_CAP_20PF;
    }
    else if(sdcard_exists(NODEINFO_RTC_CAP_12PF_FILE))
    {
      _nodeinfo_infos.main_board.rtc_cap = NODEINFO_RTC_CAP_12PF;
    }
  }


  /**
   * See if the board has the R26-R27 battery voltage divisor.
   *
   * @post has_batt_r26r27 value is updated in the node information object.
   */
  static void nodeinfo_get_batt_r26r27(void)
  {
    uint8_t buffer[16];
    File    f;
    uint8_t size;
    bool    ok;

    _nodeinfo_infos.main_board.batt_r26r27_divisor = 0;

    if((_nodeinfo_infos.main_board.has_batt_r26r27 = sdcard_exists(NODEINFO_BAT_R26R27_FILE)))
    {
      if(sdcard_fopen(&f, NODEINFO_BAT_R26R27_FILE, FILE_OPEN | FILE_READ))
      {
	size = sdcard_fread_at_most(&f, buffer, sizeof(buffer), &ok);
	if(size && ok)
	{
	  _nodeinfo_infos.main_board.batt_r26r27_divisor =
	      strn_string_to_float_trim_with_default((const char *)buffer, size, 0);
	}
	sdcard_fclose(&f);
      }
    }
  }

  /**
   * Write the R26-R27 battery voltage divisor ratio.
   *
   * @param[in] div the divisor value to write.
   *
   * @return true  on success.
   * @return false if the board does not have a R26-R27 battery voltage divisor.
   * @return false in case of write error.
   */
  bool nodeinfo_main_write_batt_r26r27_divisor(float div)
  {
    char buffer[16];
    File f;
    bool ok = false;

    _nodeinfo_infos.main_board.batt_r26r27_divisor = div;

    // Check that the board has the voltage divisor.
    if(!_nodeinfo_infos.main_board.has_batt_r26r27) { goto exit; }

    // Write
    if(sdcard_fopen(&f, NODEINFO_BAT_R26R27_FILE, FILE_TRUNCATE | FILE_WRITE))
    {
      if(_nodeinfo_infos.main_board.batt_r26r27_divisor)
      {
	snprintf(buffer, sizeof(buffer), "%1.3f", _nodeinfo_infos.main_board.batt_r26r27_divisor);
	ok = sdcard_fwrite_string(&f, buffer);
      }
      else { ok = true; }
      sdcard_fclose(&f);
    }

    exit:
    return ok;
  }




#ifdef __cplusplus
}
#endif
