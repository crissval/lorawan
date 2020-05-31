/*
 *  Implementation of a fixed size entry datalog file
 *
 *  @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */

#ifndef DATALOG_DATALOGFILE_H_
#define DATALOG_DATALOGFILE_H_

#include "defs.h"
#include "datetime.h"
#include "sdcard.h"


#ifdef __cplusplus
extern "C" {
#endif


#ifndef DATALOG_FILENAME_SIZE_MAX
#define DATALOG_FILENAME_SIZE_MAX  100
#endif


  /**
     * The flags used to set the datalog format.
     */
    typedef enum DataLogFileFormatFlag
    {
      DLOGF_FORMAT_BYTE_ORDER_LITTLE_ENDIAN  = 0x01,
      DLOGF_FORMAT_BYTE_ORDER_BIG_ENDIAN     = 0x02
    }
    DataLogFileFormatFlag;
    typedef uint8_t DataLogFileFormatFlags;

    /**
     * Defines the type that is used to identify the type of data stored in the records.
     * The value only has meaning meaning for the datalog file's user.
     */
    typedef char DataLogFileContentsId[32];


    /**
     * Defines the contents of datalog file header's dynamic part.
     */
    typedef struct DataLogFileHeaderDynamic
    {
      uint32_t nb_records_with_data;   ///< The number of records that contain data.
      uint32_t log_head_seek_pos;      ///< Seek position to the log's head. Points to the record where the next data will be inserted.
      uint32_t log_tail_seek_pos;      ///< Seek position of the log's tale. Points to the record with the oldest data.
    }
    DataLogFileHeaderDynamic;

    /**
     * Defines the datalog file header.
     */
    typedef struct DataLogFileHeader
    {
      DataLogFileFormatFlags format;     ///< File format flags.
      uint8_t                version;    ///< The header's version.
      uint8_t                unused[2];  ///< Padding and for future use.

      uint32_t records_size;  ///< The size of the records, in bytes.
      uint32_t nb_records;    ///< The number of records in the file.

      DataLogFileContentsId contents_type;  ///< The type of contents stored in the records.

      DataLogFileHeaderDynamic dynamic;     ///< The dynamic part of the header
    }
    DataLogFileHeader;

    /**
     * Define a record identifier.
     */
    typedef uint32_t DataLogFileRecordId;


    /**
     * Defines the datalog file object structure.
     */
    typedef struct DataLogFile
    {
      char              ps_filename[DATALOG_FILENAME_SIZE_MAX]; ///< The datalog's file name.
      bool              is_opened;                              ///< Indicate if the datalog file is opened or not.
      bool              created_by_last_open;                   ///< Indicate that the file has been created by the last open
      File              file;                                   ///< The file object.
      uint32_t          file_size;                              ///< The file's size.
      DataLogFileHeader header;                                 ///< The datalog's header.
    }
    DataLogFile;

    /**
     * Defines a record's header (meta data).
     */
    typedef struct DataLogFileRecordHeader
    {
      ts2000_t timestamp;   ///< The record's timestamp.
      uint32_t user_status; ///< The record's status. Whose value has only meaning for the user.
      uint32_t nb_data;     ///< The number of data in the record.
    }
    DataLogFileRecordHeader;

#define datalogfile_record_header_is_valid(pv_header)  \
  ((pv_header)->timestamp && (pv_header)->nb_data)


    extern bool datalogfile_init( DataLogFile *pv_dlf,
				  const char  *ps_filename,
				  uint32_t     records_size,
				  uint32_t     nb_records);
    extern bool datalogfile_open( DataLogFile *pv_dlf, const char *ps_contents_type, bool create);
    extern void datalogfile_close(DataLogFile *pv_dlf);
    extern void datalogfile_sync();

#define datalogfile_filename(             pv_dlf)  ((pv_dlf)->ps_filename)
#define datalogfile_file_has_been_created(pv_dlf)  ((pv_dlf)->created_by_last_open)

    extern DataLogFileRecordId datalogfile_write_record(          DataLogFile                *pv_dlf,
								  uint8_t                    *pu8_data,
								  uint32_t                    size,
								  uint32_t                    status);
    extern bool                datalogfile_is_empty(              DataLogFile                *pv_dlf);
    extern DataLogFileRecordId datalogfile_latest_record(         DataLogFile                *pv_dlf);
    extern DataLogFileRecordId datalogfile_next_record(           DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid);
    extern DataLogFileRecordId datalogfile_previous_record(       DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid);
    extern uint8_t *           datalogfile_record_data(           DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid,
								  uint8_t                    *pu8_data,
								  uint32_t                    buffer_size,
								  DataLogFileRecordHeader    *pv_header);
    extern bool                datalogfile_record_header(         DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid,
								  DataLogFileRecordHeader    *pv_header);
    extern bool                datalogfile_record_status(         DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid,
								  uint32_t                   *pu32_status);
    extern bool                datalogfile_update_record_status(  DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid,
								  uint32_t                    new_status);
    extern bool                datalogfile_update_record_status_add_flags(DataLogFile        *pv_dlf,
									  DataLogFileRecordId rid,
									  uint32_t            flags);
    extern bool                datalogfile_record_status_is(      DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid,
								  uint32_t                    status);
    extern bool                datalogfile_record_status_has_flag(DataLogFile                *pv_dlf,
								  DataLogFileRecordId         rid,
								  uint32_t                    flag);



#ifdef __cplusplus
}
#endif
#endif /* DATALOG_DATALOGFILE_H_ */
