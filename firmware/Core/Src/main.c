/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tusb.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define IRDA_TRANSFER_SIZE 1
#define UART_TRANSFER_SIZE 1
#define MAX_USB_SINGLE_READ 64
#define IRDA_BUF_SIZE 256
#define UART_BUF_SIZE 256
#define TRANSACT_LED_DECAY_TIME 5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

IRDA_HandleTypeDef hirda1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart2_rx;

PCD_HandleTypeDef hpcd_USB_DRD_FS;

/* USER CODE BEGIN PV */
uint8_t irda_irq_buf[IRDA_TRANSFER_SIZE];
uint8_t uart_irq_buf[UART_TRANSFER_SIZE];

volatile bool usb_mounted = false;

const uint8_t reply[8] = {0xfa, 0x01, 0xa1, 0x97, 0xde, 0xad, 0xbe, 0xef};

uint8_t irda_rx_buf[IRDA_BUF_SIZE];
volatile size_t irda_rx_buf_count = 0;

uint8_t uart_rx_buf[UART_BUF_SIZE];
volatile size_t uart_rx_buf_count = 0;

volatile uint32_t rx_led_off_time;
volatile uint32_t tx_led_off_time;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_IRDA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_FLASH_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * Copied straight from tinyusb
 */
size_t board_get_unique_id(uint8_t id[], size_t max_len) {
  (void) max_len;
  // fixed serial string is 01234567889ABCDEF
  uint32_t* uid32 = (uint32_t*) (uintptr_t)id;
  uid32[0] = 0x67452301u;
  uid32[1] = 0xEFCDAB89u;
  return 8;
}

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

// Invoked when device is mounted
void tud_mount_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
  usb_mounted = true;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  blink_interval_ms = BLINK_NOT_MOUNTED;
  usb_mounted = false;
}


void set_rx_led() {
	// Set RX LED and set off time
	HAL_GPIO_WritePin(RX_LED_GPIO_Port, RX_LED_Pin, GPIO_PIN_SET);
	rx_led_off_time = HAL_GetTick() + TRANSACT_LED_DECAY_TIME;
}


void set_tx_led() {
	HAL_GPIO_WritePin(TX_LED_GPIO_Port, TX_LED_Pin, GPIO_PIN_SET);
	tx_led_off_time = HAL_GetTick() + TRANSACT_LED_DECAY_TIME;
}


void start_irda_receive() {
	HAL_StatusTypeDef res = HAL_IRDA_Receive_DMA(&hirda1, irda_irq_buf, IRDA_TRANSFER_SIZE);

    // Sometimes returns `HAL_BUSY`. Not sure why but it seems fine if we ignore it
	if(res != HAL_OK) {
		Error_Handler();
	}
}


void start_uart_receive() {
	HAL_StatusTypeDef res = HAL_UART_Receive_DMA(&huart2, uart_irq_buf, UART_TRANSFER_SIZE);

    // Sometimes returns `HAL_BUSY`. Not sure why but it seems fine if we ignore it
	if(res != HAL_OK) {
		Error_Handler();
	}
}


void write_to_irda(uint8_t *buf, size_t len) {

	if(HAL_IRDA_Abort(&hirda1) != HAL_OK) {
		Error_Handler();
	}

	// 136 bytes * 10 bits / 115200 baud is 12 ms
	if(HAL_IRDA_Transmit(&hirda1, buf, len, 15) != HAL_OK) {
		Error_Handler();
	}

	start_irda_receive();

}


void write_to_user(uint8_t *buf, size_t len) {
	int itf = 0;

	// TODO: Probably need to check to see if transmit still in progress before starting
	while(HAL_UART_Transmit_DMA(&huart2, buf, len) != HAL_OK) {
		//Error_Handler();
	}

	if(usb_mounted) {
		//for(size_t i = 0; i < len; i++) {
	//		uint8_t c = buf[i];// ^ 0xaa;
	//		tud_cdc_n_write_char(itf, c);
	//	}
		tud_cdc_n_write(itf, buf, len);
		tud_cdc_n_write_flush(itf);
	}


}


void usb_to_irda_task(void) {
	int itf = 0;
	while (tud_cdc_n_available(itf)) {
		uint8_t buf[MAX_USB_SINGLE_READ];
		uint32_t count = tud_cdc_n_read(itf, buf, MAX_USB_SINGLE_READ);

		set_rx_led();

		write_to_irda(buf, count);
	}
}


void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool rts) {
  (void)rts;
}


void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if (HAL_GetTick() - start_ms < blink_interval_ms) {
    return; // not enough time
  }
  start_ms += blink_interval_ms;

  HAL_GPIO_WritePin(POWER_LED_GPIO_Port, POWER_LED_Pin, led_state);
  led_state = 1 - led_state; // toggle
}



void HAL_IRDA_RxCpltCallback(IRDA_HandleTypeDef *hirda) {
	if(hirda->Instance != hirda1.Instance) return;

	// Get data out of there ASAP to not overwrite it
	for(size_t i = 0; i < IRDA_TRANSFER_SIZE; i++) {
		irda_rx_buf[irda_rx_buf_count++] = irda_irq_buf[i];
	}

	// Main thread can check length of received data with `rx_buf_count`

	// Blink TX LED
	set_tx_led();

	// Restart receiver
	start_irda_receive();

}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if(huart->Instance != huart2.Instance) return;

	for(size_t i = 0; i < UART_TRANSFER_SIZE; i++) {
		uart_rx_buf[uart_rx_buf_count++] = uart_irq_buf[i];
	}

	set_rx_led();

	start_uart_receive();
}


void irda_to_user_task() {
	// Do we have data to act on?
	if(irda_rx_buf_count <= 0) return;

	/*
	 * We need to be careful here.
	 * We very likely will have data incoming as we print it out
	 * Can't do CPU-less since tinyusb uses CPU.
	 *
	 */
	size_t start = 0;
	do {
		size_t end_snapshot = irda_rx_buf_count;
		size_t print_count = end_snapshot - start;
		//if(print_count > 7 || irda_rx_buf[start] == (0xfc ^ 0xaa)) {
			write_to_user(&irda_rx_buf[start], print_count);
			start = end_snapshot;
		//}

	} while(start < irda_rx_buf_count);
	irda_rx_buf_count = 0;

}


void uart_to_irda_task() {
	if(uart_rx_buf_count <= 0) return;

	size_t start = 0;
	do {
		size_t end_snapshot = uart_rx_buf_count;
		size_t print_count = end_snapshot - start;
		write_to_irda(&uart_rx_buf[start], print_count);
		start = end_snapshot;
	} while(start < uart_rx_buf_count);
	uart_rx_buf_count = 0;

}


void check_leds_task() {
	uint32_t now = HAL_GetTick();
	if(now > tx_led_off_time) {
		HAL_GPIO_WritePin(TX_LED_GPIO_Port, TX_LED_Pin, GPIO_PIN_RESET);
	}

	if(now > rx_led_off_time) {
		HAL_GPIO_WritePin(RX_LED_GPIO_Port, RX_LED_Pin, GPIO_PIN_RESET);
	}
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_IRDA_Init();
  MX_USART2_UART_Init();
  MX_USB_PCD_Init();
  MX_FLASH_Init();
  /* USER CODE BEGIN 2 */
  tud_init(BOARD_TUD_RHPORT);

  // Enable IrDA transceiver
  HAL_GPIO_WritePin(IRDA_SD_GPIO_Port, IRDA_SD_Pin, GPIO_PIN_RESET);

  // Kickstart IR receiver
  start_irda_receive();

  // Kickstart UART receiver
  start_uart_receive();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	tud_task();
	usb_to_irda_task();
	uart_to_irda_task();
	irda_to_user_task();
	check_leds_task();
	led_blinking_task();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV4;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief FLASH Initialization Function
  * @param None
  * @retval None
  */
static void MX_FLASH_Init(void)
{

  /* USER CODE BEGIN FLASH_Init 0 */

  /* USER CODE END FLASH_Init 0 */

  FLASH_OBProgramInitTypeDef pOBInit = {0};

  /* USER CODE BEGIN FLASH_Init 1 */

  /* USER CODE END FLASH_Init 1 */
  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    Error_Handler();
  }

  /* Option Bytes settings */

  if (HAL_FLASH_OB_Unlock() != HAL_OK)
  {
    Error_Handler();
  }
  pOBInit.OptionType = OPTIONBYTE_USER|OPTIONBYTE_SEC;
  pOBInit.USERType = OB_USER_NBOOT_SEL|OB_USER_NBOOT1
                              |OB_USER_NRST_MODE;
  pOBInit.USERConfig = OB_BOOT0_FROM_PIN|OB_BOOT1_SYSTEM
                              |OB_NBOOT0_RESET|OB_RESET_MODE_INPUT_ONLY;
  pOBInit.BootEntryPoint = OB_BOOT_LOCK_DISABLE;
  pOBInit.SecSize = 0;
  if (HAL_FLASHEx_OBProgram(&pOBInit) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FLASH_OB_Lock() != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FLASH_Lock() != HAL_OK)
  {
    Error_Handler();
  }

  /* Launch Option Bytes Loading */
  /*HAL_FLASH_OB_Launch(); */

  /* USER CODE BEGIN FLASH_Init 2 */
  //HAL_FLASH_OB_Launch();
  /* USER CODE END FLASH_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_IRDA_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  hirda1.Instance = USART1;
  hirda1.Init.BaudRate = 115200;
  hirda1.Init.WordLength = IRDA_WORDLENGTH_8B;
  hirda1.Init.Parity = IRDA_PARITY_NONE;
  hirda1.Init.Mode = IRDA_MODE_TX_RX;
  hirda1.Init.Prescaler = 1;
  hirda1.Init.PowerMode = IRDA_POWERMODE_NORMAL;
  hirda1.Init.ClockPrescaler = IRDA_PRESCALER_DIV1;
  if (HAL_IRDA_Init(&hirda1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_DRD_FS.Instance = USB_DRD_FS;
  hpcd_USB_DRD_FS.Init.dev_endpoints = 8;
  hpcd_USB_DRD_FS.Init.speed = USBD_FS_SPEED;
  hpcd_USB_DRD_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_DRD_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.battery_charging_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.bulk_doublebuffer_enable = DISABLE;
  hpcd_USB_DRD_FS.Init.iso_singlebuffer_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_DRD_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, RX_LED_Pin|POWER_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, IRDA_SD_Pin|USER_LED_Pin|TX_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : RX_LED_Pin POWER_LED_Pin */
  GPIO_InitStruct.Pin = RX_LED_Pin|POWER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : NRST_Pin */
  GPIO_InitStruct.Pin = NRST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(NRST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : IRDA_SD_Pin USER_LED_Pin TX_LED_Pin */
  GPIO_InitStruct.Pin = IRDA_SD_Pin|USER_LED_Pin|TX_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
