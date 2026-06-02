/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "can_handler.h"
#include "usb_handler.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/* USER CODE BEGIN RTOS_QUEUES */
q_raw_frames = osMessageQueueNew(32, sizeof(CAN_RawFrame),   NULL);
q_telemetry  = osMessageQueueNew(8,  sizeof(TelemetryFrame), NULL);
q_tx_cmd     = osMessageQueueNew(4,  sizeof(DriveCmd),       NULL);
/* USER CODE END RTOS_QUEUES */

/* USER CODE BEGIN RTOS_THREADS */
osThreadNew(Task_CAN_Decode, NULL,
    &(osThreadAttr_t){ .name="CANDecode", .stack_size=512, .priority=osPriorityHigh });
osThreadNew(Task_USB_TX, NULL,
    &(osThreadAttr_t){ .name="USBTX",     .stack_size=512, .priority=osPriorityNormal });
osThreadNew(Task_Drive_TX, NULL,
    &(osThreadAttr_t){ .name="DriveTX",   .stack_size=256, .priority=osPriorityNormal });
/* USER CODE END RTOS_THREADS */

/* USER CODE BEGIN 4 */

// Task 1: odbiera surowe ramki z ISR, dekoduje, wrzuca zdekodowane do kolejki
void Task_CAN_Decode(void *arg) {
    CAN_RawFrame   raw;
    TelemetryFrame decoded;
    for (;;) {
        if (osMessageQueueGet(q_raw_frames, &raw, NULL, osWaitForever) == osOK) {
            can_decode_and_update(&raw, &decoded);
            // Nadpisuj ostatni element zamiast kolejkować — zawsze świeże dane
            osMessageQueuePut(q_telemetry, &decoded, 0, 0);
        }
    }
}

// Task 2: wysyła JSON przez USB CDC co 20ms
void Task_USB_TX(void *arg) {
    TelemetryFrame frame;
    for (;;) {
        if (osMessageQueueGet(q_telemetry, &frame, NULL, pdMS_TO_TICKS(20)) == osOK) {
            usb_send_telemetry(&frame);
        }
        osDelay(20);
    }
}

// Task 3: odbiera komendy z PC i wysyła ramki TX na CAN
void Task_Drive_TX(void *arg) {
    DriveCmd cmd;
    extern CAN_HandleTypeDef hcan1;
    for (;;) {
        if (osMessageQueueGet(q_tx_cmd, &cmd, NULL, osWaitForever) == osOK) {
            can_send_drive_mode(&hcan1, cmd);
        }
    }
}

/* USER CODE END 4 */