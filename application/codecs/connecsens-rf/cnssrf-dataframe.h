/*
 * Represents a ConnecSenS RF data frame
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#ifndef CONNECSENS_RF_CNSSRF_DATAFRAME_H_
#define CONNECSENS_RF_CNSSRF_DATAFRAME_H_

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CNSSRF_DATA_FRAME_FORMAT_VERSION_BYTE       0xC1
#define CNSSRF_DATA_FRAME_FORMAT_VERSION_BYTE_SIZE  1


  /**
   * Defines the data channels
   */
  typedef enum CNSSRFDataChannel
  {
    CNSSRF_DATA_CHANNEL_0,
    CNSSRF_DATA_CHANNEL_1,
    CNSSRF_DATA_CHANNEL_2,
    CNSSRF_DATA_CHANNEL_3,
    CNSSRF_DATA_CHANNEL_4,
    CNSSRF_DATA_CHANNEL_5,
    CNSSRF_DATA_CHANNEL_6,
    CNSSRF_DATA_CHANNEL_7,
    CNSSRF_DATA_CHANNEL_8,
    CNSSRF_DATA_CHANNEL_9,
    CNSSRF_DATA_CHANNEL_10,
    CNSSRF_DATA_CHANNEL_11,
    CNSSRF_DATA_CHANNEL_12,
    CNSSRF_DATA_CHANNEL_13,
    CNSSRF_DATA_CHANNEL_14,
    CNSSRF_DATA_CHANNEL_15,
    CNSSRF_DATA_CHANNEL_NODE      = CNSSRF_DATA_CHANNEL_0,
    CNSSRF_DATA_CHANNEL_UNDEFINED = 0xFF
  }
  CNSSRFDataChannel;
#define CNSSRF_DATA_CHANNEL_NB_MAX     (CNSSRF_DATA_CHANNEL_15 + 1)

#define CNSSRF_NB_DATA_FIELD_WIDTH_NB_BITS   3
#define CNSSRF_NB_DATA_BIT_MASK              0x07
#define CNSSRF_NB_DATA_MAX_PER_DATA_CHANNEL  7

#define CNSSRF_DATA_CHANNEL_CLEAR_GLOBAL_CTX (0x01 << 7)
#define CNSSRF_DATA_CHANNEL_HEADER_SIZE  1

#define cnssrf_get_data_channel_from_byte(        byte)  ((CNSSRFDataChannel)(((byte) >> 3) & 0x0F))
#define cnssrf_get_data_channel_nb_data_from_byte(byte)  ((uint8_t)          ( (byte)       & 0x07))
#define cnssrf_get_data_channel_get_clear_ctx_bit(byte)  ((byte) & CNSSRF_DATA_CHANNEL_CLEAR_GLOBAL_CTX)


  /**
   * Defines the type for Data Type identifier.
   */
  typedef uint32_t CNSSRFDataType;
#define CNSSRF_DATA_TYPE_UNDEFINED  0xFFFFFFFF

#define CNSSRF_DATA_TYPE_PART_NB_BITS        7
#define CNSSRF_DATA_TYPE_PART_BIT_MASK       0x7F
#define CNSSRF_DATA_TYPE_PART_VALUE_MAX      CNSSRF_DATA_TYPE_PART_BIT_MASK
#define CNSSRF_DATA_TYPE_MORE_BIT_POS        7
#define CNSSRF_DATA_TYPE_MORE_BIT_MASK       (1u << CNSSRF_DATA_TYPE_MORE_BIT_POS)

#define cnssrf_get_data_type_part_from_byte(     byte) ((uint8_t)((byte) & CNSSRF_DATA_TYPE_PART_BIT_MASK))
#define cnssrf_get_data_type_more_from_byte(     byte) ((uint8_t)((byte) & CNSSRF_DATA_TYPE_MORE_BIT_MASK))
#define cnssrf_is_data_type_more_bit_set_in_byte(byte) (cnssrf_get_data_type_more_from_byte(byte) != 0)
#define cnssrf_set_data_type_more_bit_in_byte(   byte) byte |=  CNSSRF_DATA_TYPE_MORE_BIT_MASK
#define cnssrf_clear_data_type_more_bit_in_byte( byte) byte &= ~CNSSRF_DATA_TYPE_MORE_BIT_MASK


  /**
   * Defines the type used to store meta data
   */
  typedef uint16_t CNSSRFMetaData;

  /**
   * Defines the type of a meta data
   */
  typedef enum CNSSRFMetaDataType
  {
    CNSSRF_META_DATA_TYPE_NONE         = 0,        ///< The meta data has no type
    CNSSRF_META_DATA_TYPE_FORMAT_ID    = 1u << 13, ///< The meta data corresponds to the frame format byte.
    CNSSRF_META_DATA_TYPE_DATA_CHANNEL = 2u << 13, ///< The meta data corresponds to a Data Channel.
    CNSSRF_META_DATA_TYPE_DATA_TYPE    = 3u << 13  ///< The meta data corresponds to a Data Type.
  }
  CNSSRFMetaDataType;
#define CNNSRF_META_DATA_TYPE_MASK  0xE000
#define CNSSRF_META_DATA_SIZE_MASK  0x00FF

  /**
   * Defines the flags that can be associated with a meta data
   */
#define CNSSRF_META_DATA_FLAGS_POS   8
#define CNSSRF_META_DATA_FLAGS_MASK  0x0F00
  typedef enum CNSSRFMetaDataFlag
  {
    CNSSRF_META_DATA_FLAG_NONE   = 0,
    CNSSRF_META_DATA_FLAG_GLOBAL = 0x01 << CNSSRF_META_DATA_FLAGS_POS ///< The associated data has a frame global scope
  }
  CNSSRFMetaDataFlag;
  typedef uint16_t CNSSRFMetaDataFlags;

#define cnssrf_meta_data_type( meta)  ((CNSSRFMetaDataType)((meta) & CNNSRF_META_DATA_TYPE_MASK))
#define cnssrf_meta_data_size( meta)                       ((meta) & CNSSRF_META_DATA_SIZE_MASK)
#define cnssrf_meta_data_flags(meta)  ((CNSSRFMetaDataFlag)((meta) & CNSSRF_META_DATA_FLAGS_MASK))


  /**
   * Defines the data frame structure.
   */
  typedef struct CNSSRFDataFrame
  {
    uint8_t *pu8_buffer;       ///< The buffer where the data are.
    uint16_t buffer_size;      ///< The buffer's size.
    uint16_t data_count;       ///< The number of data in the buffer.

    CNSSRFMetaData *pv_meta_buffer;       ///< The buffer where the meta data are written to. Can be NULL.
    uint16_t        meta_buffer_capacity; ///< The number of meta data the meta data buffer can receive. Can be O.
    uint16_t        meta_count;           ///< The number of meta data written to the buffer.

    CNSSRFDataChannel current_data_channel;           ///< The current Data Channel.
    uint8_t           current_nb_data;                ///< The number of data for the current Data Channel.
    uint8_t           *pu8_current_data_channel_byte; ///< Points to the buffer's byte where the current data channel information is
    CNSSRFDataType    current_data_type;              ///< The current Data Type.

    CNSSRFMetaData   *pv_meta_current_data_channel;   ///< Points to the meta data's current data channel first byte.
    CNSSRFMetaData   *pv_meta_current_data_type;      ///< Points to the meta data's current data type first byte.
  }
  CNSSRFDataFrame;


  void  cnssrf_data_frame_init( CNSSRFDataFrame *pv_frame,
				uint8_t         *pu8_buffer,
				uint16_t         size,
				CNSSRFMetaData  *pv_meta_buffer,
				uint16_t         meta_len);
  void  cnssrf_data_frame_clear(CNSSRFDataFrame *pv_frame);

#define cnssrf_data_frame_is_full(                  pv_frame)        ((pv_frame)->data_count >= (pv_frame)->buffer_size)
#define cnssrf_data_frame_has_enough_space_left_for(pv_frame, size)  ((pv_frame)->buffer_size - (pv_frame)->data_count >= (size))
#define cnssrf_data_frame_is_empty(                 pv_frame)        ((pv_frame)->data_count == 0)
#define cnssrf_data_frame_data(                     pv_frame)        ((pv_frame)->pu8_buffer)
#define cnssrf_data_frame_size(                     pv_frame)        ((pv_frame)->data_count)
#define cnssrf_data_frame_space_left(               pv_frame)        ((pv_frame)->buffer_size - (pv_frame)->data_count)
#define cnssrf_data_frame_current_data_channel(     pv_frame)        ((pv_frame)->current_data_channel)

  bool  cnssrf_data_frame_is_valid_data_channel_id(uint32_t id);

  bool  cnssrf_data_frame_set_current_data_channel(CNSSRFDataFrame     *pv_frame,
						   CNSSRFDataChannel   channel);
  bool  cnssrf_data_frame_set_current_data_type(   CNSSRFDataFrame     *pv_frame,
						   CNSSRFDataType      type,
						   CNSSRFMetaDataFlags flags);

  bool  cnssrf_data_frame_add_bytes(CNSSRFDataFrame *pv_frame, const uint8_t *pu8_data, uint16_t size);


#define cnssrf_data_frame_meta_data(      pv_frame) ((pv_frame)->pv_meta_buffer)
#define cnssrf_data_frame_meta_data_count(pv_frame) ((pv_frame)->meta_count)
#define cnssrf_data_frame_meta_data_size( pv_frame) ((pv_frame)->meta_count * sizeof(CNSSRFMetaData))

  bool cnssrf_data_frame_copy_global_values(CNSSRFDataFrame *pv_frame_dest,
					    CNSSRFDataFrame *pv_frame_src);

  bool cnssrf_data_frame_copy_from_data_index_size_max(CNSSRFDataFrame *pv_frame_dest,
						       CNSSRFDataFrame *pv_frame_src,
						       uint16_t         data_index,
						       uint16_t         size_max,
						       uint16_t         channel_split_min,
						       uint16_t        *pu16_nb_bytes_used);

#ifdef __cplusplus
}
#endif
#endif /* CONNECSENS_RF_CNSSRF_DATAFRAME_H_ */
