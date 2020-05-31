/*
 * Header for SD card usage.
 *
 *  Created on: 18 mai 2018
 *      Author: jfuchet
 */
#ifndef MODULES_SD_SDCARD_H_
#define MODULES_SD_SDCARD_H_

#include "defs.h"
#include "config.h"
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * Defined the file open action flags.
   */
  typedef enum FileActionFlag
  {
    FILE_OPEN           = 0x00,  ///< Normal open, fails if the file does not exist.
    FILE_OPEN_OR_CREATE = 0x01,  ///< Always open file, create it if it does not exist.
    FILE_TRUNCATE       = 0x02,  ///< Create a new file, if the file exists then it is truncated and overwritten.
    FILE_APPEND         = 0x04,  ///< Always open file, create it if it does not exist, and set read/write position to the end of the file.
    FILE_READ           = 0x10,
    FILE_WRITE          = 0x20,
    FILE_READ_WRITE     = FILE_READ | FILE_WRITE
  }
  FileActionFlag;
  typedef uint8_t FileOpenMode;

  typedef FIL     File;
  typedef DIR     Dir;
  typedef FILINFO FileInfo;


  extern bool sdcard_init();
  extern void sdcard_deinit();

  extern void sdcard_sleep();
  extern bool sdcard_wakeup();

  extern bool sdcard_exists(              const char *ps_filename);
  extern bool sdcard_mkdir(               const char *ps_dirname);
  extern bool sdcard_check_and_mkdir(     const char *ps_dirname);
  extern bool sdcard_remove(              const char *ps_filename);
  extern bool sdcard_remove_using_pattern(const char *ps_path, const char *ps_pattern);
  extern bool sdcard_rename(              const char *ps_old,      const char *ps_new);
  extern bool sdcard_fcreateEmpty(        const char *ps_filename, bool  force);
  extern bool sdcard_fexistOrCreateEmpty( const char *ps_filename, bool  force);

  extern bool sdcard_fopen( File *pv_file, const char *ps_filename, FileOpenMode mode);
  extern void sdcard_fclose(File *pv_file);

  extern bool     sdcard_fread(        File          *pv_file,
				       uint8_t       *pu8_dest,
				       uint32_t       nb_to_read);
  extern uint32_t sdcard_fread_at_most(File          *pv_file,
				       uint8_t       *pu8_dest,
				       uint32_t       nb_to_read,
				       bool          *pb_ok);
  extern bool     sdcard_read_line(    File          *pv_file,
				       char          *ps_buffer,
				       uint32_t       size,
				       bool           strip_eol,
				       bool          *pb_ok);
  extern bool     sdcard_fwrite(       File          *pv_file,
				       const uint8_t *pu8_data,
				       uint32_t       size);
  extern bool     sdcard_fwrite_string(File *pv_file, const char *ps);
  extern bool     sdcard_fwrite_byte(  File *pv_file, uint8_t     b );
  extern bool     sdcard_fsync(        File *pv_file);
  extern uint32_t sdcard_fsize(        File *pv_file);
  extern bool     sdcard_ftruncateTo(  File *pv_file, uint32_t size, bool expand);
  extern uint32_t sdcard_fpos(         File *pv_file);
  extern bool     sdcard_fseek_abs(    File *pv_file, uint32_t pos);
  extern bool     sdcard_fseek_rel(    File *pv_file, int32_t  step);
#define sdcard_rewind(pv_file)  sdcard_fseek_abs(pv_file, 0)

  extern const char *sdcard_ffind_first(Dir        *pv_dir,
				        FileInfo   *pv_info,
				        const char *ps_dir_path,
				        const char *ps_pattern);
  extern const char *sdcard_ffind_next( Dir *pv_dir, FileInfo *pv_info);
  extern bool        sdcard_ffilter(    const char *ps_dir_path,
				        const char *ps_pattern,
				        void      (*pf_filter_func)(const char *ps_filename, void *pv_ctx),
				        void       *pv_ctx);


#ifdef __cplusplus
}
#endif
#endif /* MODULES_SD_SDCARD_H_ */
