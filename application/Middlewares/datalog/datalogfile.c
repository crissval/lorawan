/*
 *  Implementation of a fixed size entry datalog file
 *
 *  @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */
#include <string.h>
#include "config.h"
#include "datalogfile.h"
#include "logger.h"
#include "rtc.h"


#ifdef __cplusplus
extern "C" {
#endif

  CREATE_LOGGER(datalogfile);
#undef  _logger
#define _logger datalogfile


#define DATALOG_VERSION  1


#define FIRST_RECORD_POS  sizeof(DataLogFileHeader)


  /**
   * Defines a NULL record header
   */
  static const DataLogFileRecordHeader _NULL_RECORD_HEADER = { 0, 0, 0 };


  static bool datalogfile_create(            DataLogFile *pv_dlf, const char *ps_contents_type);
  static bool datalogfile_read_header(       DataLogFile *pv_dlf, const char *ps_contents_type);
  static bool datalogfile_backup(            DataLogFile *pv_dlf);



  /**
   * Initialise a datalog file object.
   *
   * @param[in] pv_dlf       the datalog file object to initialise. MUST be NOT NULL.
   * @param[in] ps_filename  the name of the datalog file. MUST be NOT NULL and NOT empty.
   * @param[in] records_size the size of each record, in bytes. MUST be > sizeof(DataLogFileRecordHeader).
   * @param[in] nb_records   the number of records to store. MUST be > 0.
   *
   * @return true  On success.
   * @return false Otherwise.
   */
  bool datalogfile_init(DataLogFile *pv_dlf,
			const char  *ps_filename,
			uint32_t     records_size,
			uint32_t     nb_records)
  {
    bool res = true;

    pv_dlf->is_opened            = false;
    pv_dlf->created_by_last_open = false;
    pv_dlf->file_size            = 0;
    pv_dlf->header.records_size  = records_size;
    pv_dlf->header.nb_records    = nb_records;

    // Copy file name.
    if(strlcpy(pv_dlf->ps_filename,
	       ps_filename,
	       DATALOG_FILENAME_SIZE_MAX) >= DATALOG_FILENAME_SIZE_MAX)
    {
      log_error(_logger,
		"Datalog filename is longer that %u characters.",
		DATALOG_FILENAME_SIZE_MAX - 1);
      res = false;
    }

    return res;
  }


  /**
   * Open a datalog file.
   *
   * @param[in] pv_dlf           the datalog file object.
   * @param[in] ps_contents_type the name of the contents type to expect. MUST be NOT NULL and NOT empty.
   * @param[in] create           create the file if it does not exist?
   *
   * @return true  if the file has been opened.
   * @return false otherwise.
   */
  bool datalogfile_open(DataLogFile *pv_dlf, const char *ps_contents_type, bool create)
  {
    pv_dlf->created_by_last_open = false;

    if(pv_dlf->is_opened) { return true; } // Already opened

    // Check if the file exist
    if(!sdcard_exists(pv_dlf->ps_filename))
    {
      // The file does not exist
      if(!create || !datalogfile_create(pv_dlf, ps_contents_type)) { goto error_exit; }
    }

    // Open file
    if(!sdcard_fopen(&pv_dlf->file, pv_dlf->ps_filename, FILE_READ_WRITE))
    {
      log_error(_logger, "Failed to open datalog file in read and write mode: '%s'.", pv_dlf->ps_filename);
      goto error_exit;
    }

    // Read the header
    if(!datalogfile_read_header(pv_dlf, ps_contents_type))
    {
      // Failed to read the header.
      // Create a backup of the current datalog file and create a new empty one.
      sdcard_fclose(   &pv_dlf->file);
      if(!datalogfile_backup(pv_dlf) || !create || !datalogfile_create(pv_dlf, ps_contents_type)) { goto error_exit; }
      if(!sdcard_fopen(&pv_dlf->file, pv_dlf->ps_filename, FILE_READ))
      {
        log_error(_logger, "Failed to open newly created datalog file in read and write mode: '%s'.", pv_dlf->ps_filename);
        goto error_exit;
      }
      if(!datalogfile_read_header(pv_dlf, ps_contents_type))
      {
	log_error(_logger, "Failed to read header from newly created datalog file: '%s'.", pv_dlf->ps_filename);
	goto error_exit;
      }
    }

    pv_dlf->is_opened = true;
    return true;

    error_exit:
    return false;
  }

  /**
   * Close the datalog file.
   *
   * @param[in] pv_dlf the datalog file object.
   */
  void datalogfile_close(DataLogFile *pv_dlf)
  {
    if(pv_dlf->is_opened)
    {
      sdcard_fclose(&pv_dlf->file);
      pv_dlf->is_opened = false;
    }
  }

  /**
   * Flush the file caches; make sure all data is actually written to disk.
   *
   * @param[in] pv_dlf the datalog file object.
   */
  void datalogfile_sync(DataLogFile *pv_dlf)
  {
    if(pv_dlf->is_opened)
    {
      sdcard_fsync(&pv_dlf->file);
    }
  }


  /**
   * Create a new empty empty datalog file.
   *
   * @note if the file already exists then it will be overwritten.
   *
   * @pre pv_dlf->header.records_size and pv_dlf->header.nb_records MUST have been set.
   *
   * @param[in] pv_dlf           the datalog file object.
   * @param[in] ps_contents_type the name of the contents type. MUST be NOT NULL and NOT empty.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool datalogfile_create(DataLogFile *pv_dlf, const char *ps_contents_type)
  {
    uint32_t size; //, pos;

    // Open/create/truncate
    if(!sdcard_fopen(&pv_dlf->file, pv_dlf->ps_filename, FILE_TRUNCATE | FILE_WRITE))
    {
      log_error(_logger, "Failed to create new datalog file: '%s'.", pv_dlf->ps_filename);
      return false;
    }

    // Allocate space on disk for the file
    size = FIRST_RECORD_POS + pv_dlf->header.nb_records * pv_dlf->header.records_size;
    if(!sdcard_fseek_abs(&pv_dlf->file, size))
    {
      log_error(_logger, "Failed to expand datalog file '%s' size to %d bytes.", pv_dlf->ps_filename, size);
      goto error_exit;
    }

    // Set up the header
    pv_dlf->header.format  = DLOGF_FORMAT_BYTE_ORDER_LITTLE_ENDIAN;
    pv_dlf->header.version = DATALOG_VERSION;
    pv_dlf->header.dynamic.nb_records_with_data = 0;
    pv_dlf->header.dynamic.log_head_seek_pos    = FIRST_RECORD_POS;
    pv_dlf->header.dynamic.log_tail_seek_pos    = FIRST_RECORD_POS;
    if(strlcpy(pv_dlf->header.contents_type, ps_contents_type, sizeof(DataLogFileContentsId))
	>= sizeof(DataLogFileContentsId))
    {
      log_error(_logger, "Cannot create datalog file '%s' with a contents type too long.", pv_dlf->ps_filename);
      goto error_exit;
    }
    if( !sdcard_rewind(&pv_dlf->file) ||
	!sdcard_fwrite(&pv_dlf->file, (const uint8_t *)&pv_dlf->header, sizeof(DataLogFileHeader)))
    {
      log_error(_logger, "Failed to write header to newly created datalog file '%s'.", pv_dlf->ps_filename);
      goto error_exit;
    }

    /*
    // Write default values for all the records.
    // It may help if the header is broken and we need to reconstruct it.
    for(pos = FIRST_RECORD_POS; pos < size; pos += pv_dlf->header.records_size)
    {
      if( !sdcard_fseek_abs(&pv_dlf->file, pos) ||
	  !sdcard_fwrite(   &pv_dlf->file,
			    (const uint8_t *)&_NULL_RECORD_HEADER,
			    sizeof(           _NULL_RECORD_HEADER)))
      {
	log_error(_logger, "Failed to initialise contents of newly created datalog file '%s'.", pv_dlf->ps_filename);
	goto error_exit;
      }
    }
    */

    sdcard_fclose(&pv_dlf->file);
    pv_dlf->created_by_last_open = true;
    log_info(_logger, "New datalog file has been created: '%s'.", pv_dlf->ps_filename);
    return true;

    error_exit:
    sdcard_fclose(&pv_dlf->file);
    return false;
  }

  /**
   * Create a backup of the datalog file. The file is not copied; its is moved (renamed).
   *
   * @pre the file (File object pv_dlf->file) MUST be closed.
   *
   * @param[in] pv_dlf the datalog file object.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool datalogfile_backup(DataLogFile *pv_dlf)
  {
    uint32_t len;
    Datetime dt;
    char     new_name[100];

    // Build the name of the backup file
    rtc_get_date(&dt);
    if((len = strlcpy(new_name, pv_dlf->ps_filename, sizeof(new_name))) >= sizeof(new_name)) { return false; }
    if(snprintf(&new_name[len], sizeof(new_name) - len,
	     ".%04d%02d%02dT%02d%02d%02d",
	     dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds)
	>= sizeof(new_name) - len) { return false; }

    // Rename the datalog file
    return sdcard_rename(pv_dlf->ps_filename, new_name);
  }

  /**
   * Read the datalog file's header.
   *
   * @pre the file MUST have been opened in read mode.
   *
   * @param[in] pv_dlf           the datalog file object.
   * @param[in] ps_contents_type the name of the contents type to expect.
   *                             Can be NULL or empty if we don't want to check the contents type.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool datalogfile_read_header(DataLogFile *pv_dlf, const char *ps_contents_type)
  {
    uint32_t expected_size, size;

    // Check the format and the version
    if(!sdcard_fread(&pv_dlf->file, (uint8_t *)&pv_dlf->header, 2))
    {
      log_error(_logger, "Failed to read format and version from datalog file: '%s'.", pv_dlf->ps_filename);
      return false;
    }
    if( pv_dlf->header.format  != DLOGF_FORMAT_BYTE_ORDER_LITTLE_ENDIAN ||
	pv_dlf->header.version != DATALOG_VERSION)
    {
      log_error(_logger, "Format or version mismatch for datalog file: '%s'.", pv_dlf->ps_filename);
      return false;
    }

    // Read the whole header
    if(!sdcard_rewind(&pv_dlf->file) ||
	!sdcard_fread(&pv_dlf->file, (uint8_t *)&pv_dlf->header, sizeof(DataLogFileHeader)))
    {
      log_error(_logger, "Failed to read header from datalog file: '%s'.", pv_dlf->ps_filename);
      return false;
    }

    // Check the contents type
    if(ps_contents_type && *ps_contents_type)
    {
      if(strncmp(ps_contents_type, pv_dlf->header.contents_type, sizeof(DataLogFileContentsId)) != 0)
      {
	log_error(_logger, "Datalog file '%s' 's contents type is '%s' and not '%s' as expected.", pv_dlf->ps_filename, pv_dlf->header.contents_type, ps_contents_type);
	return false;
      }
    }

    // Check that the file size is the expected one
    expected_size = sizeof(DataLogFileHeader) + pv_dlf->header.nb_records * pv_dlf->header.records_size;
    if((size = sdcard_fsize(&pv_dlf->file)) != expected_size)
    {
      log_error(_logger, "Datalog file '%s' 's size expected to be %d bytes, but it is %d bytes long.", pv_dlf->ps_filename, expected_size, size);
      return false;
    }

    pv_dlf->file_size = expected_size;
    return true;
  }


  /**
   * Write a new record to the datalog file.
   *
   * The datalog file is a circular structure, so if the file is full then the oldest
   * record will be overwritten with the new data.
   *
   * @param[in] pv_dlf   the datalog file object. MUST have been opened.
   * @param[in] pu8_data the data to write to the record. MUST be NOT NULL.
   * @param[in] size     the size, in bytes, of the data to write.
   * @param[in] status   the record status. This value has no meaning for the datalog file,
   *                     only for the user.
   *                     If you do not use the record status value then set it to 0.
   *
   * @return the record identifier of the record that has just been written.
   * @return 0 if the data size is too big for the record.
   * @return 0 if the write to the file failed.
   */
  DataLogFileRecordId datalogfile_write_record(DataLogFile *pv_dlf,
					       uint8_t     *pu8_data,
					       uint32_t     size,
					       uint32_t     status)
  {
    DataLogFileRecordId     rid;
    DataLogFileRecordHeader record_header;

    // Check data size
    if(size > pv_dlf->header.records_size - sizeof(DataLogFileRecordHeader))
    {
      log_error(_logger, "Cannot write data of length %d bytes to datalog file '%s'; data is too big.", size, pv_dlf->ps_filename);
      return 0;
    }

    // Write record header
    record_header.timestamp   = rtc_get_date_as_secs_since_2000();
    record_header.user_status = status;
    record_header.nb_data     = size;
    if( !sdcard_fseek_abs(&pv_dlf->file, pv_dlf->header.dynamic.log_head_seek_pos) ||
	!sdcard_fwrite(   &pv_dlf->file, (const uint8_t *)&record_header, sizeof(record_header)))
    {
      // Failed to write the header.
      log_error(_logger, "Failed to write datalog record header.");
      return 0;
    }

    // Write data
    {
      if(!sdcard_fwrite(&pv_dlf->file, pu8_data, size))
      {
	// Write failed.
	// Try to set up a NULL record header
	(void)(
	    sdcard_fseek_abs(&pv_dlf->file, pv_dlf->header.dynamic.log_head_seek_pos) &&
	    sdcard_fwrite(   &pv_dlf->file,
			     (const uint8_t *)&_NULL_RECORD_HEADER,
			     sizeof(           _NULL_RECORD_HEADER)));
	log_error(_logger, "Failed to write NULL record header.");
	return 0;
      }
    }
    rid = pv_dlf->header.dynamic.log_head_seek_pos;

    // Update the dynamic part of the datalog file's header.
    pv_dlf       ->header.dynamic.log_head_seek_pos += pv_dlf->header.records_size;
    if(pv_dlf    ->header.dynamic.log_head_seek_pos >= pv_dlf->file_size)
    {
      pv_dlf     ->header.dynamic.log_head_seek_pos  = FIRST_RECORD_POS;
    }
    if(pv_dlf    ->header.dynamic.log_head_seek_pos == pv_dlf->header.dynamic.log_tail_seek_pos)
    {
      pv_dlf     ->header.dynamic.log_tail_seek_pos += pv_dlf->header.records_size;
      if(pv_dlf  ->header.dynamic.log_tail_seek_pos >= pv_dlf->file_size)
      {
	pv_dlf   ->header.dynamic.log_tail_seek_pos  = FIRST_RECORD_POS;
      }
    }
    else { pv_dlf->header.dynamic.nb_records_with_data++; }

    // Write the dynamic part of the header to file
    size = (uint8_t *)&pv_dlf->header.dynamic - (uint8_t *)&pv_dlf->header;
    if( !sdcard_fseek_abs(&pv_dlf->file, size) ||
	!sdcard_fwrite(   &pv_dlf->file,
			  (const uint8_t *)&pv_dlf->header.dynamic,
			  sizeof(DataLogFileHeaderDynamic)))
    {
      // Failed to write dynamic part of the header.
      log_error(_logger, "Failed to write datalog header's dynamic part.");
      return 0;
    }

    return rid;
  }

  /**
   * Indicate if the datalog file contains data or not.
   *
   * @param[in] pv_dlf the datalog file object. MUST have been opened.
   *
   * @return true  if there are data.
   * @return false otherwise.
   */
  bool datalogfile_is_empty(DataLogFile *pv_dlf)
  {
    return pv_dlf->header.dynamic.nb_records_with_data == 0;
  }

  /**
   * Get the identifier of the latest record written to the datalog file.
   *
   * @param[in] pv_dlf the datalog file object. MUST have been opened.
   *
   * @return the identifier.
   * @return 0 if the datalog file is empty.
   */
  DataLogFileRecordId datalogfile_latest_record(DataLogFile *pv_dlf)
  {
    return datalogfile_is_empty(pv_dlf) ?
	0 :
	datalogfile_previous_record(pv_dlf, pv_dlf->header.dynamic.log_head_seek_pos);
  }


#define rid_is_valid(pv_dlf, rid)  ((rid) >= FIRST_RECORD_POS && (rid) < (pv_dlf)->file_size)

  /**
   * Return the identifier of the record written immediately after a given record.
   *
   * @param[in] pv_dlf the datalog file object. MUST have been opened.
   * @param[in] rid    the identifier of the reference record.
   *
   * @return the next record.
   * @return 0 if there is no next record.
   */
  DataLogFileRecordId datalogfile_next_record(DataLogFile *pv_dlf, DataLogFileRecordId rid)
  {
    if(!rid_is_valid(pv_dlf, rid) || datalogfile_is_empty(pv_dlf) ||
	rid == pv_dlf->header.dynamic.log_head_seek_pos) { return 0; }

    rid += pv_dlf->header.records_size;
    if(rid >= pv_dlf->file_size) { rid = FIRST_RECORD_POS; }

    return (rid == pv_dlf->header.dynamic.log_head_seek_pos) ? 0 : rid;
  }

  /**
   * Return the identifier of the record written immediately before a given record.
   *
   * @param[in] pv_dlf the datalog file object. MUST have been opened.
   * @param[in] rid    the identifier of the reference record.
   *
   * @return the previous record.
   * @return 0 if there is no next record.
   */
  DataLogFileRecordId datalogfile_previous_record(DataLogFile *pv_dlf, DataLogFileRecordId rid)
  {
    if(!rid_is_valid(pv_dlf, rid) || datalogfile_is_empty(pv_dlf) ||
	rid == pv_dlf->header.dynamic.log_tail_seek_pos) { return 0; }

    if(rid < pv_dlf->header.records_size + FIRST_RECORD_POS)
    {
      rid = pv_dlf->file_size - pv_dlf->header.records_size;
    }
    else { rid -= pv_dlf->header.records_size; }

    return rid;
  }


  /**
   * Read a record header.
   *
   * @post the read/write position in the file it set to the first data byte.
   *
   * @param[in]  pv_dlf    the datalog file object. MUST have been opened.
   * @param[in]  rid       the record identifier. Can be 0.
   * @param[out] pv_header where to write the header data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool datalogfile_record_header(DataLogFile             *pv_dlf,
				 DataLogFileRecordId      rid,
				 DataLogFileRecordHeader *pv_header)
  {
    return rid_is_valid(  pv_dlf, rid)       &&
	sdcard_fseek_abs(&pv_dlf->file, rid) &&
	sdcard_fread(    &pv_dlf->file, (uint8_t *)pv_header, sizeof(DataLogFileRecordHeader));
  }

  /**
   * Get the data from a record.
   *
   * @param[in]  pv_dlf      the datalog file object. MUST have been opened.
   * @param[in]  rid         the record identifier. Can be 0.
   * @param[out] pu8_data    the buffer where the data are written to.
   * @param[in]  buffer_size the buffer's size.
   * @param[out] pv_header   where the record's header (meta data) are written to.
   *                         Can be NULL if you are not interested in these informations.
   *
   * @return pu8_data.
   * @return NULL if rid is not valid.
   * @return NULL if failed to read the data from the file.
   * @return NULL if the buffer is too small.
   */
  uint8_t *datalogfile_record_data(DataLogFile             *pv_dlf,
				   DataLogFileRecordId      rid,
				   uint8_t                 *pu8_data,
				   uint32_t                 buffer_size,
				   DataLogFileRecordHeader *pv_header)
  {
    DataLogFileRecordHeader record_header;

    if(!pv_header)  { pv_header = &record_header; }
    pv_header->timestamp = pv_header->user_status = pv_header->nb_data = 0;

    return (
	rid_is_valid(             pv_dlf, rid)            &&
	datalogfile_record_header(pv_dlf, rid, pv_header) &&
	record_header.nb_data <= buffer_size              &&
	sdcard_fread(            &pv_dlf->file, pu8_data, pv_header->nb_data)) ?
	    pu8_data : NULL;
  }

  /**
   * Get the status of a given record.
   *
   * @param[in]  pv_dlf      the datalog file object. MUST have been opened.
   * @param[in]  rid         the record identifier. Can be 0.
   * @param[out] pu32_status where the status is written to. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool datalogfile_record_status(DataLogFile        *pv_dlf,
				 DataLogFileRecordId rid,
				 uint32_t           *pu32_status)
  {
    DataLogFileRecordHeader record_header;
    bool                    res = false;

    if(datalogfile_record_header(pv_dlf, rid, &record_header))
    {
      *pu32_status = record_header.user_status;
    }

    return res;
  }

  /**
   * Update the status, change it to a new value, of a given record.
   *
   * @param[in] pv_dlf     the datalog file object. MUST have been opened.
   * @param[in] rid        the record identifier. Can be 0.
   * @param[in] new_status the new status.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool datalogfile_update_record_status(DataLogFile        *pv_dlf,
					DataLogFileRecordId rid,
					uint32_t            new_status)
  {
    DataLogFileRecordHeader record_header;
    bool                    res = false;

    if(datalogfile_record_header(pv_dlf, rid, &record_header) &&
	record_header.timestamp && record_header.nb_data)
    {
      record_header.user_status = new_status;
      res = sdcard_fseek_abs(&pv_dlf->file, rid) &&
	  sdcard_fwrite(     &pv_dlf->file, (const uint8_t *)&record_header, sizeof(record_header));
    }

    return res;
  }

  /**
   * Update the status, OR the current value with flags, of a given record.
   *
   * @param[in] pv_dlf     the datalog file object. MUST have been opened.
   * @param[in] rid        the record identifier. Can be 0.
   * @param[in] flags      the value to OR the current status with.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool datalogfile_update_record_status_add_flags(DataLogFile        *pv_dlf,
						  DataLogFileRecordId rid,
						  uint32_t            flags)
  {
    DataLogFileRecordHeader record_header;
    bool                    res = false;

    if(datalogfile_record_header(pv_dlf, rid, &record_header) &&
	record_header.timestamp && record_header.nb_data)
    {
      record_header.user_status |= flags;
      res = sdcard_fseek_abs(&pv_dlf->file, rid) &&
	  sdcard_fwrite(&pv_dlf->file, (const uint8_t *)&record_header, sizeof(record_header));
    }

    return res;
  }


  /**
   * Indicate if a record's status equals a given value or not.
   *
   * @param[in] pv_dlf the datalog file object. MUST have been opened.
   * @param[in] rid    the record identifier. Can be 0.
   * @param[in] status the value to compare to.
   *
   * @return true  if the record's status is status.
   * @return false otherwise.
   * @return false if failed to read datalog file.
   */
  bool datalogfile_record_status_is(DataLogFile        *pv_dlf,
				    DataLogFileRecordId rid,
				    uint32_t            status)
  {
    DataLogFileRecordHeader record_header;

    return datalogfile_record_header(pv_dlf, rid, &record_header) &&
	record_header.user_status == status;
  }

  /**
   * Indicate if a record's status ORed with a given value is 0 or not.
   *
   * @param[in] pv_dlf the datalog file object. MUST have been opened.
   * @param[in] rid    the record identifier. Can be 0.
   * @param[in] flag   the value to OR with.
   *
   * @return true  if the OR is not 0.
   * @return false otherwise.
   * @return false if failed to read datalog file.
   */
  bool datalogfile_record_status_has_flag(DataLogFile        *pv_dlf,
					  DataLogFileRecordId rid,
					  uint32_t            flag)
  {
    DataLogFileRecordHeader record_header;

    return datalogfile_record_header(pv_dlf, rid, &record_header) &&
	(record_header.user_status | flag);
  }


#ifdef __cplusplus
}
#endif
