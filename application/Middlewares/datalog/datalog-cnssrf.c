/*
 *  Implementation of a fixed size entry datalog file to store CNSSRF data.
 *
 *  @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */

#include <string.h>
#include "datalog-cnssrf.h"
#include "config.h"
#include "datalogfile.h"
#include "board.h"
#include "logger.h"
#include "utils.h"


#ifdef __cplusplus
extern "C" {
#endif


#if !defined DATALOG_CNSSRF_NB_RECORDS || DATALOG_CNSSRF_NB_RECORDS < 100
#error "You MUST define a number of records stored in the datalog file > 100 using DATALOG_CNSSRF_NB_RECORDS."
#endif
#define DATALOG_CNSSRF_RECORDS_SIZE  512

#ifndef DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME
#define DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME  20
#else
#if DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME < 1
#error "DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME must be > 0."
#endif
#endif
#ifndef DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES
#define DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES  16
#elif   DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES < 0
#error "DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES must be >= 0"
#elif   DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES > 40
#error "DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES must be <= 40"
#endif

#define CONTENTS_TYPE    "CNSSRF-data+meta"
#define INDEXES_FILE_EXT ".indexes"


  CREATE_LOGGER(datalog_cnssrf);
#undef  _logger
#define _logger datalog_cnssrf


  /**
   * Defines the record's status flags.
   */
  typedef enum DataLogCNSSRFStatus
  {
    DATALOG_CNSSRF_STATUS_NONE                = 0,          ///< No status
    DATALOG_CNSSRF_STATUS_DATA_NOT_SENT       = 0x10000000, ///< The record contains data, none of which has been sent.
    DATALOG_CNSSRF_STATUS_DATA_PARTIALLY_SENT = 0x20000000, ///< The record contains data and some of it has been sent.
    DATALOG_CNSSRF_STATUS_DATA_ALL_SENT       = 0x30000000  ///< The record contains data and all of it has been sent.
  }
  DataLogCNSSRFStatus;
#define STATUS_STATUS_MASK        0xF0000000
#define STATUS_NB_BYTES_USED_MASK 0x000000FF
#define status_from_record_status(status)         ((status) & STATUS_STATUS_MASK)
#define nb_bytes_used_from_record_status(status)  ((status) & STATUS_NB_BYTES_USED_MASK)
#define set_nb_bytes_used_from_record_status(status, nb)  \
  (status) &= ~STATUS_NB_BYTES_USED_MASK; \
  (status) |=  (nb) & STATUS_NB_BYTES_USED_MASK
#define build_status(status, nb_bytes_used)  \
  ((status) | ((nb_bytes_used) & STATUS_NB_BYTES_USED_MASK))


  /**
   * Store a new status for a record.
   *
   * It make the association between a record identifier and it's new status.
   */
  typedef struct DataLogCNSSRFRecordNewStatus
  {
    DataLogFileRecordId rid;    ///< The record identifier.
    uint32_t            status; ///< The status
  }
  DataLogCNSSRFRecordNewStatus;

  /**
   * Store the meta data associated with building a CNSSRF data frame..
   */
  typedef struct DataLogCNSSRBuildMeta
  {
    /// The number or records used to build the frame.
    uint32_t nb_records_used;

    /// Their new statuses.
    DataLogCNSSRFRecordNewStatus statuses[DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME];

    /// The last search index. O if not set.
    DataLogFileRecordId rid_last_search;
  }
  DataLogCNSSRBuildMeta;

#ifdef USE_DATALOG_CNSSRF_INDEXES
  /**
   * Keep indexes to speed up datalog records usage.
   */
  typedef struct DataLogCNSSRFIndexes
  {
    /**
     * Store the record id of the latest record at the time
     * the last frame was built using record data.
     */
    DataLogFileRecordId rid_of_last_frame_build;

    /**
     * Store the record id of the oldest record we looked at the last time
     * we searched for a record containing unsent data.
     * So this identifier may point to unsent data.
     * All records between this identifier and rid_of_last_frame_build should
     * all contain data that have been sent.
     */
    DataLogFileRecordId rid_of_last_not_sent_search;
  }
  DataLogCNSSRFIndexes;
#define INDEX_KEY_LAST_FRAME_BUILD     'b'
#define INDEX_KEY_LAST_NOT_SENT_SEARCH 's'
#define INDEX_KEY_PAIRS_SEP            '\n'

  static DataLogCNSSRFIndexes  _datalog_cnssrf_file_indexes;
  static DataLogCNSSRFIndexes  _datalog_cnssrf_file_indexes_ref;  // The reference used to detect changes

  static bool datalog_cnssrf_build_indexes_filename(char *ps_buffer, uint32_t size);
  static bool datalog_cnssrf_read_indexes_file(     void);
  static bool datalog_cnssrf_save_indexes_file(     void);
  static void datalog_cnssrf_copy_indexes(          DataLogCNSSRFIndexes       *pv_dest,
					            const DataLogCNSSRFIndexes *pv_src);
  static void datalog_cnssrf_parse_indexes_file_data(DataLogCNSSRFIndexes      *pv_ixs,
						     const uint8_t             *pu8_data,
						     uint32_t                   size);
  static bool datalog_cnssrf_indexes_are_the_same(const DataLogCNSSRFIndexes *pv_ixs1,
						  const DataLogCNSSRFIndexes *pv_ixs2);
#endif  // USE_DATALOG_CNSSRF_INDEXES

  static DataLogFile           _datalog_cnssrf_file;
  static uint8_t               _datalog_cnssrf_record_buffer[DATALOG_CNSSRF_RECORDS_SIZE];
  static DataLogCNSSRBuildMeta _datalog_cnssrf_records_build_meta;

  static bool _datalog_cnssrf_has_been_initialised = false;

  static bool datalog_cnssrf_process_record_to_frame(CNSSRFDataFrame         *pv_frame,
						     DataLogFileRecordId      rid,
						     DataLogFileRecordHeader *pv_header,
						     uint16_t                 size_max,
						     bool                    *pb_done);



  /**
   * Initialise the datalogging for the CNSSRF data.
   *
   * @param[in] ps_filename the datalog's file name. MUST be NOT NULL and NOT empty.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool datalog_cnssrf_init(const char *ps_filename)
  {
    if(!_datalog_cnssrf_has_been_initialised)
    {
      if(!datalogfile_init(&_datalog_cnssrf_file,
			   ps_filename,
			   DATALOG_CNSSRF_RECORDS_SIZE,
			   DATALOG_CNSSRF_NB_RECORDS)) { goto exit; }
      _datalog_cnssrf_records_build_meta.nb_records_used = 0;

      // Open the datalog file
      _datalog_cnssrf_has_been_initialised = datalogfile_open(&_datalog_cnssrf_file, CONTENTS_TYPE, true);

#ifdef USE_DATALOG_CNSSRF_INDEXES
      // Read the indexes files
      datalog_cnssrf_read_indexes_file();
#endif // USE_DATALOG_CNSSRF_INDEXES
    }

    exit:
    return _datalog_cnssrf_has_been_initialised;
  }

  /**
   * De-initialise (close) the datalogging for CNSSRF data.
   */
  void datalog_cnssrf_deinit(void)
  {
    if(_datalog_cnssrf_has_been_initialised)
    {
#ifdef USE_DATALOG_CNSSRF_INDEXES
      datalog_cnssrf_save_indexes_file();
#endif
      datalogfile_close(&_datalog_cnssrf_file);
      _datalog_cnssrf_has_been_initialised = false;
    }
  }

  /**
   * Flush all file caches; make sure the data are actually written to disk.
   */
  void datalog_cnssrf_sync()
  {
    if(_datalog_cnssrf_has_been_initialised)
    {
      datalogfile_sync(&_datalog_cnssrf_file);
#ifdef USE_DATALOG_CNSSRF_INDEXES
      datalog_cnssrf_save_indexes_file();
#endif // USE_DATALOG_CNSSRF_INDEXES
    }
  }


#ifdef USE_DATALOG_CNSSRF_INDEXES
  /**
   * Build the indexes file's name.
   *
   * @param[out] ps_buffer the buffer where the filename will be built. MUST be NOT NULL.
   * @param[in]  size      the buffer's size.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool datalog_cnssrf_build_indexes_filename(char *ps_buffer, uint32_t size)
  {
    size_t s;
    bool   res;

    strlcpy(      ps_buffer, datalogfile_filename(&_datalog_cnssrf_file), size);
    s   = strlcat(ps_buffer, INDEXES_FILE_EXT,                            size);
    res = s < size;
    if(!res)
    {
      log_error(_logger,
		"Failed to build datalog indexes file's name; "
		"buffer is too small: %i < %i.", size, s);
    }
    else { log_debug(_logger, "Datalog indexes file is: %s", ps_buffer); }

    return res;
  }

  /**
   * Read the datalog indexes file.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool datalog_cnssrf_read_indexes_file(void)
  {
    File     file;
    uint32_t size;
    char     buffer[100];
    bool     res = false;

    _datalog_cnssrf_file_indexes.rid_of_last_frame_build     = 0;
    _datalog_cnssrf_file_indexes.rid_of_last_not_sent_search = 0;

    // Open the indexes file
    // First build the indexes file name
    if(!datalog_cnssrf_build_indexes_filename(buffer, sizeof(buffer))) { goto exit; }

    // Check if the file exists. If it doesn't then we have nothing to do
    if(!sdcard_exists(buffer)) { res = true; goto exit; }

    // If the datalog file has just been created then the indexes are no longer valid, so delete the indexes file
    if(datalogfile_file_has_been_created(&_datalog_cnssrf_file) && !sdcard_remove(buffer))
    {
	log_error(_logger, "Failed to remove datalog indexes file '%s'.", buffer);
	goto exit;
    }

    // Read the indexes file
    if(!sdcard_fopen(&file, buffer, FILE_OPEN | FILE_READ))
    {
      log_error(_logger, "Failed to open indexes file '%s' in read mode.", buffer);
      goto exit;
    }
    size = sdcard_fread_at_most(&file, (uint8_t *)buffer, sizeof(buffer), NULL);
    sdcard_fclose(              &file);

    // Get the indexes from the file's data
    datalog_cnssrf_parse_indexes_file_data(&_datalog_cnssrf_file_indexes,
					   (const uint8_t *)buffer,
					   size);
    res = true;

    exit:
    datalog_cnssrf_copy_indexes(&_datalog_cnssrf_file_indexes_ref, &_datalog_cnssrf_file_indexes);
    return res;
  }

  /**
   * Save the indexes file.
   *
   * @return true on success.
   * @return false otherwise.
   */
  static bool datalog_cnssrf_save_indexes_file(void)
  {
    File     file;
    char     buffer[100];
    uint16_t size;
    bool     res = false;

    // If the indexes are unchanged then there is no need to save them
    if(datalog_cnssrf_indexes_are_the_same(&_datalog_cnssrf_file_indexes_ref,
					   &_datalog_cnssrf_file_indexes))
    {
      res = true;
      goto exit;
    }

    // Save the indexes to the indexes file.
    // First build the file's name
    if(!datalog_cnssrf_build_indexes_filename(buffer, sizeof(buffer))) { goto exit; }

    // Open the file, and truncate it.
    if(!sdcard_fopen(&file, buffer, FILE_TRUNCATE | FILE_WRITE))
    {
      log_error(_logger, "Failed top open file '%s' in write mode.", buffer);
      goto exit;
    }

    // Write the indexes.
    // The buffer should be big enough, so I do not check the resulting size.
    size = snprintf(buffer, sizeof(buffer), "%c%u%c%c\%u",
		    INDEX_KEY_LAST_FRAME_BUILD,
		    (unsigned int)_datalog_cnssrf_file_indexes.rid_of_last_frame_build,
		    INDEX_KEY_PAIRS_SEP,
		    INDEX_KEY_LAST_NOT_SENT_SEARCH,
		    (unsigned int)_datalog_cnssrf_file_indexes.rid_of_last_not_sent_search);
    res  = sdcard_fwrite(&file, (const uint8_t *)buffer, size);
    sdcard_fclose(       &file);

    // Copy indexes to reference.
    datalog_cnssrf_copy_indexes(&_datalog_cnssrf_file_indexes_ref, &_datalog_cnssrf_file_indexes);

    exit:
    return res;
  }

  /**
   * Pare the data from an indexes file.
   *
   * @param[out] pv_ixs   where the parsed data are written to. MUST be NOT NULL.
   * @param[in]  pu8_data the data to parse. MUST be NOT NULL.
   * @param[in]  size     the number of data bytes to parse.
   */
  static void datalog_cnssrf_parse_indexes_file_data(DataLogCNSSRFIndexes *pv_ixs,
						     const uint8_t        *pu8_data,
						     uint32_t              size)
  {
    char *pc, *pc_end;

    for(pc = (char *)pu8_data, pc_end = pc + size; pc < pc_end - 1; pc++)
    {
      switch(*pc++)
      {
	case INDEX_KEY_LAST_FRAME_BUILD:
	  pv_ixs->rid_of_last_frame_build     =
	      strn_to_uint_with_default_and_sep(pc, pc_end - pc, INDEX_KEY_PAIRS_SEP, 0, &pc);
	  break;

	case INDEX_KEY_LAST_NOT_SENT_SEARCH:
	  pv_ixs->rid_of_last_not_sent_search =
	      strn_to_uint_with_default_and_sep(pc, pc_end - pc, INDEX_KEY_PAIRS_SEP, 0, &pc);
	  break;

	default:
	  ; // Do nothing
      }
    }
  }

  /**
   * Copy the values from one index storage structure to another.
   *
   *
   * @param[out] pv_dest where the indexes will be copied to. MUST be NOT NULL.
   * @param[in]  pv_src  from where the indexes are copied from. MUST be NOT NULL.
   */
  static void datalog_cnssrf_copy_indexes(DataLogCNSSRFIndexes       *pv_dest,
					  const DataLogCNSSRFIndexes *pv_src)
  {
    pv_dest->rid_of_last_frame_build     = pv_src->rid_of_last_frame_build;
    pv_dest->rid_of_last_not_sent_search = pv_src->rid_of_last_not_sent_search;
  }

  /**
   * Indicate if to structures used to store indexes store the same index values or not.
   *
   * @param[in] pv_ixs1 the first indexes structure. MUST be NOT NULL.
   * @param[in] pv_ixs2 the second indexes structure. MUST be NOT NULL.
   *
   * @return true  if the indexes are the same.
   * @return false otherwise.
   */
  static bool datalog_cnssrf_indexes_are_the_same(const DataLogCNSSRFIndexes *pv_ixs1,
						  const DataLogCNSSRFIndexes *pv_ixs2)
  {
    return pv_ixs1->rid_of_last_frame_build  == pv_ixs2->rid_of_last_frame_build &&
	pv_ixs1->rid_of_last_not_sent_search == pv_ixs2->rid_of_last_not_sent_search;
  }
#endif // USE_DATALOG_CNSSRF_INDEXES


  /**
   * Add a CNSSRF data frame to the data logger.
   *
   * @param[in] pv_frame the frame to add.
   *
   * @return true  on success.
   * @return false if the frame has no data or no meta data.
   * @return false if failed to write to data logger.
   */
  bool datalog_cnssrf_add(const CNSSRFDataFrame *pv_frame)
  {
    const uint8_t *pu8_src;
    uint8_t       *pu8_dest, *pu8_end;
    uint32_t      nb_data = 0;

    if(!pv_frame || !cnssrf_data_frame_data(pv_frame) ||
	!cnssrf_data_frame_meta_data_count( pv_frame))
    {
      log_debug(_logger, "No frame or no data in the frame to add to datalog.");
      goto error_exit;
    }

    // First write the number of data bytes to the record buffer
    _datalog_cnssrf_record_buffer[nb_data++] = cnssrf_data_frame_size(pv_frame);

    // Then copy the frame data to the record buffer
    for(pu8_src   =            cnssrf_data_frame_data(pv_frame),
	pu8_dest  = &_datalog_cnssrf_record_buffer[nb_data],
	pu8_end   = pu8_dest + cnssrf_data_frame_size(pv_frame);
	pu8_dest != pu8_end; )
    {
      *pu8_dest++ = *pu8_src++;
    }
    nb_data += cnssrf_data_frame_size(pv_frame);

    // Then the size, in byte, of the meta data
    _datalog_cnssrf_record_buffer[nb_data++] = cnssrf_data_frame_meta_data_size(pv_frame);

    // Align the data to come on 32 bits addresses
    // With this alignment it is then possible to just cast the meta data values, whatever their actual type
    if(nb_data & 0x3) { nb_data += 4u - (nb_data & 0x3); }

    // Then copy the frame meta data to the record buffer
    for(pu8_src   = (const uint8_t *)cnssrf_data_frame_meta_data(     pv_frame),
	pu8_dest  = &_datalog_cnssrf_record_buffer[nb_data],
	pu8_end   = pu8_dest +       cnssrf_data_frame_meta_data_size(pv_frame);
	pu8_dest != pu8_end; )
    {
      *pu8_dest++ = *pu8_src++;
    }
    nb_data += cnssrf_data_frame_meta_data_size(pv_frame);

    // Write to the record, with it's status
    if(datalogfile_write_record(&_datalog_cnssrf_file,
				 _datalog_cnssrf_record_buffer,
				 nb_data,
				 DATALOG_CNSSRF_STATUS_DATA_NOT_SENT) == 0)
    {
      log_error(_logger, "Failed to write datalog record.");
      goto error_exit;
    }

#ifdef USE_DATALOG_CNSSRF_INDEXES
    // If the indexes are caught up by the datalog tail, then resets the indexes
    if( _datalog_cnssrf_file_indexes.rid_of_last_frame_build ==
	_datalog_cnssrf_file.header.dynamic.log_tail_seek_pos ||
	_datalog_cnssrf_file_indexes.rid_of_last_not_sent_search ==
	_datalog_cnssrf_file.header.dynamic.log_tail_seek_pos)
    {
      _datalog_cnssrf_file_indexes.rid_of_last_frame_build     = 0;
      _datalog_cnssrf_file_indexes.rid_of_last_not_sent_search = 0;
      datalog_cnssrf_save_indexes_file();
    }
#endif

    return true;

    error_exit:
    return false;
  }



  /**
   * Get a CNSSRF data frame from the buffer.
   *
   * This function uses frame slicing and rewriting to potentially build one frame from
   * several ones.
   * The idea is to cut into pieces the frames that are too long to send in a single transmission,
   * or to be able to send several frames or pieces of frames in a single transmission.
   *
   * To build the output frame we'll use data starting with the most recent ones, not already sent.
   * To avoid going too far back in the past it is possible to set a date before which we are not
   * interested in getting the data from.
   *
   * The caller of this function must call the function datalog_cnssrf_frame_has_been_sent()
   * once the frame built by this function has been successfully sent so that the status of the
   * data used to build the frame is updated.
   * If the sending of the frame fails then do not call this function. this way the data may
   * be used to build another frame and they'll get another chance to be sent.
   *
   * @param[out] pv_frame    where the frame will be written to. MUST be NOT NULL.
   *                         It's data buffer must be big enough to contains at least size_max bytes;
   *                         pv_frame->buffer_size MUST be set.
   * @param[in]  size_max    the maximum number of data bytes to write to the output frame.
   * @param[in]  since       the timestamp of the oldest data we are allowed to get.
   *                         If 0 then there is no time limitation; we can use all the data.
   *                         Whatever the value it is always possible to use the latest value if it
   *                         has not already been sent.
   * @param[out] pb_has_more Indicate if there are data left, that met the criteria,
   *                         that have not been written to the frame for lack of space (true)
   *                         or not (false).
   *                         Using this parameter you can known if you should call this function
   *                         ahead of schedule to get and send the remaining data.
   *                         If set to NULL the search for remaining data is not performed.
   *
   * @return true  if the data have been written to the frame. pv_frame->data_count is set with
   *               the data count.
   * @return true  if there are no data to send. pv_frame->data_count is set to 0.
   * @return false in case of error.
   */
  bool datalog_cnssrf_get_frame(CNSSRFDataFrame *pv_frame,
				uint16_t         size_max,
				ts2000_t         since,
				bool            *pb_has_more)
  {
    DataLogFileRecordId           rid;
    DataLogFileRecordHeader       header;
    DataLogCNSSRFRecordNewStatus *pv_rns;
    DataLogCNSSRFStatus           status;
    bool                          done;
#ifdef USE_DATALOG_CNSSRF_INDEXES
    bool                          rid_changed;
    bool                          can_jump_to_last_build_rid, can_jump_to_last_search_rid;
#endif // USE_DATALOG_CNSSRF_INDEXES

    cnssrf_data_frame_clear(pv_frame);
    _datalog_cnssrf_records_build_meta.nb_records_used = 0;

    if(pv_frame->buffer_size < size_max)
    {
      if(       !pv_frame->buffer_size) { goto error_exit; }
      size_max = pv_frame->buffer_size;
    }

#ifdef USE_DATALOG_CNSSRF_INDEXES
    can_jump_to_last_build_rid  = true;
    can_jump_to_last_search_rid = true;
#endif
    for(rid  = datalogfile_latest_record(&_datalog_cnssrf_file),
	done = false;
	rid; )
    {
#ifdef USE_DATALOG_CNSSRF_INDEXES
      rid_changed = false;
#endif

      // Read the record header
      board_watchdog_reset();
      if(!datalogfile_record_header(&_datalog_cnssrf_file, rid, &header)) { goto error_exit; }
      if(!datalogfile_record_header_is_valid(&header) ||
	  (status = status_from_record_status(header.user_status)) == DATALOG_CNSSRF_STATUS_NONE)
      {
	// Get previous record
	rid = datalogfile_previous_record(&_datalog_cnssrf_file, rid);
	continue;  // And loop
      }

#ifdef USE_DATALOG_CNSSRF_INDEXES
      // If first record with all data send then go to last build index, if it exists.
      if(can_jump_to_last_build_rid                            &&
	  _datalog_cnssrf_file_indexes.rid_of_last_frame_build &&
	  status == DATALOG_CNSSRF_STATUS_DATA_ALL_SENT)
      {
	rid                         = _datalog_cnssrf_file_indexes.rid_of_last_frame_build;
	can_jump_to_last_build_rid  = false;
	rid_changed                 = true;
      }
      // If we are at the last build index and there is a last search index then go at that last search index
      if(can_jump_to_last_search_rid                               &&
	  _datalog_cnssrf_file_indexes.rid_of_last_not_sent_search &&
	  rid    == _datalog_cnssrf_file_indexes.rid_of_last_frame_build)
      {
	rid                         = _datalog_cnssrf_file_indexes.rid_of_last_not_sent_search;
	can_jump_to_last_build_rid  = false;
	can_jump_to_last_search_rid = false;
	rid_changed                 = true;
      }
      // If rid has changed then read the record's header for the new rid
      if(rid_changed) continue; // Loop to use the new rid.
#endif // USE_DATALOG_CNSSRF_INDEXES

      if(status != DATALOG_CNSSRF_STATUS_DATA_ALL_SENT)
      {
	if(!datalog_cnssrf_process_record_to_frame(pv_frame, rid, &header, size_max, &done)) { goto error_exit; }

	// Store the new status so that it can be written later if the frame is sent.
	pv_rns         = &_datalog_cnssrf_records_build_meta.statuses[_datalog_cnssrf_records_build_meta.nb_records_used++];
	pv_rns->rid    = rid;
	pv_rns->status = header.user_status;
      }
      if(done || header.timestamp < since || _datalog_cnssrf_records_build_meta.nb_records_used ==
    	    DATALOG_CNSSRF_NB_MAX_RECORDS_TO_USE_TO_BUILD_ONE_FRAME) break;  // Our search is done

      // Get previous record
      rid = datalogfile_previous_record(&_datalog_cnssrf_file, rid);
    }

    // If not all of the current record data have been used then the last search index
    // is the current record.
    _datalog_cnssrf_records_build_meta.rid_last_search   = 0;
    if(status_from_record_status(header.user_status)    != DATALOG_CNSSRF_STATUS_DATA_ALL_SENT)
    {
      _datalog_cnssrf_records_build_meta.rid_last_search = rid;
    }

    // Do we need to indicate if there are more data left?
    if(pb_has_more)
    {
      *pb_has_more = false;
      if(_datalog_cnssrf_records_build_meta.rid_last_search) { *pb_has_more = true; }
      else
      {
	// Check the previous records
	while((rid = datalogfile_previous_record(&_datalog_cnssrf_file, rid)))
	{
	  board_watchdog_reset();
	  if(!datalogfile_record_header(&_datalog_cnssrf_file, rid, &header)) { goto exit; } // Not a deadly error; keep the frame we have built
	  if(!datalogfile_record_header_is_valid(&header)) continue; // Check next record.
	  if(header.timestamp < since) break; // We're done. Previous records should be earlier than 'since'.
	  if(status_from_record_status(header.user_status) != DATALOG_CNSSRF_STATUS_DATA_ALL_SENT)
	  {
	    // Our search is done. We have found a record that has not been completely sent.
	    _datalog_cnssrf_records_build_meta.rid_last_search = rid;
	    *pb_has_more = true;
	    break;
	  }
	}
      }
    }

    exit:
    return true;

    error_exit:
    cnssrf_data_frame_clear(pv_frame);
    return false;
  }


  /**
   * Function to call after a CNSSRF frame previously built have successfully been sent.
   *
   * This function update the records' statuses so that their data are not used twice.
   * Also updates the indexes.
   *
   * It used the current build meta data to do so.
   */
  void datalog_cnssrf_frame_has_been_sent(void)
  {
    uint32_t                      i;
    DataLogCNSSRFRecordNewStatus *pv_rns;

    // Check that we have something to do.
    if(_datalog_cnssrf_records_build_meta.nb_records_used == 0) return;

    // Update the records' statuses
    for(i = 0; i < _datalog_cnssrf_records_build_meta.nb_records_used; i++)
    {
      pv_rns = &_datalog_cnssrf_records_build_meta.statuses[i];

      // We do not really care if the update fails. So no test here.
      // In particular, do not update the indexes to avoid to re-use the data that actually have bee sent.
      datalogfile_update_record_status(&_datalog_cnssrf_file, pv_rns->rid, pv_rns->status);
    }

#ifdef USE_DATALOG_CNSSRF_INDEXES
    // Update the indexes with the ones stored in the build meta data.
    // Build index
    _datalog_cnssrf_file_indexes.rid_of_last_frame_build     =
	_datalog_cnssrf_records_build_meta.statuses
	[_datalog_cnssrf_records_build_meta.nb_records_used - 1].rid;
    // Search index
    _datalog_cnssrf_file_indexes.rid_of_last_not_sent_search =
	_datalog_cnssrf_records_build_meta.rid_last_search;
#endif // USE_DATALOG_CNSSRF_INDEXES

    // Clear the meta data to avoid re-using them
    _datalog_cnssrf_records_build_meta.nb_records_used = 0;
    _datalog_cnssrf_records_build_meta.rid_last_search = 0;

    // flush data to file(s).
    datalog_cnssrf_sync();
  }


  /**
   * Add data to a CNSSRF frame using a data log record.
   *
   * @post the indexes are updated.
   *
   * @param[out]    pv_frame  the CNSSRF frame to write to. MUST be NOT NULL.
   * @param[in]     rid       the record identifier. MUST be valid.
   * @param[in,out] pv_header the record's header, with the current values. MUST be NOT NULL.
   *                          The 'user_status' value is updated by this function if part
   *                          or all of the record's data have been used to build the frame.
   * @param[in]     size_max  the output frame data's maximum size.
   *                          MUST be <= pv_frame->buffer_size.
   * @param[out]    pb_done   Indicate if the frame is done (true),
   *                          or if we may still be able to write some data to the frame (false).
   *
   * @return true  if data have been written to the frame.
   *               pv_frame->data_count is updated to take into account the bytes that have been added.
   * @return true  if no data have been written because the frame is done.
   * @return false in case of error.
   */
  static bool datalog_cnssrf_process_record_to_frame(CNSSRFDataFrame         *pv_frame,
						     DataLogFileRecordId      rid,
						     DataLogFileRecordHeader *pv_header,
						     uint16_t                 size_max,
						     bool                    *pb_done)
  {
    uint16_t        pos, data_size, nb_bytes_used;
    CNSSRFDataFrame src_frame;

    *pb_done = false;

    switch(status_from_record_status(pv_header->user_status))
    {
      case DATALOG_CNSSRF_STATUS_NONE:
      case DATALOG_CNSSRF_STATUS_DATA_ALL_SENT:
	goto exit;  // Nothing to do.
      default:
	; // Do nothing
    }

    // Read the record's data
    if(!datalogfile_record_data(&_datalog_cnssrf_file,
				rid,
				_datalog_cnssrf_record_buffer, sizeof(_datalog_cnssrf_record_buffer),
				NULL))
    {
      log_error(_logger, "Failed to read data from record with id: %d.", rid);
      goto error_exit;
    }
    data_size                =  _datalog_cnssrf_record_buffer[0];
    src_frame.pu8_buffer     = &_datalog_cnssrf_record_buffer[1];
    src_frame.meta_count     =  _datalog_cnssrf_record_buffer[data_size + 1] / sizeof(CNSSRFMetaData);
    pos                      = data_size + 2; if(pos & 0x3) { pos += 4 - (pos & 0x3); }
    src_frame.pv_meta_buffer = (CNSSRFMetaData *)&_datalog_cnssrf_record_buffer[pos];

    if((nb_bytes_used = nb_bytes_used_from_record_status(pv_header->user_status)))
    {
      // Copy to output frame the global values that are in the already used record's data.
      src_frame.data_count   = nb_bytes_used;  // Only used the already used data
      if(!cnssrf_data_frame_copy_global_values(pv_frame, &src_frame))  { goto error_exit; }
    }

    // Copy the data from the record to the output frame
    src_frame.data_count = data_size;
    if(!cnssrf_data_frame_copy_from_data_index_size_max(
	pv_frame,
	&src_frame,
	nb_bytes_used,
	size_max,
	DATALOG_CNSSRF_DO_NOT_SPLIT_CHANNEL_DATA_WHEN_LESS_THAN_X_BYTES,
	&nb_bytes_used)) { goto error_exit; }

    pv_header->user_status   = build_status((nb_bytes_used == data_size) ?
	DATALOG_CNSSRF_STATUS_DATA_ALL_SENT : DATALOG_CNSSRF_STATUS_DATA_PARTIALLY_SENT,
	nb_bytes_used);

    *pb_done = (nb_bytes_used == 0 || nb_bytes_used != data_size);

    exit:
    return true;

    error_exit:
    *pb_done = true;
    return false;
  }





#ifdef __cplusplus
}
#endif
