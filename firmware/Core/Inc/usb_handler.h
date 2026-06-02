#ifndef USB_HANDLER_H
#define USB_HANDLER_H

#include "can_handler.h"

// Serializuje TelemetryFrame do JSON i wysyła przez CDC
void usb_send_telemetry(const TelemetryFrame *f);

// Parsuje komendę przychodzącą z PC przez CDC
// format: "CMD:SPORT\r\n" itp.
DriveCmd usb_parse_cmd(const uint8_t *buf, uint32_t len);

#endif