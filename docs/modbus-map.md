# Modbus Map

The device works as a Modbus TCP slave.

## Coils

Read: `FC01`  
Write: `FC05` / `FC15`

| Address | Description | Format |
| --- | --- | --- |
| `0` | Relay command | `0 = OFF`, `1 = ON` |

## Input Registers

Read: `FC04`

| Address | Description | Format |
| --- | --- | --- |
| `0` | Actual relay state | `0 = OFF`, `1 = ON` |
| `1` | Remaining time | Seconds |

Modbus 3xxxx notation:

| Modbus Poll address | Zero-based address | Description |
| --- | ---: | --- |
| `30001` | `0` | Actual relay state |
| `30002` | `1` | Remaining time in seconds |

## Holding Registers

Read: `FC03`  
Write: `FC06` / `FC16`

| Address | Description | Format |
| --- | --- | --- |
| `0` | Relay runtime | Seconds |

Default value: `300` seconds.

Allowed range:

| Limit | Value |
| --- | ---: |
| Minimum | `1` second |
| Maximum | `3600` seconds |

Values outside the allowed range are clamped by the firmware.

## Modbus Poll Setup

### Turn Relay On or Off

```text
Function: 05 Write Single Coil
Address: 0
Value: ON / OFF
```

### Read Relay State and Remaining Time

```text
Function: 04 Read Input Registers
Address: 0
Quantity: 2
```

Expected result:

- `30001 / Address 0`: relay state.
- `30002 / Address 1`: remaining time in seconds.

### Read Configured Runtime

```text
Function: 03 Read Holding Registers
Address: 0
Quantity: 1
```

### Write New Runtime

```text
Function: 06 Write Single Register
Address: 0
Value: 120
```

Example: writing `120` sets the relay runtime to 120 seconds and saves the value to EEPROM.
