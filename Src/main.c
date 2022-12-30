/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"

// Data and mutex for threads
Data data;
SemaphoreHandle_t mutex;

// Handles for threads
osThreadId defaultTaskHandle;
osThreadId buttonTask01Handle;

// Function prototypes
static void MX_GPIO_Init(void);
static void RTOS_Init(void);
static void debug(int*);
static int  writeData(void);
static int  readData(void);
void 		SystemClock_Config(void);
void 		StartDefaultTask(void const * argument);
void 		buttonTask(void const* argument);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
	// Init device peripherals
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();

	// Init RTOS and start the scheduler
	RTOS_Init();
	osKernelStart();

	// Shouldn't get here
	// RTOS scheduler will take over
	while (1);
}

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
void StartDefaultTask(void const* argument) {
	while(1) {
		xSemaphoreTake(mutex, portMAX_DELAY);
		int r_state = readData();
		if (r_state != GPIO_PIN_SET) {
			HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_Pin, GPIO_PIN_SET);
			osDelay(10);
			HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_Pin, GPIO_PIN_RESET);
		}
		xSemaphoreGive(mutex);
		osDelay(1);
	}
}

/**
* @brief Function implementing the buttonTask01 thread.
* @param argument: Not used
* @retval None
*/
void buttonTask(void const* argument) {
	while (1) {
		xSemaphoreTake(mutex, portMAX_DELAY);
		int state = writeData();
		if (state != GPIO_PIN_SET) {
			for (int i = 0; i < 5; ++i) {
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
				osDelay(100);
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);	// LED is active low
				osDelay(100);
			}
		}
		xSemaphoreGive(mutex);
		osDelay(1);
    }
}

static int readData(void) {
	int state = data.buffer[data.read_buff_idx++];
	if (data.read_buff_idx == 10) {
		data.read_buff_idx = 0;
	}
	return state;
}

/**
* @brief Function to write to the data buffer
* @retval None
*/
static int writeData(void) {
	int state = HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);
	data.buffer[data.write_buff_idx++] = state;
	if (data.write_buff_idx == 10) {
		data.write_buff_idx = 0;
	}
	//debug(&state);
	return state;
}

/**
* @brief Function for debug viewing of variables during runtime
* @retval None
*/
static void debug(int* state) {
	if ((data.write_buff_idx % 2) == 0) {
		data.buffer[data.write_buff_idx] = 0;
		*state = 0;
	}
}

/**
  * @brief Initialize variables for RTOS tasks
  * @retval None
  */
static void RTOS_Init(void) {
	data.write_buff_idx = 0;
	data.read_buff_idx = 0;
	Buff_Init(data.buffer, 1, 10);

	mutex = xSemaphoreCreateMutex();

	osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
	defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

	osThreadDef(buttonTask01, buttonTask, osPriorityNormal, 0, 128);
	buttonTask01Handle = osThreadCreate(osThread(buttonTask01), NULL);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = LED_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = BUZZ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BUZZ_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = BTN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
	__disable_irq();
	while (1);
}
