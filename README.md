# OBD-Telemetry



ramki focus:
070 moment obliczony na skrzyni
080 tps // 9000=0%, 98e3=100%
090 rpm
420 tryby jazdy
0x190 predkosci kol obrotowych

ID: 0x1B0
Refresh Rate: 20ms

Torque Vectoring Max Torque
Represents a request from the AWD module to limit wheel torque
Length: 13 bits
Mask: 00 00 00 00 00 1F FF 00
Torque Vectoring Max Torque = Value - 1250 (N*m)

ID: 0x2C0
Refresh Rate: 50ms

Torque Vectoring Right Torque
Represents amount of torque sent to the right (passenger side) of the car
Length: 12bits
Mask: 00 00 00 00 0F FF 00 00

If value = 0xFFE, value is unknown
If value = 0xFFF, value cannot be determined due to fault
All other values:
Right Torque = Value (N*m)

Torque Vectoring Left Torque
Represents amount of torque sent to the left (driver side) of the car
Length: 12bits
Mask: 0F FF 00 00 00 00 00 00

If value = 0xFFE, value is unknown
If value = 0xFFF, value cannot be determined due to fault
All other values:
Left Torque = Value (N*m)