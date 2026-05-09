# GitHub Publishing Notes

Suggested repository name:

```text
scada-modbus-tcp-relay-timer
```

Suggested short description:

```text
Arduino Modbus TCP slave relay timer for SCADA with EEPROM runtime storage and Ethernet watchdog.
```

Suggested topics:

```text
arduino
modbus
modbus-tcp
scada
ethernet
w5100
w5500
eeprom
relay
industrial-automation
```

Recommended first commit:

```text
Initial release of SCADA Modbus TCP relay timer
```

Recommended GitHub commands after creating an empty repository:

```bash
git init
git add .
git commit -m "Initial release of SCADA Modbus TCP relay timer"
git branch -M main
git remote add origin https://github.com/UterGrooll/scada-modbus-tcp-relay-timer.git
git push -u origin main
```

Before publishing:

- Check that the static IP address, MAC address, and pin numbers are safe to publish.
- Confirm the license choice.
- Add project photos only if they do not expose private infrastructure, serial numbers, passwords, IP schemes, or company-sensitive details.
- If photos are added, place them in `docs/images/` and reference them from the README.
- Add the wiring diagram as `docs/images/wiring-example.png`.
- Add Rapid SCADA screenshots after removing any sensitive hostnames, IP addresses, object names, or company-specific labels.
