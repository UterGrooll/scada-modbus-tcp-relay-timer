# Modbus Map

The device works as a Modbus TCP slave.

## Coils

Read: `FC01`  
Write: `FC05` / `FC15`

| Address | Description | Format |
| --- | --- | --- |
| `0` | Relay command | `0 = OFF`, `1 = ON` |
| `1` | Relay timer enable | `0 = timer disabled`, `1 = timer enabled` |

## Input Registers

Read: `FC04`

| Address | Description | Format |
| --- | --- | --- |
| `0` | Actual relay state | `0 = OFF`, `1 = ON` |
| `1` | Remaining time | Seconds |
| `2` | Relay timer enabled state | `0 = disabled`, `1 = enabled` |

Modbus 3xxxx notation:

| Modbus Poll address | Zero-based address | Description |
| --- | ---: | --- |
| `30001` | `0` | Actual relay state |
| `30002` | `1` | Remaining time in seconds |
| `30003` | `2` | Relay timer enabled state |

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

### Enable or Disable Relay Timer

```text
Function: 05 Write Single Coil
Address: 1
Value: ON / OFF
```

When `Coil 1` is ON, the relay turns off automatically after the configured runtime. When `Coil 1` is OFF, the relay stays on until SCADA writes OFF to `Coil 0`.

### Read Relay State and Remaining Time

```text
Function: 04 Read Input Registers
Address: 0
Quantity: 3
```

Expected result:

- `30001 / Address 0`: relay state.
- `30002 / Address 1`: remaining time in seconds.
- `30003 / Address 2`: relay timer enabled state.

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
