/*
 * Implements SD card usage with a FAT file system.
 *
 *  Created on: 18 mai 2018
 *      Author: jfuchet
 */
#include <string.h>
#include "sdcard.h"
#include "ff_gen_drv.h"
#include "gpio.h"


#ifdef __cplusplus
extern "C" {
#endif


#if defined SDCARD_USE_DETECT && SDCARD_USE_DETECT != 0
#if !defined SDCARD_DETECT_GPIO || !defined SDCARD_DETECT_PULL || !defined SDCARD_DETECT_ACTIVE_LEVEL
#error "SDCARD_DETECT_GPIO, SDCARD_DETECT_PULL and SDCARD_DETECT_ACTIVE_LEVEL must be defined."
#endif
#else
#ifdef SDCARD_USE_DETECT
#undef SDCARD_USE_DETECT
#endif
#endif


#define SDCARD_VOLUME_NAME  ""  // Default volume

#define sdcard_periph_clock_freq_hz()  48000000u


  static SD_HandleTypeDef _sdcard_periph;  ///< The SD/MMC peripheral object

  static FATFS _sdcard_fatfs; ///< The FAT file system object for the SD card logical drive
  static bool  _sdcard_ios_have_been_initialised   = false;
  static bool  _sdcard_periph_has_been_initialised = false;
  static bool  _sdcard_fat_has_been_initialised    = false;

  static bool  sdcard_init_ios(     void);
  static void  sdcard_deinit_ios(   void);
  static bool  sdcard_init_periph(  void);
  static void  sdcard_deinit_periph(void);
  static bool  sdcard_init_fat(     void);
  static void  sdcard_deinit_fat(   void);

  static bool  sdcard_card_is_detected(void);

  static BYTE  sdcard_file_access_to_ff_file_access(uint8_t file_access);


  // The functions used to communicate between the SD layer and the FAT layer
  DSTATUS sdcard_diskio_init(BYTE lun);
  DSTATUS sdcard_diskio_status(BYTE lun);
  DRESULT sdcard_diskio_read(BYTE lun, BYTE *pu8_buffer, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT sdcard_diskio_write(BYTE lun, const BYTE *pu8_buffer, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
  DRESULT sdcard_diskio_ioctl(BYTE lun, BYTE cmd, void *pv_buff);
#endif

  /**
   * The object used by the FAT layer to speed with the SD layer.
   */
  static const Diskio_drvTypeDef _sdcard_diskio =
  {
      sdcard_diskio_init,
      sdcard_diskio_status,
      sdcard_diskio_read,
#if _USE_WRITE == 1
      sdcard_diskio_write,
#endif
#if _USE_IOCTL == 1
      sdcard_diskio_ioctl
#endif
  };


#ifdef SDCARD_USE_DMA

#define SDCARD_DMA_RX_ENABLE_CLOCK()  PASTER3(__HAL_RCC_DMA, SDCARD_DMA_RX_NUM ,_CLK_ENABLE)()
#define SDCARD_DMA_TX_ENABLE_CLOCK()  PASTER3(__HAL_RCC_DMA, SDCARD_DMA_TX_NUM ,_CLK_ENABLE)()
#define SDCARD_DMA_RX_CHANNEL      PASTER4(DMA, SDCARD_DMA_RX_NUM, _Channel, SDCARD_DMA_RX_CHANNEL_NUM)
#define SDCARD_DMA_RX_CHANNEL_IRQN PASTER5(DMA, SDCARD_DMA_RX_NUM, _Channel, SDCARD_DMA_RX_CHANNEL_NUM, _IRQn)
#define SDCARD_DMA_RX_IRQ_HANDLER  PASTER5(DMA, SDCARD_DMA_RX_NUM, _Channel, SDCARD_DMA_RX_CHANNEL_NUM, _IRQHandler)
#define SDCARD_DMA_TX_CHANNEL      PASTER4(DMA, SDCARD_DMA_TX_NUM, _Channel, SDCARD_DMA_TX_CHANNEL_NUM)
#define SDCARD_DMA_TX_CHANNEL_IRQN PASTER5(DMA, SDCARD_DMA_TX_NUM, _Channel, SDCARD_DMA_TX_CHANNEL_NUM, _IRQn)
#define SDCARD_DMA_TX_IRQ_HANDLER  PASTER5(DMA, SDCARD_DMA_TX_NUM, _Channel, SDCARD_DMA_TX_CHANNEL_NUM, _IRQHandler)


  /**
   * Defines the DMA transfer statuses.
   */
  typedef enum SDCardDMATransfertStatus
  {
    SDCARD_DMA_TRANSFER_STATUS_DONE,
    SDCARD_DMA_TRANSFER_STATUS_IN_PROGRESS,
    SDCARD_DMA_TRANSFER_STATUS_ERROR,
    SDCARD_DMA_TRANSFER_STATUS_ABORTED
  }

  SDCardDMATransfertStatus;

  static bool sdcard_dma_config_rx(SD_HandleTypeDef *pv_hsd);
  static bool sdcard_dma_config_tx(SD_HandleTypeDef *pv_hsd);
  static bool sdcard_wait_for_end_of_dma_transfer(uint32_t timeout_ms);

  static SDCardDMATransfertStatus _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_DONE;

#endif



  /**
   * Initialise the SD card.
   */
  bool sdcard_init()
  {
    return sdcard_init_periph() && sdcard_init_ios() && sdcard_init_fat();
  }

  /**
   * De-initialise the SD card
   */
  void sdcard_deinit()
  {
    sdcard_deinit_fat();
    sdcard_deinit_ios();
    sdcard_deinit_periph();
  }

  /**
   * Switch to sleep mode.
   */
  void sdcard_sleep()
  {
    // Gate SD peripheral clock.
    __HAL_RCC_SDMMC1_CLK_DISABLE();

    sdcard_deinit_ios();
  }

  /**
   * Wakeup from sleep mode.
   */
  bool sdcard_wakeup()
  {
    // Enable peripheral clock
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    return sdcard_init_ios();
  }


  //============================== SD card related stuff  ================================


  /**
   * Initialise the GPIOs used by SD card.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdcard_init_ios(void)
  {
    GPIO_InitTypeDef init;

    if(_sdcard_ios_have_been_initialised) { goto exit; }

    // Enable GPIO ports clocks
    gpio_use_gpios_with_ids(SDCARD_D0_GPIO,  SDCARD_D1_GPIO, SDCARD_D2_GPIO, SDCARD_D3_GPIO,
			    SDCARD_CLK_GPIO, SDCARD_CMD_GPIO);

    // Configure communication GPIOs
    init.Mode      = GPIO_MODE_AF_PP;
    init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    init.Alternate = SDCARD_GPIOS_ALTERNATE;
#ifdef SDCARD_HAS_EXTERNAL_PULLUPS
    init.Pull      = GPIO_NOPULL;
#else
    init.Pull      = GPIO_PULLUP;
#endif
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_D0_GPIO,  &init.Pin), &init);
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_D1_GPIO,  &init.Pin), &init);
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_D2_GPIO,  &init.Pin), &init);
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_D3_GPIO,  &init.Pin), &init);
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_CLK_GPIO, &init.Pin), &init);
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_CMD_GPIO, &init.Pin), &init);

#ifdef SDCARD_USE_DETECT
    // Set up detect pin
    gpio_use_gpio_with_id(SDCARD_DETECT_GPIO);
    init.Mode      = GPIO_MODE_INPUT;
    init.Speed     = GPIO_SPEED_LOW;
    init.Pull      = SDCARD_DETECT_PULL;
    HAL_GPIO_Init(gpio_hal_port_and_pin_from_id(SDCARD_DETECT_GPIO, &init.Pin));
#endif

    _sdcard_ios_have_been_initialised = true;

    exit:
    return true;
  }

  /**
   * De-initialises the GPIOs used by SD card. Put them in analog Mode.
   */
  static void sdcard_deinit_ios(void)
  {
    // Configure GPIOs in analog mode
    gpio_free_gpios_with_ids(SDCARD_D0_GPIO,  SDCARD_D1_GPIO, SDCARD_D2_GPIO, SDCARD_D3_GPIO,
			     SDCARD_CLK_GPIO, SDCARD_CMD_GPIO);

#ifdef SDCARD_USE_DETECT
    gpio_free_gpio_with_id(SDCARD_DETECT_GPIO);
#endif

    _sdcard_ios_have_been_initialised = false;
  }

  /**
   * Initialises the MCU SD card peripheral
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdcard_init_periph(void)
  {
    if(_sdcard_periph_has_been_initialised) { goto exit; }

    // Check that the card is present
    if(!sdcard_card_is_detected()) { goto error_exit; }

    // Enable peripheral clock
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    // Set up NVIC for SDMMC1 interruptions
    HAL_NVIC_SetPriority(SDMMC1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(  SDMMC1_IRQn);

    // Set up peripheral for base operation, so that we can communicate with any king of SD card.
    // The minimum communication requirements are 1 bit bus and 400kHz bus clock frequency.
    _sdcard_periph.Instance                 = SDMMC1;
    _sdcard_periph.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
    _sdcard_periph.Init.ClockBypass         = SDMMC_CLOCK_BYPASS_DISABLE;
    _sdcard_periph.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    _sdcard_periph.Init.BusWide             = SDMMC_BUS_WIDE_1B;
    _sdcard_periph.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    _sdcard_periph.Init.ClockDiv            = sdcard_periph_clock_freq_hz() / 400000u;
    if(HAL_SD_Init(&_sdcard_periph) != HAL_OK) { goto error_exit; }

    // Switch SD operation for more performance (4bits bus)
    // Switch bus clock speed to 6 MHz. Using higher frequencies do not improve file transfer speed.
     _sdcard_periph.Init.ClockDiv       = sdcard_periph_clock_freq_hz() / 6000000u;
     _sdcard_periph.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_ENABLE; // for reducing power consumption
    if(HAL_SD_ConfigWideBusOperation(&_sdcard_periph, SDMMC_BUS_WIDE_4B) != HAL_OK) { goto error_exit; }

    // The DMA channel will be initialised when needed.

    _sdcard_periph_has_been_initialised = true;

    exit:
    return true;

    error_exit:
    return false;
  }

  /**
   * De-initialises the MCU SD card peripheral
   */
  static void sdcard_deinit_periph(void)
  {
#ifdef SDCARD_USE_DMA
    DMA_HandleTypeDef dma;
#endif

    // Disable interruptions
    HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    // Maybe do 'detect' interruption too.

    // Free DMA if we need to.
#ifdef SDCARD_USE_DMA
      HAL_NVIC_DisableIRQ(SDCARD_DMA_RX_CHANNEL_IRQN);
#if SDCARD_DMA_TX_CHANNEL_IRQN != SDCARD_DMA_RX_CHANNEL_IRQN
      HAL_NVIC_DisableIRQ(SDCARD_DMA_TX_CHANNEL_IRQN);
#endif

      dma.Instance = SDCARD_DMA_RX_CHANNEL;
      HAL_DMA_DeInit(&dma);
#if SDCARD_DMA_TX_NUM != SDCARD_DMA_RX_NUM || SDCARD_DMA_TX_CHANNEL_NUM != SDCARD_DMA_RX_CHANNEL_NUM
      dma.Instance = SDCARD_DMA_TX_CHANNEL;
      HAL_DMA_DeInit(&dma);
#endif
#endif // SDCARD_USE_DMA

    // Disable SD/MMC clock
    __HAL_RCC_SDMMC1_CLK_DISABLE();

    _sdcard_periph_has_been_initialised = false;
  }

  /**
   * Indicate if the card is detetced or not.
   *
   * @return true  if the card is present.
   * @return false otherwise.
   */
  static bool sdcard_card_is_detected(void)
  {
#ifdef SDCARD_USE_DETECT
    uint32_t pin;

    return HAL_GPIO_ReadPin(gpio_hal_port_and_pin_from_id(SDCARD_DETECT_GPIO, &pin), pin) ==
	SDCARD_DETECT_ACTIVE_LEVEL;
#else
    return true;
#endif
  }


  /**
   * Irq handler for SD card interruptions
   */
  void SDMMC1_IRQHandler(void)
  {
    HAL_SD_IRQHandler(&_sdcard_periph);
  }

#ifdef SDCARD_USE_DMA
  /**
   * Configure DMA for rxtransfer.
   *
   * @param[in] pv_hsd the SD card object to transfer data from/to. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdcard_dma_config_rx(SD_HandleTypeDef *pv_hsd)
  {
    static DMA_HandleTypeDef hdma_rx;
    bool      res;

    hdma_rx.Instance                 = SDCARD_DMA_RX_CHANNEL;
    hdma_rx.Init.Request             = DMA_REQUEST_7;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_rx.Init.Priority            = DMA_PRIORITY_VERY_HIGH;

    // Associate the DMA handle
    __HAL_LINKDMA(pv_hsd, hdmarx, hdma_rx);
    pv_hsd->hdmatx = NULL;

    // Stop any ongoing transfer and reset state
    HAL_DMA_Abort(&hdma_rx);

    // Deinitialise channel for new transfer
    HAL_DMA_DeInit(&hdma_rx);

    // Configure channel
    if((res = (HAL_DMA_Init(&hdma_rx) == HAL_OK)))
    {
      // Set up interruptions
      HAL_NVIC_SetPriority(SDCARD_DMA_RX_CHANNEL_IRQN, 6, 0);
      HAL_NVIC_EnableIRQ(  SDCARD_DMA_RX_CHANNEL_IRQN);
    }

    return res;
  }

  /**
   * Configure DMA for rxtransfer.
   *
   * @param[in] pv_hsd the SD card object to transfer data from/to. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdcard_dma_config_tx(SD_HandleTypeDef *pv_hsd)
  {
    static DMA_HandleTypeDef hdma_tx;
    bool      res;

    hdma_tx.Instance                 = SDCARD_DMA_TX_CHANNEL;
    hdma_tx.Init.Request             = DMA_REQUEST_7;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    hdma_tx.Init.Priority            = DMA_PRIORITY_VERY_HIGH;

    // Associate the DMA handle
    __HAL_LINKDMA(pv_hsd, hdmatx, hdma_tx);
    pv_hsd->hdmarx = NULL;

    // Stop any ongoing transfer and reset state
    HAL_DMA_Abort(&hdma_tx);

    // Deinitialise channel for new transfer
    HAL_DMA_DeInit(&hdma_tx);

    // Configure channel
    if((res = (HAL_DMA_Init(&hdma_tx) == HAL_OK)))
    {
      // Set up interruptions
      HAL_NVIC_SetPriority(SDCARD_DMA_TX_CHANNEL_IRQN, 6, 0);
      HAL_NVIC_EnableIRQ(  SDCARD_DMA_TX_CHANNEL_IRQN);
    }

    return res;
  }


  /**
   * Wait for the end of the current DMA transfer (read or write).
   *
   * @param[in] timeout_ms the maximum time, in milliseconds, to wait for the end of the transfer.
   *
   * @return true  if the transfer has ended in time.
   * @return false otherwise.
   */
  static bool sdcard_wait_for_end_of_dma_transfer(uint32_t timeout_ms)
  {
    uint32_t ts_ref = board_ms_now();

    while(_sdcard_dma_transfer_status == SDCARD_DMA_TRANSFER_STATUS_IN_PROGRESS)
    {
      if(board_is_timeout(ts_ref, timeout_ms)) { return false; }
    }

    return true;
  }

  /**
   * Called when the current DMA Rx transfer has completed.
   *
   * Overwrite a week function definition from HAL.
   *
   * @param[in] hsd the handle to the DMA object.
   */
  void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
  {
    UNUSED(hsd);
    _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_DONE;
  }

  /**
   * Called when the current DMA tx transfer has completed.
   *
   * Overwrite a week function definition from HAL.
   *
   * @param[in] hsd the handle to the DMA object.
   */
  void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
  {
    UNUSED(hsd);
    _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_DONE;
  }

  /**
   * Called when the current DMA transfer failed because of an error.
   *
   * Overwrite a week function definition from HAL.
   *
   * @param[in] hsd the handle to the DMA object.
   */
  void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
  {
    UNUSED(hsd);
    _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_ERROR;
  }

  /**
   * Called when the current DMA transfer has been aborted.
   *
   * Overwrite a week function definition from HAL.
   *
   * @param[in] hsd the handle to the DMA object.
   */
  void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
  {
    UNUSED(hsd);
    _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_ABORTED;
  }


  /**
   * Irq handler for SD card DMA channel
   */
#if SDCARD_DMA_RX_IRQ_HANDLER == SDCARD_DMA_TX_IRQ_HANDLER
  void SDCARD_DMA_RX_IRQ_HANDLER(void)
  {
    switch(_sdcard_periph.Context)
    {
      case SD_CONTEXT_DMA | SD_CONTEXT_READ_SINGLE_BLOCK:
      case SD_CONTEXT_DMA | SD_CONTEXT_READ_MULTIPLE_BLOCK:
      HAL_DMA_IRQHandler(_sdcard_periph.hdmarx);
      break;

      case SD_CONTEXT_DMA | SD_CONTEXT_WRITE_SINGLE_BLOCK:
      case SD_CONTEXT_DMA | SD_CONTEXT_WRITE_MULTIPLE_BLOCK:
      HAL_DMA_IRQHandler(_sdcard_periph.hdmatx);
      break;

      default:
	break;
    }
  }
#else  // SDCARD_DMA_RX_IRQ_HANDLER == SDCARD_DMA_TX_IRQ_HANDLER
  void SDCARD_DMA_RX_IRQ_HANDLER(void)
  {
    switch(_sdcard_periph.Context)
    {
      case SD_CONTEXT_DMA | SD_CONTEXT_READ_SINGLE_BLOCK:
      case SD_CONTEXT_DMA | SD_CONTEXT_READ_MULTIPLE_BLOCK:
      HAL_DMA_IRQHandler(_sdcard_periph.hdmarx);
      break;

      default:
	break;
    }
  }
  void SDCARD_DMA_TX_IRQ_HANDLER(void)
  {
    switch(_sdcard_periph.Context)
    {
      case SD_CONTEXT_DMA | SD_CONTEXT_WRITE_SINGLE_BLOCK:
      case SD_CONTEXT_DMA | SD_CONTEXT_WRITE_MULTIPLE_BLOCK:
      HAL_DMA_IRQHandler(_sdcard_periph.hdmatx);
      break;

      default:
	break;
    }
  }
#endif  // SDCARD_DMA_RX_IRQ_HANDLER == SDCARD_DMA_TX_IRQ_HANDLER
#endif  // SDCARD_USE_DMA

  //============================= Diskio SD/FAT communication interface stuff =======

  /**
   * Initialisea a drive
   *
   * @param[in] lun unused.
   *
   * @return operation status.
   */
  DSTATUS sdcard_diskio_init(BYTE lun)
  {
    UNUSED(lun);

    return sdcard_init() ? 0 : STA_NOINIT;
  }

  /**
   * Return card status.
   *
   * @param[in] lun unused.
   *
   * @return card status.
   */
  DSTATUS sdcard_diskio_status(BYTE lun)
  {
    UNUSED(lun);

    return (HAL_SD_GetCardState(&_sdcard_periph) == HAL_SD_CARD_TRANSFER) ? 0 : STA_NOINIT;
  }

  /**
   * Read data from the SD card.
   *
   * @param[in]  lun        not used.
   * @param[out] pu8_buffer the buffer where the data are written to. MUST be NOT NULL.
   *                        MUST be big enough to receive all data.
   * @param[in]  sector     sector address (LBA).
   * @param[in]  count      number of sectors to read [1..128]
   *
   * @return RES_OK    on success.
   * @return RES_ERROR otherwise.
   */
  DRESULT sdcard_diskio_read(BYTE lun, BYTE *pu8_buffer, DWORD sector, UINT count)
  {
    uint32_t ts_ref;
    uint32_t timeout_ms = count * 1000;
    UNUSED(lun);

    // Read
    ts_ref = board_ms_now();
#ifdef SDCARD_USE_DMA
    // Check that the DMA is free
    if(_sdcard_dma_transfer_status == SDCARD_DMA_TRANSFER_STATUS_IN_PROGRESS) { goto error_exit; }

    // Set up DMA channel for read operation.
    if(!sdcard_dma_config_rx(&_sdcard_periph)) { goto error_exit; }

    // Start DMA transfer
    _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_IN_PROGRESS;
    if(HAL_SD_ReadBlocks_DMA(&_sdcard_periph,
			     (uint8_t *)pu8_buffer,
			     (uint32_t) sector,
			     count) != HAL_OK)
    {
      _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_DONE;
      goto error_exit;
    }

    // Wait for end of DMA transfer
    if(!sdcard_wait_for_end_of_dma_transfer(timeout_ms)) { goto error_exit; }
#else  // SDCARD_USE_DMA
    if(HAL_SD_ReadBlocks(&_sdcard_periph,
			 (uint8_t *)pu8_buffer,
			 (uint32_t) sector,
			 count,
			 timeout_ms) != HAL_OK) { goto error_exit; }

#endif  // SDCARD_USE_DMA

    // Wait for end of operation
    while(HAL_SD_GetCardState(&_sdcard_periph) != HAL_SD_CARD_TRANSFER)
    {
      if(board_is_timeout(ts_ref, timeout_ms)) { goto error_exit; }
    }
    return RES_OK;

    error_exit:
    return RES_ERROR;
  }

#if _USE_WRITE == 1
  /**
   * Write data to the SD card
   *
   * @param[in] lun      unused.
   * @param[in] pu8_data the data to write. MUST be NOT NULL.
   * @param[in] sector   sector address (LBA).
   * @param[in] count    the number of sectors to write [1..128].
   *g
   * @return RES_OK    on success.
   * @return RES_ERROR otherwise.
   */
  DRESULT sdcard_diskio_write(BYTE lun, const BYTE *pu8_data, DWORD sector, UINT count)
  {
    uint32_t ts_ref;
    uint32_t timeout_ms = count * 1000;
    UNUSED(lun);

    // Write
    ts_ref = board_ms_now();
#ifdef SDCARD_USE_DMA
    // Check that the DMA is free
    if(_sdcard_dma_transfer_status == SDCARD_DMA_TRANSFER_STATUS_IN_PROGRESS) { goto error_exit; }

    // Set up DMA channel for read operation.
    if(!sdcard_dma_config_tx(&_sdcard_periph)) { goto error_exit; }

    // Start DMA transfer
    _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_IN_PROGRESS;
    if(HAL_SD_WriteBlocks_DMA(&_sdcard_periph,
			      (uint8_t *)pu8_data,
			      (uint32_t) sector,
			      count) != HAL_OK)
    {
      _sdcard_dma_transfer_status = SDCARD_DMA_TRANSFER_STATUS_DONE;
      goto error_exit;
    }

    // Wait for end of DMA transfer
    if(!sdcard_wait_for_end_of_dma_transfer(timeout_ms)) { goto error_exit; }
#else  // SDCARD_USE_DMA
    if(HAL_SD_WriteBlocks(&_sdcard_periph,
			  (uint8_t *)pu8_data,
			  (uint32_t) sector,
			  count,
			  timeout_ms) != HAL_OK) { goto error_exit; }
#endif // SDCARD_USE_DMA

    // Wait for end of operation
    while(HAL_SD_GetCardState(&_sdcard_periph) != HAL_SD_CARD_TRANSFER)
    {
      if(board_is_timeout(ts_ref, timeout_ms)) { goto error_exit; }
    }
    return RES_OK;

    error_exit:
    return RES_ERROR;
  }
#endif

#if _USE_IOCTL == 1
  /**
   * I/O control operations.
   *
   * @param[in]     lun     unused.
   * @param[in]     cmd     the command (or control code).
   * @param[in,out] pv_buff the input/output buffer.
   *
   * @return RES_OK     on success.
   * @return RES_PARERR if the command is unknown.
   * @return RES_NOTRDY if the SD card is not ready.
   * @return RES_ERROR  for other errors.
   */
  DRESULT sdcard_diskio_ioctl(BYTE lun, BYTE cmd, void *pv_buff)
  {
    HAL_SD_CardInfoTypeDef infos;
    DRESULT                res = RES_OK;

    // Check if the card is ready
    if(HAL_SD_GetCardState(&_sdcard_periph) != HAL_SD_CARD_TRANSFER)
    {
      res = RES_NOTRDY;
      goto exit;
    }

    // Process command:
    HAL_SD_GetCardInfo(&_sdcard_periph, &infos);
    switch(cmd)
    {
      case CTRL_SYNC:        // Make sure that no pending write process
	break;  // Do nothing

      case GET_SECTOR_COUNT: // Get number of sectors on the disk (DWORD)
	*(DWORD *)pv_buff = infos.LogBlockNbr;
	break;

      case GET_SECTOR_SIZE:  // Get R/W sector size (WORD)
      case GET_BLOCK_SIZE:   // Get erase block size in unit of sector (DWORD)
	*(DWORD *)pv_buff = infos.LogBlockSize;
	break;

      default:
	res = RES_PARERR;
	break;
    }

    exit:
    return res;
  }
#endif



  //============================= FAT related stuff =================================

  /**
   * Initialises the FAT file system software layer.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  static bool sdcard_init_fat(void)
  {
    if(_sdcard_fat_has_been_initialised) { goto exit; }

    // Link the drivers
    if(FATFS_LinkDriver((Diskio_drvTypeDef *)&_sdcard_diskio, SDCARD_VOLUME_NAME) != 0)
    {
      // Failed
      goto error_exit;
    }

    // Mount volume
    if(f_mount(&_sdcard_fatfs, SDCARD_VOLUME_NAME, 0) != FR_OK)
    {
      // Failed to mount default volume.
      goto error_exit;
    }

    _sdcard_fat_has_been_initialised = true;

    exit:
    return true;

    error_exit:
    return false;
  }

  /**
   * De-initialises the FAT file system software layer.
   */
  static void sdcard_deinit_fat(void)
  {
    if(_sdcard_fat_has_been_initialised)
    {
      // Unmount volume
      f_mount(NULL, SDCARD_VOLUME_NAME, 0);

      // Unregister drivers
      FATFS_UnLinkDriver(SDCARD_VOLUME_NAME);
    }
  }





  /**
   * Indicate if a file or a directory exists.
   *
   * @param[in] ps_filename the name of the file or of the directory to check for.
   *                        MUST be NOT NULL and NOT EMPTY.
   *
   * @return true  if the file or the directory exists.
   * @return false otherwise.
   */
  bool sdcard_exists(const char *ps_filename)
  {
    return f_stat(ps_filename, NULL) == FR_OK;
  }

  /**
   * Create a directory.
   *
   * @param[in] ps_dirname the directory's name.
   *
   * @return true  if the directory has been created.
   * @return false otherwise.
   */
  bool sdcard_mkdir(const char *ps_dirname)
  {
    return f_mkdir(ps_dirname) == FR_OK;
  }

  /**
   * Check if a directory exists, and if not then create it.
   *
   * @param[in] ps_dirname the directory's name.
   *
   * @return true  if the directory exists or if it has been created.
   * @return false if failed to create directory.
   */
  bool sdcard_check_and_mkdir(const char *ps_dirname)
  {
    return sdcard_exists(ps_dirname) || sdcard_mkdir(ps_dirname);
  }

  /**
   * Removes a file or a directory
   *
   * @param[in] ps_filename the name of the file or of the directory to remove.
   *                        MUST be NOT NULL and NOT EMPTY.
   *
   * @return true  on success.
   * @return false otherwise.
   */
    bool sdcard_remove(const char *ps_filename)
    {
      FRESULT res = f_unlink(ps_filename);

      return res == FR_OK || res == FR_NO_FILE || res == FR_NO_PATH;
    }

    /**
     * Remove files and directories matching a given pattern.
     *
     * @param[in] ps_path    the directory where are the files and directories to remove.
     *                       MUST be NOT NULL.
     * @param[in] ps_pattern the pattern used to match the files' and directories' names.
     *                       MUST be NOT NULL.
     *
     * @return true  on success.
     * @return false otherwise.
     */
    bool sdcard_remove_using_pattern(const char *ps_path, const char *ps_pattern)
    {
      Dir      dir;
      FileInfo info;
      char     ps_filename[200];
      uint16_t len, size;
      bool     res = true;

      if(f_findfirst(    &dir, &info, ps_path, ps_pattern) != FR_OK ||
	  (len = strlcpy(ps_filename, ps_path, sizeof(ps_filename))) >= sizeof(ps_filename) - 1)
      {    res = false; goto exit; }

      ps_filename[len++] = '/';
      size     = sizeof(ps_filename) - len;
      while(info.fname[0])
      {
	if(strlcpy( &ps_filename[len], info.fname, size) >= size ||
	    f_unlink(ps_filename)  != FR_OK) { res = false;            }
        if(f_findnext(&dir, &info) != FR_OK) { res = false; goto exit; }
      }

      exit:
      return res;
    }

    /**
     * Rename a file or a directory
     *
     * @param[in] ps_old the current name. MUST be NOT NULL and NOT empty.
     * @param[in] ps_new the new name. MUST be NOT NULL and NOT empty.
     *
     * @return true  on success.
     * @return false otherwise.
     */
    bool sdcard_rename(const char *ps_old, const char *ps_new)
    {
      return f_rename(ps_old, ps_new) == FR_OK;
    }

    /**
     * Create an empty file.
     *
     * @param[in] ps_filename the filename. MUST be NOT NULL and NOT empty.
     * @param[in] force       force creation, even if a file with the given name already exists?
     *
     * @return true  if the file has been created.
     * @return false if there already was a file with the given name and force was set to false.
     * @return false if the operation failed.
     */
    bool sdcard_fcreateEmpty(const char *ps_filename, bool force)
    {
      File file;
      bool res;

      res = (f_open(&file, ps_filename, force ? FA_CREATE_ALWAYS : FA_CREATE_NEW) == FR_OK);
      f_close(&file);

      return res;
    }

    /**
     * Check if a file exist, and if not create an empty file.
     *
     * @param[in] ps_filename the filename. MUST be NOT NULL and NOT empty.
     * @param[in] force       force creation, even if a file with the given name already exists?
     *
     * @return true  if the file exists.
     * @return true  if the file has been created.
     * @return false if there already was a file with the given name and force was set to false.
     * @return false if the operation failed.
     */
    bool sdcard_fexistOrCreateEmpty(const char *ps_filename, bool force)
    {
      return sdcard_exists(ps_filename) || sdcard_fcreateEmpty(ps_filename, force);
    }


  /**
   * Converts a file access bit mask to a FF file access bit mask.
   *
   * @parapm[in] file_access the bit mask to convert.
   *
   * @return the converted bit mask.
   */
  static BYTE sdcard_file_access_to_ff_file_access(uint8_t file_access)
  {
    BYTE res;

    if(     file_access & FILE_APPEND)         { res = FA_OPEN_APPEND   | FA_WRITE; }
    else if(file_access & FILE_TRUNCATE)       { res = FA_CREATE_ALWAYS | FA_WRITE; }
    else if(file_access & FILE_OPEN_OR_CREATE) { res = FA_OPEN_ALWAYS;              }
    else                                       { res = FA_OPEN_EXISTING;            }

    if(file_access & FILE_READ)  { res |= FA_READ;  }
    if(file_access & FILE_WRITE) { res |= FA_WRITE; }

    return res;
  }

  /**
   * Open a file.
   *
   * For the mode you have to choose between FILE_OPEN, FILE_OPEN_OR_CREATE,
   * FILE_TRUNCATE or FILE_APPEND to open the file; you cannot use several of them.
   *
   * @param[in,out] pv_file     the file descriptor to use. MUST be NOT NULL.
   * @param[in]     ps_filename the file's name. MUST be NOT NULL and NOT EMPTY.
   * @param[in]     mode        the file opening mode, a ORed combination of FileActionFlag values.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_fopen(File *pv_file, const char *ps_filename, FileOpenMode mode)
  {
    return f_open(pv_file, ps_filename, sdcard_file_access_to_ff_file_access(mode)) == FR_OK;
  }

  /**
   * Close a file.
   *
   * @param[in] pv_file the file descriptor.
   */
  void sdcard_fclose(File *pv_file)
  {
    f_close(pv_file);
  }

  /**
   * Read data from a file from the current file read/write position.
   *
   * This function expects to be able to read all the bytes requested.
   *
   * @post the file read/write position has advanced of the number of data read from the file.
   *
   * @param[in,out] pv_file    the file descriptor to use. MUST be NOT NULL.
   * @param[out]    pu8_dest   where the data read will be written to. MUST be NOT NULL.
   * @param[in]     nb_to_read the number of data to read.
   *                           The destination buffer MUST be big enough to receive this number of data.
   *
   * @return true  on success (all data requested have been read).
   * @return false otherwise.
   */
  bool sdcard_fread(File *pv_file, uint8_t *pu8_dest, uint32_t nb_to_read)
  {
    UINT nb_read;

    return f_read(pv_file, pu8_dest, nb_to_read, &nb_read) == FR_OK && nb_read == nb_to_read;
  }

  /**
   * Read data from a file from the current file read/write position.
   *
   * This function accepts that the file can provide less bytes than what's requested.
   *
   * @post the file read/write position has advanced of the number of data read from the file.
   *
   * @param[in,out] pv_file    the file descriptor to use. MUST be NOT NULL.
   * @param[out]    pu8_dest   where the data read will be written to. MUST be NOT NULL.
   * @param[in]     nb_to_read the number of data to read.
   *                           The destination buffer MUST be big enough to receive this number of data.
   * @param[out]    pb_ok      will be set to true if everything went right, set to false in case of error.
   *                           Can be NULL if you are not interested in this information.
   *
   * @return the number of bytes read from the file. It can be 0 if there was nothing to read.
   * @return 0 in case of error.
   *           It is possible to see if 0 is the result of an error or not using pb_ok.
   */
  uint32_t sdcard_fread_at_most(File    *pv_file,
				uint8_t *pu8_dest,
				uint32_t nb_to_read,
				bool    *pb_ok)
  {
    bool ok;
    UINT nb_read;

    if(!pb_ok) { pb_ok = &ok; }

    *pb_ok = (f_read(pv_file, pu8_dest, nb_to_read, &nb_read) == FR_OK);
    return nb_read;
  }

  /**
   * Read data until it reaches a end of line or the end of file. The result is a string.
   *
   * @param[in]  pv_file   the file descriptor to use. MUST be NOT NULL.
   * @param[out] ps_buffer where the data read from the line are written to as a string.
   *                       MUST be NOT NULL.
   * @param[in]  size      the output buffer size. MUST be > 0.
   * @param[in]  strip_eol strip end of line characters from the string?
   * @param[out] pb_ok     where the error status is written to. Can be NULL.
   *                       Set to true in case of success.
   *                       Set to false if the buffer is too small to receive the whole line.
   *                       Set to false in case of IO error.
   *
   * @return true  if data have been written to the output buffer.
   * @return false if we have reached the end of the file.
   * @return false in case of error.
   */
  extern bool sdcard_read_line(File *pv_file, char *ps_buffer, uint32_t size, bool strip_eol, bool *pb_ok)
  {
    UINT     nb_read;
    char     c;
    bool     ok;
    uint32_t len = 0;

    if(!pb_ok) { pb_ok = &ok; }
    *pb_ok = true;

    while(1)
    {
      if(f_read(pv_file, &c, 1, &nb_read) != FR_OK) { goto error_exit; }
      if(nb_read != 1) break;  // Reached end of file
      if(strip_eol)
      {
	if(c == '\r')  continue;
	if(c == '\n')  break;
      }
      *ps_buffer++ = c;
      if(  c == '\n')  break;

      if(++len == size) { goto error_exit; }  // Not enough space in the buffer
    }
    *ps_buffer = '\0';

    return len != 0;

    error_exit:
    *pb_ok = false;
    return false;
  }

  /**
   * Write a data buffer to a file.
   *
   * Write to the file's current read/write position.
   *
   * @param[in,out] pv_file  the file descriptor to use. MUST be NOT NULL.
   * @param[in]     pu8_data the data to write. MUST be NOT NULL.
   * @param[in]     size     the number of data to write.
   *
   * @return true if all the data have been written.
   * @return false otherwise.
   */
  bool sdcard_fwrite(File *pv_file, const uint8_t *pu8_data, uint32_t size)
  {
    UINT nb_written;

    return f_write(pv_file, pu8_data, size, &nb_written) == FR_OK && nb_written == size;
  }

  /**
   * Write a string to a file.
   *
   * @param[in,out] pv_file the file descriptor to use. MUST be NOT NULL.
   * @param[in]     ps      the string to write to the file. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_fwrite_string(File *pv_file, const char *ps)
  {
    return sdcard_fwrite(pv_file, (const uint8_t *)ps, strlen(ps));
  }

  /**
   * Write a byte to a file.
   *
   * @param[in,out] pv_file the file descriptor to use. MUST be NOT NULL.
   * @param[in]     b       the byte to write.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_fwrite_byte(File *pv_file, uint8_t b)
  {
    UINT nb_written;

    return f_write(pv_file, &b, 1, &nb_written) == FR_OK && nb_written == 1;
  }

  /**
   * Flushes the cache.
   *
   * @param[in] pv_file the file descriptor to use. MUST be NOT NULL.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_fsync(File *pv_file)
  {
    return f_sync(pv_file) == FR_OK;
  }

  /**
   * Return a file's size.
   *
   * @param[in,out] pv_file the file descriptor to use. MUST be NOT NULL.
   */
  uint32_t sdcard_fsize(File *pv_file)
  {
    return f_size(pv_file);
  }

  /**
   * Truncate a file to a given size.
   *
   * @param[in] pv_file the file object. MUST be opened in write mode.
   * @param[in] size    the size, in bytes, to truncate the file size to.
   * @param[in] expand  if size is greater than the current file size do we expand
   *                    the current file size (true), or do we keep the current size (false)?
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_ftruncateTo(File *pv_file, uint32_t size, bool expand)
  {
    if(f_size(pv_file) == size || (f_size(pv_file) < size && !expand)) { return true; }

    return f_lseek(pv_file, size) == FR_OK && f_truncate(pv_file) == FR_OK;
  }


  /**
   * Get a file's current read/write position.
   *
   * @param[in,out] pv_file the file descriptor to use. MUST be NOT NULL.
   *
   * @return the position
   */
  uint32_t sdcard_fpos(File *pv_file) { return f_tell(pv_file); }

  /**
   * Perform an absolute read/write position seek.
   *
   * If the seek position is beyond the current file size then the file is expanded.
   * The expanded section is filled with UNKNOWN data.
   *
   * @param[in,out] pv_file the file descriptor to use. MUST be NOT NULL.
   * @param[in]     pos     the position.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_fseek_abs(File *pv_file, uint32_t pos)
  {
    return f_lseek(pv_file, pos) == FR_OK;
  }

  /**
   * Perform a relative read/write position seek, relative to the current position.
   *
   * If the seek position is beyond the current file size then the file is expanded.
   * The expanded section is filled with UNKNOWN data.
   *
   * @param[in,out] pv_file the file descriptor to use. MUST be NOT NULL.
   * @param[in]     step    the seek step. Can be negative.
   *
   * @return true  on success.
   * @return false otherwise.
   */
  bool sdcard_fseek_rel(File *pv_file, int32_t step)
  {
    uint32_t current_pos = sdcard_fpos(pv_file);

    if(step < 0 && current_pos < (uint32_t)(-step)) { return false; }

    return sdcard_fseek_abs(pv_file, current_pos + step);
  }


  /**
   * Find the first file, or directory, matching a given pattern
   *
   * @param[out] pv_dir      The working directory object. MUST be NOT NULL.
   * @param[out] pv_info     The working file info object. MUST be NOT NULL.
   * @param[in]  ps_dir_path the path of the directory where the search is performed. MUST be NOT NULL.
   *                         Can be empty for the root directory.
   * @param[in]  ps_pattern  the pattern used the filter the names.
   *                         This value also is used by the next ffindNext calls so it must persist
   *                         between these calls.
   *
   * @return the name of the first file or directory matching the pattern.
   * @return NULL if no file or directory matching the pattern has been found.
   * @return NULL in case of error.
   */
  const char *sdcard_ffind_first(Dir *pv_dir, FileInfo *pv_info, const char *ps_dir_path, const char *ps_pattern)
  {
    return (f_findfirst(pv_dir, pv_info, ps_dir_path, ps_pattern) == FR_OK && pv_info->fname[0]) ?
	pv_info->fname : NULL;
  }

  /**
   * Return the next file or directory matching the pattern defined by the last ffindFirst function.
   *
   * @pre sdcard_ffind_first() MUST have been called once before this function can be used.
   *
   * @param[in]  pv_dir  The working directory object. MUST be NOT NULL.
   *                     Has been initialised by the call to function sdcard_ffind_first().
   * @param[out] pv_info The working file info object. MUST be NOT NULL.
   *
   * @return NULL if we've reached the end of the matching list.
   * @return NULL in case of error.
   */
  const char *sdcard_ffind_next(Dir *pv_dir, FileInfo *pv_info)
  {
    return (f_findnext(pv_dir, pv_info) == FR_OK && pv_info->fname[0]) ? pv_info->fname : NULL;
  }

  /**
   * Go through the files and directories matching a pattern a apply a filter function to each of them.
   *
   * @param[in]     ps_dir_path    the path of the directory where the search is performed. MUST be NOT NULL.
   *                               Can be empty for the root directory.
   * @param[in]     ps_pattern     the pattern used the filter the names.
   * @param[in]     pf_filter_func the filter function to use. MUST be NOT NULL.
   *                               ps_filename is the name of the matched entry to filter. Cannot be NULL or empty.
   *                               pv_ctx      is the working context for the filter function.
   * @param[in,out] pv_ctx         is the context object to pass to the filter function at each call.
   *
   * @return true  on success, even if the filter function has never been called.
   * @return false otherwise (in case of error).
   */
  bool sdcard_ffilter(const char *ps_dir_path,
		      const char *ps_pattern,
		      void      (*pf_filter_func)(const char *ps_filename, void *pv_ctx),
		      void       *pv_ctx)
  {
    Dir      dir;
    FileInfo info;

    if(f_findfirst(&dir, &info, ps_dir_path, ps_pattern) != FR_OK) { return false; }

    while(info.fname[0])
    {
      pf_filter_func(info.fname, pv_ctx);
      if(f_findnext(&dir, &info) != FR_OK) { return false; }
    }

    return true;
  }


#ifdef __cplusplus
}
#endif
