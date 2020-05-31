/*
 * ConnecSens RF data frame implementation
 *
 *  Created on: 15 mai 2018
 *      Author: jfuchet
 */
#include "cnssrf-dataframe.h"


#ifdef __cplusplus
extern "C" {
#endif


#define cnssrf_frame_meta_buffer_is_full(pv_frame)  \
  ((pv_frame)->meta_count >= (pv_frame)->meta_buffer_capacity)

#define cnssrf_frame_meta_add_data_bytes_count(pv_frame, count)  \
  *(pv_frame)->pv_meta_current_data_channel =                                    \
  (*(pv_frame)->pv_meta_current_data_channel &  ~CNSSRF_META_DATA_SIZE_MASK) | ( \
  cnssrf_meta_data_size(*(pv_frame)->pv_meta_current_data_channel) + (count));   \
  *(pv_frame)->pv_meta_current_data_type    =                                    \
  (*(pv_frame)->pv_meta_current_data_type    &  ~CNSSRF_META_DATA_SIZE_MASK) | ( \
  cnssrf_meta_data_size(*(pv_frame)->pv_meta_current_data_type)    + (count))


  static bool cnssrf_data_frame_get_data_type_id_from_bytes(CNSSRFDataType *pv_dt,
							    uint8_t        *pu8_data,
							    uint16_t        data_size,
							    uint8_t        *nb_bytes_used);


  /**
   * Initialises a data frame object.
   *
   * @param[in] pv_frame       the data frame object to initialise. MUST be NOT NULL.
   * @param[in] pu8_buffer     the buffer used by the frame. MUST be NOT NULL.
   * @param[in] size           the buffer's size.
   * @param[in] pv_meta_buffer where the meta data are written to.
   *                           Can be NULL if we are not interested in the meta data.
   * @param[in] meta_len       the number of meta data the meta data buffer can receive.
   *                           Can be 0 if we are not interested in the meta data.
   */
  void cnssrf_data_frame_init(CNSSRFDataFrame *pv_frame,
			      uint8_t         *pu8_buffer,
			      uint16_t         size,
			      CNSSRFMetaData  *pv_meta_buffer,
			      uint16_t         meta_len)
  {
    pv_frame->pu8_buffer  = pu8_buffer;
    pv_frame->buffer_size = size;

    if(!pv_meta_buffer || !meta_len)
    {
      pv_frame->pv_meta_buffer       = NULL;
      pv_frame->meta_buffer_capacity = 0;
    }
    else
    {
      pv_frame->pv_meta_buffer       = pv_meta_buffer;
      pv_frame->meta_buffer_capacity = meta_len;
    }

    cnssrf_data_frame_clear(pv_frame);
  }

  /**
   * Clear a data frame object.
   *
   * @param[in] pv_frame the frame object to clear. MUST be not NULL.
   */
  void cnssrf_data_frame_clear(CNSSRFDataFrame *pv_frame)
  {
    pv_frame->data_count                    = 0;
    pv_frame->current_data_channel          = CNSSRF_DATA_CHANNEL_UNDEFINED;
    pv_frame->current_nb_data               = 0;
    pv_frame->pu8_current_data_channel_byte = pv_frame->pu8_buffer;
    pv_frame->current_data_type             = CNSSRF_DATA_TYPE_UNDEFINED;

    pv_frame->meta_count                    = 0;
    pv_frame->pv_meta_current_data_channel  = NULL;
    pv_frame->pv_meta_current_data_type     = NULL;
  }


  /**
   * Indicate if an identifier is a valid Data Channel or not.
   *
   * @param[in] id the identifier.
   *
   * @return true  if it is a valid Data Channel number.
   * @return false otherwise.
   */
  bool cnssrf_data_frame_is_valid_data_channel_id(uint32_t id)
  {
    return id >= CNSSRF_DATA_CHANNEL_0 && id <= CNSSRF_DATA_CHANNEL_15;
  }

  /**
   * Set the current Data Channel in a data frame object.
   *
   * @param[in,out] pv_frame the data frame object. MUST be NOT NULL and MUST have been initialised.
   * @param[in]     channel  the current Data Channel to set.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_data_frame_set_current_data_channel(CNSSRFDataFrame *pv_frame, CNSSRFDataChannel channel)
  {
    bool clear_global_ctx = false;

    pv_frame->current_data_type = CNSSRF_DATA_TYPE_UNDEFINED;

    if(channel == pv_frame->current_data_channel &&
	pv_frame->current_nb_data < CNSSRF_NB_DATA_MAX_PER_DATA_CHANNEL) { goto ok_exit; }

    if(pv_frame->data_count == 0)
    {
      // Before writing the data channel data, write the format/version byte
      if(cnssrf_data_frame_is_full(pv_frame)) { goto error_exit; }
      pv_frame->pu8_buffer[pv_frame->data_count++] = CNSSRF_DATA_FRAME_FORMAT_VERSION_BYTE;

      // Write meta data if we have to
      if(pv_frame->pv_meta_buffer)
      {
        if(cnssrf_frame_meta_buffer_is_full(pv_frame)) { goto error_exit; }
        pv_frame->pv_meta_buffer[pv_frame->meta_count++] = CNSSRF_META_DATA_TYPE_FORMAT_ID + 1; // + 1 to count the size of the RF frame format byte
      }

      // Also indicate that the global context has to be cleared
      clear_global_ctx = true;
    }

    if(cnssrf_data_frame_is_full(pv_frame)) { goto error_exit; }
    pv_frame ->current_data_channel          = channel;
    pv_frame ->current_nb_data               = 0;
    pv_frame ->pu8_current_data_channel_byte = &pv_frame->pu8_buffer[pv_frame->data_count++];
    *pv_frame->pu8_current_data_channel_byte =
	clear_global_ctx ? CNSSRF_DATA_CHANNEL_CLEAR_GLOBAL_CTX : 0 |
	    (channel & 0x0F) << CNSSRF_NB_DATA_FIELD_WIDTH_NB_BITS;

    // Write meta data if we have to
    if(pv_frame->pv_meta_buffer)
    {
      if(cnssrf_frame_meta_buffer_is_full(pv_frame)) { goto error_exit; }
      pv_frame ->pv_meta_current_data_channel = &pv_frame->pv_meta_buffer[pv_frame->meta_count++];
      *pv_frame->pv_meta_current_data_channel = CNSSRF_META_DATA_TYPE_DATA_CHANNEL + 1;  // + 1 to count the size of the Data Channel byte
    }

    ok_exit:
    return true;

    error_exit:
    return false;
  }

  /**
   * Set the current Data Type in a data frame object.
   *
   * @param[in,out] pv_frame the data frame object. MUST be NOT NULL and MUST have been initialised.
   * @param[in]     type     the current Data Type to set.
   * @param[in]     flags    the flags associated with the Data Type and it's data.
   *
   * @return true  on success.
   * @return false otherwise (not enough space in the buffer).
   */
  bool cnssrf_data_frame_set_current_data_type(CNSSRFDataFrame    *pv_frame,
					       CNSSRFDataType      type,
					       CNSSRFMetaDataFlags flags)
  {
    uint8_t byte;

    // Check if we can add a new Data Type in the current channel; have we reached the limit?
    if(pv_frame->current_nb_data >= CNSSRF_NB_DATA_MAX_PER_DATA_CHANNEL)
    {
      // Then create a new data channel header with the same data channel number
      if(!cnssrf_data_frame_set_current_data_channel(pv_frame, pv_frame->current_data_channel))
      { goto error_exit; }
    }

    // Update the number of data
    pv_frame ->current_nb_data++;
    *pv_frame->pu8_current_data_channel_byte &= ~CNSSRF_NB_DATA_BIT_MASK;
    *pv_frame->pu8_current_data_channel_byte |=  pv_frame->current_nb_data;

    // Set the current data type
    pv_frame->current_data_type = type;

    // Write meta data if we have to
    if(pv_frame->pv_meta_buffer)
    {
      if(cnssrf_frame_meta_buffer_is_full(pv_frame)) { goto error_exit; }
      pv_frame ->pv_meta_current_data_type = &pv_frame->pv_meta_buffer[pv_frame->meta_count++];
      *pv_frame->pv_meta_current_data_type = CNSSRF_META_DATA_TYPE_DATA_TYPE | flags;
    }

    // If the type fits in a single byte then use a simplified procedure
    if(type <= CNSSRF_DATA_TYPE_PART_VALUE_MAX)
    {
      return cnssrf_data_frame_add_bytes(pv_frame, (uint8_t *)&type, 1);
    }

    // The type does not fit into a single byte
    for( ; type; type >>= CNSSRF_DATA_TYPE_PART_NB_BITS)
    {
      byte = cnssrf_get_data_type_part_from_byte(type);
      if(type > CNSSRF_DATA_TYPE_PART_VALUE_MAX)
      {
	cnssrf_set_data_type_more_bit_in_byte(byte);
      }
      if(!cnssrf_data_frame_add_bytes(pv_frame, &byte, 1)) { goto error_exit; }
    }

    return true;

    error_exit:
    return false;
  }


  /**
   * Add bytes to a data frame's buffer.
   *
   * @param[in,out] pv_frame the data frame object. MUST be NOT NULL and MUST have been initialised.
   * @param[in]     pu8_data the bytes to add. MUST be NOT NULL.
   * @param[in]     size     the number of bytes in pu8_data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_data_frame_add_bytes(CNSSRFDataFrame *pv_frame, const uint8_t *pu8_data, uint16_t size)
  {
    if(!cnssrf_data_frame_has_enough_space_left_for(pv_frame, size)) { return false; }

    // Update meta data if we have to
    if(pv_frame->pv_meta_buffer)
    {
      if(cnssrf_frame_meta_buffer_is_full(   pv_frame)) { return false; }
      cnssrf_frame_meta_add_data_bytes_count(pv_frame, size);
    }

    for( ; size; size--)
    {
      pv_frame->pu8_buffer[pv_frame->data_count++] = *pu8_data++;
    }

    return true;
  }


  /**
   * Create a data frame containing only the global values from another frame.
   *
   * @param[out] pv_frame_dest the frame where the data are written to. MUST be NOT NULL.
   * @param[in]  pv_frame_src  the frame the data are read from. MUST be NOT NULL.
   *                           This frame MUST have mete data.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool cnssrf_data_frame_copy_global_values(CNSSRFDataFrame *pv_frame_dest,
					    CNSSRFDataFrame *pv_frame_src)
  {
    uint8_t          *pu8_data, *pu8_data_end, nb_used, clear_global;
    CNSSRFMetaData   *pv_meta, *pv_meta_end;
    CNSSRFDataChannel channel;
    CNSSRFDataType    dt;

    if(!pv_frame_src->pv_meta_buffer || !pv_frame_src->meta_count) { goto error_exit; }

    pu8_data      =            pv_frame_src->pu8_buffer;
    pu8_data_end  = pu8_data + pv_frame_src->data_count;
    pv_meta       = pv_frame_src->pv_meta_buffer;
    pv_meta_end   = pv_meta  + pv_frame_src->meta_count;
    channel       = CNSSRF_DATA_CHANNEL_UNDEFINED;
    clear_global  = 0;
    for( ; pu8_data  < pu8_data_end && pv_meta < pv_meta_end; pv_meta++)
    {
      switch(cnssrf_meta_data_type(*pv_meta))
      {
	case CNSSRF_META_DATA_TYPE_DATA_CHANNEL:
	  channel      = cnssrf_get_data_channel_from_byte(        *pu8_data);
	  clear_global = cnssrf_get_data_channel_get_clear_ctx_bit(*pu8_data);
	  pu8_data    += CNSSRF_DATA_CHANNEL_HEADER_SIZE;
	  break;

	case CNSSRF_META_DATA_TYPE_DATA_TYPE:
	  if(cnssrf_meta_data_flags(*pv_meta) & CNSSRF_META_DATA_FLAG_GLOBAL)
	  {
	    if( !cnssrf_data_frame_set_current_data_channel(pv_frame_dest, channel)        ||
		!cnssrf_data_frame_get_data_type_id_from_bytes(&dt,
							       pu8_data,
							       cnssrf_meta_data_size(*pv_meta),
							       &nb_used)                   ||
		!cnssrf_data_frame_set_current_data_type(pv_frame_dest,
							 dt,
							 cnssrf_meta_data_flags(*pv_meta)) ||
		!cnssrf_data_frame_add_bytes(pv_frame_dest,
					     pu8_data                        + nb_used,
					     cnssrf_meta_data_size(*pv_meta) - nb_used))
	    {
	      goto error_exit;
	    }
	    *pv_frame_dest->pu8_current_data_channel_byte |= clear_global;
	    clear_global = 0;
	  }
	  pu8_data += cnssrf_meta_data_size(*pv_meta);

	  // Make sure that we will not concatenate values to the last data type values written.
	  pv_frame_dest->current_data_type = CNSSRF_DATA_TYPE_UNDEFINED;
	  break;

	case CNSSRF_META_DATA_TYPE_FORMAT_ID:
	  // Do nothing
	  pu8_data += cnssrf_meta_data_size(*pv_meta);
	  break;

	default:
	  // This should not happen
	  goto error_exit;
      }
    }

    return true;

    error_exit:
    return false;
  }


  /**
   * Get a Data Type identifier from bytes.
   *
   * @param[out] pv_dt where   the Data Type identifier is written to. MUST be NOT NULL.
   * @param[in]  pu8_data      the bytes to get the identifier from. MUST be NOT NULL.
   * @param[in]  data_size     the number of bytes available to read.
   * @param[out] nb_bytes_used where the number of bytes used to get the Data Type identifier
   *                           is written to.
   *                           Can be NULL if you are not interested in this information.
   *
   * @return true  on success.
   * @return false in case of error.
   */
  static bool cnssrf_data_frame_get_data_type_id_from_bytes(CNSSRFDataType *pv_dt,
							    uint8_t        *pu8_data,
							    uint16_t        data_size,
							    uint8_t        *nb_bytes_used)
  {
    uint8_t count, _nb_used;

    if(!nb_bytes_used) { nb_bytes_used = &_nb_used; }
    *nb_bytes_used = 0;

    for(*pv_dt = 0, count = 0; count < data_size && count < 5; count++, pu8_data++)
    {
      *pv_dt |= (uint32_t)(*pu8_data & CNSSRF_DATA_TYPE_PART_BIT_MASK) << count * CNSSRF_DATA_TYPE_PART_NB_BITS;
      if(!(*pu8_data & CNSSRF_DATA_TYPE_MORE_BIT_MASK))
      {
	// We have our id
	*nb_bytes_used = count + 1;
	return true;
      }
    }

    return false;
  }


  /**
   * Copy the data from one frame to another, starting from a given data index. Can only copy
   * a given number of bytes.
   *
   * @param[out] pv_frame_dest      the frame where the data are written to. MUST be NOT NULL.
   * @param[in]  pv_frame_src       the frame the data are read from. MUST be NOT NULL.
   *                                This frame MUST have meta data.
   * @param[in]  data_index         the index, in the data, to start the copy from.
   *                                It must points to the first byte of a meta data type,
   *                                that is points to the format byte, a Channel id byte or the first
   *                                byte of a Data Type id.
   * @param[in]  size_max           the maximum size the output frame can get.
   *                                If set to 0 then no explicit maximum size, the copy
   *                                is limited by the output frame buffer size.
   * @param[in]  channel_split_min  The minimum amount of data a channel must have to be split.
   *                                If it contains less data than than this value then the
   *                                channel's data cannot be split.
   *                                Use a value of 0 if you don't want to use this constraint.
   * @param[out] pu16_nb_bytes_used indicate the number of bytes used from the source frame.
   *                                Can be NULL if you are not interested in this information.
   *
   * @return true  on success.
   * @return false if the data index does not point to a meta data type's first byte.
   * @return false if the source frame has no meta data.
   */
  bool cnssrf_data_frame_copy_from_data_index_size_max(CNSSRFDataFrame *pv_frame_dest,
  						       CNSSRFDataFrame *pv_frame_src,
  						       uint16_t         data_index,
  						       uint16_t         size_max,
						       uint16_t         channel_split_min,
						       uint16_t        *pu16_nb_bytes_used)
  {
    uint8_t           nb_bytes_used;
    uint16_t          pos, _nb_used;
    CNSSRFMetaData   *pv_meta, *pv_meta_end;
    CNSSRFDataType    dt;

    if(!pu16_nb_bytes_used) { pu16_nb_bytes_used = &_nb_used; }
    *pu16_nb_bytes_used = 0;

    if( cnssrf_data_frame_is_empty(pv_frame_src) ||
	cnssrf_data_frame_size(    pv_frame_dest) >= size_max) { goto exit; }  // Nothing to do

    // Make sure that a new channel byte is written
    pv_frame_dest->current_data_channel = CNSSRF_DATA_CHANNEL_UNDEFINED;
    pv_frame_dest->current_data_type    = CNSSRF_DATA_TYPE_UNDEFINED;

    for(pv_meta     =           pv_frame_src->pv_meta_buffer,
	pv_meta_end = pv_meta + pv_frame_src->meta_count,
	pos         = 0;
	pv_meta     < pv_meta_end && pos < pv_frame_src->data_count;
	pv_meta++)
    {
      switch(cnssrf_meta_data_type(*pv_meta))
      {
	case CNSSRF_META_DATA_TYPE_DATA_CHANNEL:
	  if(pos + cnssrf_meta_data_size(*pv_meta) > data_index)
	  {
	    // Check the number of data bytes in the channel to see if we have to split it and if we can split it
	    if( pos >= data_index &&  // if pos < data_index then the channel already has been split
		cnssrf_data_frame_size(pv_frame_dest) + cnssrf_meta_data_size(*pv_meta) > size_max &&
		cnssrf_meta_data_size(*pv_meta) < channel_split_min)
	    { goto exit; } // Do not split the channel's data.

	    // Write channel byte
	    if(cnssrf_data_frame_size(pv_frame_dest) >= size_max)                  { goto exit;       }
	    if(!cnssrf_data_frame_set_current_data_channel(
		pv_frame_dest,
		cnssrf_get_data_channel_from_byte(pv_frame_src->pu8_buffer[pos]))) { goto error_exit; }
	    *pv_frame_dest->pu8_current_data_channel_byte |=
		cnssrf_get_data_channel_get_clear_ctx_bit(pv_frame_src->pu8_buffer[pos]);
	  }
	  pos += CNSSRF_DATA_CHANNEL_HEADER_SIZE;
	  break;

	case CNSSRF_META_DATA_TYPE_DATA_TYPE:
	  if(pos >= data_index)
	  {
	    if(cnssrf_data_frame_size(pv_frame_dest) + cnssrf_meta_data_size(*pv_meta) > size_max)
	    { goto exit; }
	    // Copy Data Type data
	    if( !cnssrf_data_frame_get_data_type_id_from_bytes(&dt,
							       &pv_frame_src->pu8_buffer[pos],
							       cnssrf_meta_data_size(*pv_meta),
							       &nb_bytes_used)             ||
		!cnssrf_data_frame_set_current_data_type(pv_frame_dest,
							 dt,
							 cnssrf_meta_data_flags(*pv_meta)) ||
		!cnssrf_data_frame_add_bytes(pv_frame_dest,
					     &pv_frame_src->pu8_buffer[pos   + nb_bytes_used],
					     cnssrf_meta_data_size(*pv_meta) - nb_bytes_used))

	    {
	      goto error_exit;
	    }
	  }
	  pos += cnssrf_meta_data_size(*pv_meta);
	  break;

	case CNSSRF_META_DATA_TYPE_FORMAT_ID:
	  // Do nothing
	  pos += cnssrf_meta_data_size(*pv_meta);
	  break;

	default:
	  goto error_exit;  // This should not happen
      }
    }

    exit:
    *pu16_nb_bytes_used = pos;
    return true;

    error_exit:
    return false;
  }


#ifdef __cplusplus
}
#endif


