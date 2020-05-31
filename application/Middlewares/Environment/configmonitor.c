/*
 * Detects changes in the configuration file.
 *
 * @author FUCHET Jérôme (Jerome.FUCHET@uca.fr)
 * @date   2018
 */

#include <string.h>
#include <stdio.h>
#include "configmonitor.h"
#include "sdcard.h"
#include "murmur3.h"


#define REFERENCE_FILE                   PRIVATE_DATA_DIRECTORY_NAME "/config.json.ref"
#define MANUAL_TIME_REFERENCE_FILE       PRIVATE_DATA_DIRECTORY_NAME "/config.json.manual_time.ref"
#define TIME_SYNC_METHOD_REFERENCE_FILE  PRIVATE_DATA_DIRECTORY_NAME "/config.json.time_sync_method.ref"



#ifdef __cplusplus
extern "C" {
#endif


  static bool     _config_monitor_hash_mm3_32_is_set;  ///< Has the configuration's murmur3 32 bits hash already been computed?
  static uint32_t _config_monitor_hash_mm3_32;         ///< The configuration's murmur3 32 bits hash.



  /**
   * Initialises the configuration file changes detector.
   */
  void config_monitor_init(void)
  {
    // Do nothing
  }

  /**
   * Indicate if the configuration file has changed or not.
   *
   * @param[in] pu8_cfg  the configuration file's data. MUST be NOT NULL.
   * @param[in] size     the configuration file's size (the amount of data).
   * @param[in] save     make pu8_cfg the new reference configuration (true), or not (false).
   *                     If set to true then the next comparison will be done with the data in pu8_cfg.
   *                     If set to false then the next comparison will be done with the current data.
   *
   * @return true  if the configuration file has changed.
   * @return false otherwise.
   */
  bool config_monitor_config_has_changed(const uint8_t *pu8_cfg, uint32_t size, bool save)
  {
    File           file;
    bool           fileOpened, ok;
    uint8_t        buffer[200];
    const uint8_t *pu8;
    uint32_t       nb_read;
    uint32_t       current_size = 0;
    bool           res          = false;

    // Open the reference file and get it's size
    if((fileOpened   = sdcard_fopen( &file, REFERENCE_FILE, FILE_OPEN | FILE_READ)))
    {
      current_size   = sdcard_fsize(&file);
    }
    if(current_size != size) { res = true; goto exit; }

    // Compare the data byte to byte
    for(pu8 = pu8_cfg, nb_read = 1; nb_read; pu8 += nb_read)
    {
      // Read a chunk of the reference file
      nb_read = sdcard_fread_at_most(&file, buffer, sizeof(buffer), &ok);
      if(!ok || memcmp(pu8, buffer, nb_read) != 0) { res = true; goto exit; }
    }

    exit:
    if(fileOpened)                                         { sdcard_fclose(&file);          }
    if(save && !config_monitor_save_config(pu8_cfg, size)) { sdcard_remove(REFERENCE_FILE); }
    return res;
  }


  /**
   * Set the configuration reference's data.
   *
   * @param[in] pu8_cfg the data to save. MUST be NOT NULL.
   * @param[in] size    the amount of data to save.
   *
   * @return true  on success.
   * @return false if size is 0.
   * @return false if failed to write to disk.
   */
  bool config_monitor_save_config(const uint8_t *pu8_cfg, uint32_t size)

  {
    File file;
    bool res;

    if(!pu8_cfg || !size || !sdcard_fopen(&file, REFERENCE_FILE, FILE_TRUNCATE | FILE_WRITE))
    { res = false; goto exit; }

    res = sdcard_fwrite(&file, pu8_cfg, size);
    sdcard_fclose(      &file);

    exit:
    return res;
  }


  /**
   * Indicate if the manual time set in the configuration file has changed or not.
   *
   * @param[in] pv_dt the time set in the current configuration.
   * @param[in] save  make pv_dt the reference time configuration (true), or not (false).
   *
   * @return true  if the manual time configuration has changed.
   * @return true  if fails to open or read reference time.
   * @return false otherwise.
   * @return false if pv_dt is NULL.
   */
  bool config_monitor_manual_time_has_changed(const Datetime *pv_dt, bool save)
  {
    File    file;
    uint8_t buffer_ref[19], buffer[20];  // +1 in buffer for the ending '\0' character.
    bool    ok;
    bool    res = true;

    if(!pv_dt) { res = false; goto exit; }

    // Build the string for the current time.
    sprintf((char *)buffer,
	    "%04d-%02d-%02dT%02d:%02d:%02d",
	    pv_dt->year, pv_dt->month, pv_dt->day, pv_dt->hours, pv_dt->minutes, pv_dt->seconds);

    // Open reference file
    if(!sdcard_fopen(&file, MANUAL_TIME_REFERENCE_FILE, FILE_OPEN | FILE_READ)) { goto save_and_exit; }

    // Read the file's contents
    ok = sdcard_fread(&file, buffer_ref, sizeof(buffer_ref));
    sdcard_fclose(&file);
    if(!ok) { goto save_and_exit; }

    // Compare the strings
    res = (strncmp((const char *)buffer_ref, (const char *)buffer, sizeof(buffer_ref)) != 0);

    save_and_exit:
    // Only save if a change has been detected.
    if(res && save && sdcard_fopen(&file, MANUAL_TIME_REFERENCE_FILE, FILE_TRUNCATE | FILE_WRITE))
    {
      sdcard_fwrite(&file, buffer, sizeof(buffer_ref));  // Size of buffer_ref is on purpose.
      sdcard_fclose(&file);
    }

    exit:
    return res;
  }


  /**
   * Indicate if the method used to synchronise time has changed in the configuration file or not.
   *
   * @param[in] ps_method_name the name of the synchronisation method to use. Can be NULL or empty.
   *                           the name comparison is not case sensitive.
   * @param[in] default_value  the default value to return.
   * @param[in] save           make ps_method_name the reference (true), not not (false).
   *
   * @return true          if the synchronisation method has changed.
   * @return default_value if fails to find, open or read the reference.
   * @return default value if ps_method_name is NULL or empty.
   * @return false         if the method is the same.
   */
  bool config_monitor_time_sync_method_has_changed(const char *ps_method_name,
						   bool        default_value,
						   bool        save)
  {
    File     file;
    char     buffer[32];
    uint32_t size;
    bool     ok;
    bool     res = default_value;

    // Check name
    if(!ps_method_name || !*ps_method_name) { goto exit; }

    // Open reference
    if(!sdcard_fopen(&file, TIME_SYNC_METHOD_REFERENCE_FILE, FILE_OPEN | FILE_READ)) { goto save_and_exit; }

    // Read the file's contents.
    size = sdcard_fread_at_most(&file, (uint8_t *)buffer, sizeof(buffer), &ok);
    sdcard_fclose(&file);
    if(!ok) { goto save_and_exit; }

    // Compare names
    res = (strncasecmp(ps_method_name, buffer, size) != 0);

    save_and_exit:
    // Only save if a change has been detected.
    if(res && save && sdcard_fopen(&file, TIME_SYNC_METHOD_REFERENCE_FILE, FILE_TRUNCATE | FILE_WRITE))
    {
      sdcard_fwrite_string(&file, ps_method_name);
      sdcard_fclose(       &file);
    }

    exit:
    return res;
  }


  /**
   * Computes the configuration's murmur3 32 bits hash.
   *
   * @param[out] pu32_hash where the hash value is written to. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  extern bool config_monitor_hash_mm3_32(uint32_t *pu32_hash)
  {
    MM332Stream stream;
    uint32_t    size;
    uint8_t     buffer[256];
    File        file;
    bool        res = true;

    // If hash already has been computed then return previous value
    if(_config_monitor_hash_mm3_32_is_set)
    {
     *pu32_hash = _config_monitor_hash_mm3_32;
      goto exit;
    }

    // Open configuration file
    if(!sdcard_fopen(&file, CONFIG_FILE_NAME, FILE_READ)) { res = false; goto exit; }

    // Read the configuration fire chunk by chunk and compute hash
    mm3_32_stream_init_cnss(&stream);
    while((size = sdcard_fread_at_most(&file, buffer, sizeof(buffer), &res)) && res)
    {
      mm3_32_stream_digest(&stream, buffer, size);
    }
    *pu32_hash = _config_monitor_hash_mm3_32 = mm3_32_stream_finish(&stream);
    _config_monitor_hash_mm3_32_is_set       = res;
    sdcard_fclose(&file);

    exit:
    return res;
  }


#ifdef __cplusplus
}
#endif


