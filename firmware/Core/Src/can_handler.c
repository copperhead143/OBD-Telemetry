#include "can_handler.h"
#include <string.h>

osMessageQueueId_t q_raw_frames;
osMessageQueueId_t q_telemetry;
osMessageQueueId_t q_tx_cmd;

// ── Filtry ────────────────────────────────────────────────────────────────

void can_filter_init_all(CAN_HandleTypeDef *hcan1, CAN_HandleTypeDef *hcan2) {

    // CAN1 — przepuść wszystko (sniffing), banki 0-13
    CAN_FilterTypeDef f = {0};
    f.FilterActivation     = CAN_FILTER_ENABLE;
    f.FilterBank           = 0;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh         = 0x0000;
    f.FilterMaskIdHigh     = 0x0000;
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdLow      = 0x0000;
    HAL_CAN_ConfigFilter(hcan1, &f);

    // CAN2 — banki od 14 (slave do CAN1 w F405)
    f.FilterBank           = 14;
    f.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(hcan2, &f);

    HAL_CAN_Start(hcan1);
    HAL_CAN_Start(hcan2);
    HAL_CAN_ActivateNotification(hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
}

// ── Callback przerwania RX ────────────────────────────────────────────────
// Wywoływany z ISR — tylko wrzuca do kolejki, nic więcej

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef hdr;
    CAN_RawFrame frame;

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, frame.data) != HAL_OK)
        return;

    frame.id        = hdr.StdId;
    frame.dlc       = hdr.DLC;
    frame.timestamp = HAL_GetTick();

    // Z ISR używamy FromISR wersji API
    osMessageQueuePut(q_raw_frames, &frame, 0, 0);
}

// ── Dekoder ramek ─────────────────────────────────────────────────────────

// Znane CAN ID (standardowe + do uzupełnienia po sniffingu)
#define CAN_ID_RPM          0x0C0   // lub z OBD response 0x7E8
#define CAN_ID_WHEEL_SPEED  0x190
#define CAN_ID_THROTTLE     0x213
#define CAN_ID_TEMP         0x420
// Poniższe do odkrycia przez sniffing — tymczasowe placeholdery
#define CAN_ID_TV           0x000   // torque vectoring — TBD
#define CAN_ID_DRIVEMODE    0x000   // drive mode — TBD

static TelemetryFrame current = {0};

static void decode_frame(const CAN_RawFrame *f) {
    switch (f->id) {

        case CAN_ID_WHEEL_SPEED:
            // Bajty 0-1: FL, 2-3: FR, 4-5: RL, 6-7: RR
            // Skala: value * 0.01 = km/h (typowa dla Forda)
            current.wheel_spd[0] = (f->data[0] << 8 | f->data[1]);
            current.wheel_spd[1] = (f->data[2] << 8 | f->data[3]);
            current.wheel_spd[2] = (f->data[4] << 8 | f->data[5]);
            current.wheel_spd[3] = (f->data[6] << 8 | f->data[7]);
            // Prędkość pojazdu = średnia czterech kół
            current.speed = (current.wheel_spd[0] + current.wheel_spd[1] +
                             current.wheel_spd[2] + current.wheel_spd[3]) / 4;
            break;

        case CAN_ID_THROTTLE:
            current.throttle = (uint8_t)(f->data[0] * 100 / 255);
            break;

        case CAN_ID_TEMP:
            current.temp_engine = (int16_t)f->data[0] - 40;
            break;

        // OBD-II response (0x7E8) — odpowiedź ECU na zapytanie 0x7DF
        case 0x7E8:
            if (f->data[1] == 0x41) {  // service 01 response
                switch (f->data[2]) {
                    case 0x0C:  // RPM
                        current.rpm = ((f->data[3] << 8) | f->data[4]) / 4;
                        break;
                    case 0x0D:  // prędkość
                        current.speed = f->data[3] * 10;
                        break;
                    case 0x0B:  // boost (MAP)
                        current.boost_kpa = f->data[3];
                        break;
                    case 0x5C:  // temperatura oleju
                        current.temp_oil = (int16_t)f->data[3] - 40;
                        break;
                }
            }
            break;

        default:
            // Nieznane ID — logujemy surową ramkę do sniffingu
            // (task decoder wyśle ją przez USB jako raw)
            break;
    }
    current.timestamp = f->timestamp;
}

// Publiczna funkcja dekodera — wywołana z taska
void can_decode_and_update(const CAN_RawFrame *f, TelemetryFrame *out) {
    decode_frame(f);
    *out = current;
}

// ── Nadawanie TX — zmiana trybu jazdy ────────────────────────────────────

// Bajty ramki Drive Mode — do uzupełnienia po sniffingu
// Na razie struktura gotowa, wartości TBD
/*static const uint8_t drive_mode_data[5][8] = {
    [CMD_MODE_NORMAL] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    [CMD_MODE_SPORT]  = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    [CMD_MODE_TRACK]  = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    [CMD_MODE_DRIFT]  = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    [CMD_MODE_SNOW]   = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static uint32_t last_tx_tick = 0;

void can_send_drive_mode(CAN_HandleTypeDef *hcan, DriveCmd cmd) {
    if (cmd == CMD_NONE) return;

    // Rate limiting — max 1 TX co 500ms
    if (HAL_GetTick() - last_tx_tick < 500) return;

    CAN_TxHeaderTypeDef hdr = {
        .StdId              = 0x000,  // TBD po sniffingu
        .IDE                = CAN_ID_STD,
        .RTR                = CAN_RTR_DATA,
        .DLC                = 8,
        .TransmitGlobalTime = DISABLE,
    };

    uint32_t mailbox;
    HAL_CAN_AddTxMessage(hcan, &hdr, (uint8_t*)drive_mode_data[cmd], &mailbox);
    last_tx_tick = HAL_GetTick();
}*/