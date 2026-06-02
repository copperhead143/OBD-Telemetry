# Dokumentacja projektu — OBD-Telemetry

## Cel
- Krótkie narzędzie telemetryczne dla pojazdu OBD: sniffowanie ramek CAN, dekodowanie parametrów (RPM, prędkości kół, TPS, temperatury, boost itp.) i wysyłka w formacie JSON przez USB CDC oraz opcjonalne wysyłanie poleceń trybu jazdy.

## Kluczowe pliki
- `firmware/Core/Src/can_handler.c` — konfiguracja filtrów CAN, ISR RX, dekoder ramek, (szablon) funkcja nadawcza trybu jazdy.
- `firmware/Core/Inc/can_handler.h` — definicje struktur danych: `CAN_RawFrame`, `TelemetryFrame`, enum `DriveCmd` i deklaracje kolejek.
- `firmware/Core/Src/usb_handler.c` — serializacja `TelemetryFrame` do JSON i funkcja parsująca komendy przychodzące przez CDC.
- `firmware/Core/Inc/usb_handler.h` — prototypy `usb_send_telemetry()` i `usb_parse_cmd()`.
- `firmware/Core/Src/freertos.c` — tworzenie kolejek FreeRTOS oraz implementacja zadań: `Task_CAN_Decode`, `Task_USB_TX`, `Task_Drive_TX`.
- `firmware/Core/Src/main.c` — inicjalizacja peryferiów (CAN1/CAN2, GPIO, USB), start RTOS i `defaultTask` (uruchamia USB device).
- `firmware/obd-telemetry.ioc` — konfiguracja CubeMX (można otworzyć w STM32CubeIDE aby wygenerować/zmodyfikować projekt).
- `firmware/EWARM/` — projekt Keil MDK-ARM (pliki projektu do budowy dla Keil).

## Architektura runtime
- ISR CAN (HAL callback `HAL_CAN_RxFifo0MsgPendingCallback`) odczytuje ramkę i wrzuca `CAN_RawFrame` do kolejki `q_raw_frames`.
- `Task_CAN_Decode` pobiera surowe ramki, wywołuje dekoder (`can_decode_and_update`) i publikuje aktualny `TelemetryFrame` do `q_telemetry` (zastępując poprzedni wpis).
- `Task_USB_TX` odczytuje `q_telemetry` i co ~20 ms wysyła JSON przez USB CDC (`usb_send_telemetry`).
- `Task_Drive_TX` czeka na komendy w `q_tx_cmd` i przekazuje je do funkcji nadawczej CAN (`can_send_drive_mode`) — obecnie jest to szablon/zakomentowane (wymaga uzupełnienia ID i payload).

## Struktury i kolejki
- `CAN_RawFrame` (w `can_handler.h`): { `id`, `dlc`, `data[8]`, `timestamp` } — surowa ramka odbierana z ISR.
- `TelemetryFrame`: zawiera pola: `rpm`, `speed` (km/h*10), `temp_engine`, `temp_oil`, `boost_kpa`, `throttle`, `tv_rear_left/right`, `drive_mode`, `wheel_spd[4]`, `timestamp`.
- Kolejki (wg `freertos.c`): `q_raw_frames` (32 elementy), `q_telemetry` (8 elementów), `q_tx_cmd` (4 elementy).

## Dekodowanie CAN (co robi kod)
- Filtry: `can_filter_init_all()` ustawia CAN1 w trybie "sniff" (przepuszcza wszystkie ramki), CAN2 jako slave (banki filtrów zależne od układu F405).
- ISR `HAL_CAN_RxFifo0MsgPendingCallback`: pobiera nagłówek i bajty ramki, buduje `CAN_RawFrame` i wrzuca do `q_raw_frames` (wersja FromISR).
- `decode_frame()` rozpoznaje kilka stałych ID i ekstraktuje wartości:
  - `CAN_ID_WHEEL_SPEED` (0x190): bajty 0-1 FL, 2-3 FR, 4-5 RL, 6-7 RR; zapisane jako 16-bit, `speed` = średnia czterech kół.
  - `CAN_ID_THROTTLE` (0x213): throttle = data[0] * 100/255.
  - `CAN_ID_TEMP` (0x420): `temp_engine` = data[0] - 40.
  - OBD-II response `0x7E8`: obsługa service 0x41 (response dla service 01) — parsuje PIDy: 0x0C (RPM), 0x0D (vehicle speed), 0x0B (boost/MAP), 0x5C (oil temp).
- Nieznane ID są ignorowane przez dekoder (zapisywane jako raw do sniffingu — można wysyłać je przez USB w celu analizy).

## USB / protokół szeregowy
- `usb_send_telemetry()` serializuje `TelemetryFrame` do pojedynczego JSON-owego obiektu zakończonego CRLF. Przykład pól:
  - `ts` (timestamp), `rpm`, `spd` (km/h), `boost`, `tps`, `t_eng`, `t_oil`, `tv_rl`, `tv_rr`, `spd_fl`, `spd_fr`, `spd_rl`, `spd_rr`, `mode`.
- `usb_parse_cmd()` parsuje komendy tekstowe w formacie `CMD:NAME`, obsługiwane predefiniowane: `CMD:NORMAL`, `CMD:SPORT`, `CMD:TRACK`, `CMD:DRIFT`, `CMD:SNOW`. Zwraca enum `DriveCmd` lub `CMD_NONE`.

## Nadawanie poleceń (TX) — status
- W `can_handler.c` istnieje szkic funkcji `can_send_drive_mode()` oraz tablica `drive_mode_data[]`, ale są one zakomentowane i zawierają wartości placeholder. Aby wysyłać polecenia:
  1. Uzupełnić rzeczywiste ID nadawcze (`StdId`) i payloady (z sniffingu oryginalnych ramek).
  2. Odkomentować/zweryfikować `can_send_drive_mode()` i ewentualnie dodać rate-limiting.


## Budowa i wgrywanie
- Projekt Keil: otwórz `firmware/EWARM/obd-telemetry.ewp` (lub WorkSpace `Project.eww`) w Keil MDK-ARM i skompiluj.
- Alternatywnie otwórz `firmware/obd-telemetry.ioc` w STM32CubeIDE (CubeMX) aby wygenerować projekt dla STM32CubeIDE i skompilować/wgrać przez ST-Link.
- Do wgrywania użyj ST-Linka (np. z poziomu Keil/STM32CubeIDE) lub innego programatora kompatybilnego z STM32F4.

---