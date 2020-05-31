/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* ConnecSenS Environment Class ***************************/
/**********************************************************/
#pragma once

/* Dependencies *******************************************/
// Standard C/C++ Libraries
#include <map>
#include <string>
#include <time.h>
#include <cmath>

/* Middlewares ********************************************/
#include "buffer.hpp"
#include "binhex.hpp"
#include "json.hpp"
#include "periodic.hpp"
#include "gps.hpp"
#include "network.hpp"
#include "cnssrf.h"
#include "cnssrf-dt_config.h"
#include "sensor.hpp"
#include "sdcard.h"
#include "timer.h"



#define CONNECSENS_NB_SENSOR_MAX         CNSSRF_DATA_CHANNEL_15
#define CONNECSENS_BUFFER_SIZE		 4096
#define CONNECSENS_NAME_MAX_SIZE         32
#define CONNECSENS_UNIQUE_ID_SIZE        (CNSSRF_DT_CONFIG_VALUE_LEN_MAX + 1)
#define CONNECSENS_EXPERIMENT_NAME_SIZE  (CNSSRF_DT_CONFIG_VALUE_LEN_MAX + 1)

#ifndef CONNECSENS_CNSSRF_DATA_FRAME_BUFFER_SIZE
#define CONNECSENS_CNSSRF_DATA_FRAME_BUFFER_SIZE  250
#endif
#ifndef CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY
#define CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY  120
#endif

#ifndef OUTPUT_DATA_CSV_BUFFER_SIZE
#define OUTPUT_DATA_CSV_BUFFER_SIZE 512
#endif

#ifndef CNSSRF_DATALOG_FILE_NAME_LEN_MAX
#define CNSSRF_DATALOG_FILE_NAME_LEN_MAX  100
#endif

#define CONNECSENS_NODE_TYPE  "ConnecSenS"


extern uint8_t DevEui[8];
extern uint8_t AppEui[8];
extern uint8_t AppKey[16];


/* Class **************************************************/
class ConnecSenS : ClassNetwork::EventClient
{
public:
  /**
   * Lists the different working modes
   */
  typedef enum WorkingMode
  {
    WMODE_NORMAL,         ///< Normal working mode
    WMODE_CAMPAIGN_RANGE  ///< Working mode for a range measurement campaign
  }
  WorkingMode;


public:
  ConnecSenS();

  const char *uniqueId();
  bool        setUniqueId(const char *psUId);

  void        initialisation();
  bool        process(bool irq = true, bool periodic = true);
  void        EndOfExecution();
  void        Sleep(ts2000_t tsWakeup);
  WorkingMode workingMode() { return this->_workingMode; }
  void        startCampaignRange();
  void        lookAtResetSource();

  bool saveSensorData(Sensor *pvSensor, ts2000_t ts2000);

  static ConnecSenS *instance();
  static void        yield();

  // Overwrites network events client default function.
  void sendStateChanged(ClassNetwork           *pvInterface,
			ClassNetwork::SendState state);

  static uint32_t getPeriodSec(const JsonObject &obj,
			       uint32_t          defaultValue = 0,
			       bool             *pbOk         = NULL,
			       const char       *psBase       = "period");


private:
  static ConnecSenS *   _pvInstance; ///< The unique instance of this class.
  char 			DeviceName[     CONNECSENS_NAME_MAX_SIZE];         ///< The node's name
  char                  _uniqueId[      CONNECSENS_UNIQUE_ID_SIZE];        ///< The node's unique identifier.
  char                  _experimentName[CONNECSENS_EXPERIMENT_NAME_SIZE];  ///< The name of the experiment the node is part of.
  bool                  _addGeoPosToEachRFFrame;                           ///< Add geographical position, if available, to each RF frame?
  bool                  _sensitiveToExternalInterruption;
  bool                  _sensitiveToInternalInterruption;
  bool                  _output_data_to_csv;              ///< Output sensor's data to CSV file.
  bool                  _output_data_csv_file_is_opened;  ///< Indicate if the CSV data output file is opened or not.
  File                  _output_data_csv_file;            ///< The CSV data output file.
  char                  _output_data_csv_buffer[OUTPUT_DATA_CSV_BUFFER_SIZE];  ///< The CSV data output working buffer.
  WorkingMode           _workingMode;

  buffer<uint8_t> myBuffer;								// Buffer utilis� par plusieurs applications

  /* Gestion des Sensors ********************************/
  Sensor *		_sensors[CONNECSENS_NB_SENSOR_MAX];		// Référencement des capteurs utilisés dans la configuration courante
  uint8_t 		NumberOfSensors;						// Nombre de capteurs int�gr�s � la configuration courante
  void 			ActionOnSensor(Sensor *pvSensor);			// R�alise une s�rie d'actions sur un objet Sensor quelconque

  /* Gestion de l'interface r�seau **********************/
  ClassNetwork *	Network;								// Interface connectivit� sans fil
  void 			ActionOnNetwork();						// R�alise la transmission des donn�es suivant le moyen de communication configur�

  /* Gestion des p�riph�riques embarqu�s ****************/
  ClassGPS		GPS;									// Module GPS pour resynchro RTC

  /* Gestion de l'alimentation **************************/
  void                  setPowerForSleepMode();

  /* Gestion du temps ***********************************/
  Timer                 _wakeupTimer;                ///< Timer used to wake up from programmed sleep.
  uint32_t              _lastWakeupTs2000;           ///< Timestamp computed last time for the next wakeup.
  bool 			_enableGPS;								// [on/off] utilisation du GPS pour synchroniser la RTC
  bool                  _firstGPSFix;                ///< Indicate if the current GPS fix is the first one or not.
  bool                  _gpsFixDoneOutsidePeriodic;  ///< Indicate if a GPS fix has been done outside the periodic task
  bool                  _timeSyncMethodChanged;      ///< Indicate if the time synchronisation method changed.
  Datetime 		currentTimestamp;						// date de l'ex�cution courante
  Datetime		manualTimestamp;						// date venant du fichier config.json lors d'un r�glage manuel
  uint32_t              _lastPeriodicProcess;        ///< Last time the periodic tasks have been processed.
  ts2000_t              _batteryLastReadTs2000;      ///< Timestamp of the last node's battery voltage reading.
  bool                  setRTCUsingGPS(bool blocking);
  bool                  setRTCUsingManualTime();
  void 			RefreshTimestamp();						// R�cup�re la date RTC pour rafraichir les propri�t�s CurrentTimestamp et TimestampString
  void                  updatePeriodicTaskNextTime();

  /* Gestion des diff�rents fichiers d'environnement ****/
  bool 			loadConfig();							// Chargement de la configuration pr�sente dans le fichier config.json

  void  readBatteryVoltage();

  void  InterruptHandler();
  void  PeriodicHandler();

  void  openCNSSRFDatalog();
  void  closeCNSSRFDatalog();

  bool  startNewCNSSRFDataFrame(ts2000_t timestamp = 0);
  bool  appendSensorDataToCurrentCNSSRFDataFrame(Sensor *pvSensor);
  bool  saveCurrentCNSSRFDataFrame();

  void detectConfigurationChange();
  bool writeCNSSRFFramesWithConfig();
  bool appendConfigValueToCurrentCNSSRFFrame(CNSSRFDataChannel   channel,
					     CNSSRFConfigParamId paramId,
					     const char         *psValue);

  // CSV data output related functions
  bool openDataOutputCSVFile();
  void closeDataOutputCSVFile();
  bool startDataOutputCSVLine(const Datetime *pvDt = NULL);


  ClassPeriodic         _sendConfigTimer;  ///< Timer used to periodically send configuration over RF

  CNSSRFDataFrame       _cnssrfDataFrame;  ///< The data frame object used for ConnecSenS RF payloads.
  static uint8_t        _cnssrfBuffer[    CONNECSENS_CNSSRF_DATA_FRAME_BUFFER_SIZE];
  static CNSSRFMetaData _cnssrfMetaBuffer[CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY];

  CNSSRFDataFrame       _cnssrfSendDataFrame;  ///< The ConnecSenS RF payload frame where the data to send are written to.
  static uint8_t        _cnssrfSendBuffer[    CONNECSENS_CNSSRF_DATA_FRAME_BUFFER_SIZE];
  static CNSSRFMetaData _cnssrfSendMetaBuffer[CONNECSENS_CNSSRF_DATA_FRAME_META_DATA_BUFFER_CAPACITY];

  char _cnssrfDatalogFileName[CNSSRF_DATALOG_FILE_NAME_LEN_MAX];  ///< Store the ConnecSenS RF datalog filename.

  /**
   * List the directories that are required on the SD card.
   * The list is NULL terminated.
   */
  static const char *_requiredSDDirectoryStructure[];

  /**
   * List directory and files that were used by previous versions of the software
   * and that are not longer usefull.
   * The list is NULL terminated.
   */
  static const char *_oldDirAndFilesToRemove[];
};
