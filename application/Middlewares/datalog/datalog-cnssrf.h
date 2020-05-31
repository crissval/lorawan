/*
 *  Implementation of a fixed size entry datalog file to store CNSSRF data.
 *
 *  @author Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 *  @date   2018
 */

#ifndef DATALOG_DATALOG_CNSSRF_H_
#define DATALOG_DATALOG_CNSSRF_H_

#include "defs.h"
#include "cnssrf-dataframe.h"
#include "datetime.h"


#ifdef __cplusplus
extern "C" {
#endif



  bool  datalog_cnssrf_init(  const char *ps_filename);
  void  datalog_cnssrf_deinit(void);
#define datalog_cnssrf_open()  datalog_cnssrf_init()
#define datalog_cnssrf_close() datalog_cnssrf_deinit()
  void  datalog_cnssrf_sync();

  bool  datalog_cnssrf_add(const CNSSRFDataFrame *pv_frame);

  bool  datalog_cnssrf_get_frame(CNSSRFDataFrame *pv_frame,
				 uint16_t         size_max,
				 ts2000_t         since,
				 bool            *pb_has_more);
  void  datalog_cnssrf_frame_has_been_sent(void);


#ifdef __cplusplus
}
#endif
#endif /* DATALOG_DATALOG_CNSSRF_H_ */
