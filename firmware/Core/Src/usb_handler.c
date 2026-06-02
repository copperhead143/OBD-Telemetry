#include "usb_handler.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <string.h>

void usb_send_telemetry(const TelemetryFrame *f) {
    char buf[320];
    int len = snprintf(buf, sizeof(buf),
        "{"
        "\"ts\":%lu,"
        "\"rpm\":%u,"
        "\"spd\":%.1f,"
        "\"boost\":%u,"
        "\"tps\":%u,"
        "\"t_eng\":%d,"
        "\"t_oil\":%d,"
        "\"tv_rl\":%u,"
        "\"tv_rr\":%u,"
        "\"spd_fl\":%.1f,"
        "\"spd_fr\":%.1f,"
        "\"spd_rl\":%.1f,"
        "\"spd_rr\":%.1f,"
        "\"mode\":%u"
        "}\r\n",
        f->timestamp,
        f->rpm,
        f->speed        / 10.0f,
        f->boost_kpa,
        f->throttle,
        f->temp_engine,
        f->temp_oil,
        f->tv_rear_left,
        f->tv_rear_right,
        f->wheel_spd[0] / 10.0f,
        f->wheel_spd[1] / 10.0f,
        f->wheel_spd[2] / 10.0f,
        f->wheel_spd[3] / 10.0f,
        f->drive_mode
    );
    CDC_Transmit_FS((uint8_t*)buf, (uint16_t)len);
}

DriveCmd usb_parse_cmd(const uint8_t *buf, uint32_t len) {
    if (len < 4) return CMD_NONE;
    if (strncmp((char*)buf, "CMD:NORMAL", 10) == 0) return CMD_MODE_NORMAL;
    if (strncmp((char*)buf, "CMD:SPORT",   9) == 0) return CMD_MODE_SPORT;
    if (strncmp((char*)buf, "CMD:TRACK",   9) == 0) return CMD_MODE_TRACK;
    if (strncmp((char*)buf, "CMD:DRIFT",   9) == 0) return CMD_MODE_DRIFT;
    if (strncmp((char*)buf, "CMD:SNOW",    8) == 0) return CMD_MODE_SNOW;
    return CMD_NONE;
}