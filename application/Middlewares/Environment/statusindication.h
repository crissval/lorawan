/*
 * Header file used to indicate application status to the user.
 *
 *  Created on: 3 juil. 2018
 *      Author: Jérôme FUCHET (Jerome.FUCHET@uca.fr)
 */

#ifndef ENVIRONMENT_STATUSINDICATION_H_
#define ENVIRONMENT_STATUSINDICATION_H_

#ifdef __cplusplus
extern "C" {
#endif


  /**
   * The different statuses.
   */
  typedef enum StatusInd
  {
    STATUS_IND_NONE,
    STATUS_IND_ASLEEP,
    STATUS_IND_AWAKE,
    STATUS_IND_CFG_ERROR,
    STATUS_IND_GPS_FIX_TRYING,
    STATUS_IND_GPS_FIX_OK,
    STATUS_IND_GPS_FIX_KO,
    STATUS_IND_RF_SEND_TRYING,
    STATUS_IND_RF_SEND_OK,
    STATUS_IND_RF_SEND_KO
  }
  StatusInd;


  void status_ind_set_status(StatusInd status);


#ifdef __cplusplus
}
#endif
#endif /* ENVIRONMENT_STATUSINDICATION_H_ */
