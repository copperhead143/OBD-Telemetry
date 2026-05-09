#include "can_handler.h"
#include <string.h>
#include "../../Drivers/CMSIS/RTOS2/Include/cmsis_os2.h"

osMessageQueueId_t q_raw_frames;
osMessageQueueId_t q_telemetry;
osMessageQueueId_t q_tx_cmd;

// -=-=-=-=- FILTRY -=-=-=-=-=-

void can_filter_init_all(CAN_HandleTypeDef *hcan1, CAN_HandleTypeDef *hcan2){

    //CAN1 - przepuszczanie wszystkiego do sniffingu
    CAN_FilterTypeDef f = {0};
    f.FilterActivation      = CAN_FILTER_ENABLE;
    f.FilterBank            = 0;
    f.FilterFIFOAssignment  = CAN_RX_FIFO0;
    f.FilterMode            = CAN_FILTERMODE_IDMASK;
    f.FilterScale           = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh          = 0x0000;
    f.FilterMaskIdHigh      = 0x0000;
    f.FilterIdLow           = 0x0000;
    f.FilterMaskIdLow       = 0x0000;
    HAL_CAN_ConfigFilter(hcan1, &f);

    //CAN2 - banki od 14 (slvave do CAN1 wg dokumentacji F405RGT6)
    f.FilterBank           = 14;
    f.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(hcan2, &f);

    HAL_CAN_Start(hcan1);
    HAL_CAN_Start(hcan2);
    HAL_CAN_ActivateNotification(hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
}