#ifndef CAN_HANDLER_H
#define CAN_HANDER_H

#include "stm32f405xx.h"
#include "stm32f4xx_hal.h"
#include "cmsis_armcc.h"


//struct pojedynczej ramki can
typedef struct{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t timestamp;
} CAN_RawFrame;

//ramka wysyłana do telemetrii
typedef struct{
    uint16_t rpm;
    uint16_t speed;        // km/h * 10
    int16_t  temp_engine;  // °C
    int16_t  temp_oil;     // °C
    uint16_t boost_kpa;    // kPa
    uint8_t  throttle;     // %
    uint8_t  tv_rear_left; // % momentu lewe tylne koło
    uint8_t  tv_rear_right;// % momentu prawe tylne koło
    uint8_t  drive_mode;   // 0=Normal 1=Sport 2=Track 3=Drift 4=Snow
    uint16_t wheel_spd[4]; // FL FR RL RR, km/h * 10
    uint32_t timestamp;
} TelemetryFrame;

// Komendy TX z PC
typedef enum {
    CMD_NONE = 0,
    CMD_MODE_NORMAL,
    CMD_MODE_SPORT,
    CMD_MODE_TRACK,
    CMD_MODE_DRIFT,
    CMD_MODE_SNOW,
} DriveCmd;

//PONIŻEJ DO ZDEFINIOWANIA W can_handler.c

//freeRTOS kolejki
extern osMessageQueueId_t q_raw_frames;
extern osMessageQueueId_t q_telemetry;
extern osMessageQueueId_t q_tx_cmd;


void can_filter_init_all(CAN_HandleTypeDef *hcan1, CAN_HandleTypeDef *hcan2);
void can_send_drive_mode(CAN_HandleTypeDef *hcan, DriveCmd cmd);

#endif