#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include "ModbusTCP_RU.h"

extern EthernetServer MbServer;

ModbusTCP_RU Mb;

byte mac[]  = {0x02, 0x47, 0xA1, 0x10, 0x00, 0x04};
IPAddress ip(192, 168, 0, 178);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

/* ---------- Pins ---------- */
#define RELAY_PIN 2
#define W5100_RESET_PIN 3

/* ---------- Modbus map ---------- */
const word COIL_RELAY = 0;
const word COIL_RELAY_TIMER_ENABLE = 1;

const word IREG_RELAY_STATE = 0;
const word IREG_REMAINING_SEC = 1;
const word IREG_RELAY_TIMER_ENABLED = 2;

const word HREG_RELAY_TIME_SEC = 0;

/* ---------- EEPROM ---------- */
const int EEPROM_RELAY_TIME_ADDR = 0;
const int EEPROM_RELAY_TIMER_ENABLE_ADDR = 2;
const int EEPROM_MAGIC_ADDR = 10;
const uint16_t EEPROM_MAGIC = 0xA55A;

const uint16_t DEFAULT_RELAY_TIME_SEC = 300;
const uint16_t MIN_RELAY_TIME_SEC = 1;
const uint16_t MAX_RELAY_TIME_SEC = 3600;

uint16_t relayTimeSec = DEFAULT_RELAY_TIME_SEC;
uint16_t lastSavedRelayTimeSec = DEFAULT_RELAY_TIME_SEC;
bool relayTimeApplying = false;
bool relayTimerEnabled = true;
bool lastSavedRelayTimerEnabled = true;
bool relayTimerEnableApplying = false;

/* ---------- Logic ---------- */
const unsigned long W5100_RESET_INTERVAL_MS = 420000UL;

/* ---------- Ethernet watchdog ---------- */
const unsigned long LINK_CHECK_PERIOD_MS = 1000UL;
const unsigned long LINK_RECOVER_DELAY_MS = 1500UL;
const unsigned long FORCE_REINIT_PERIOD_MS = 30000UL;

unsigned long linkCheckTimer = 0;
unsigned long linkDownTime = 0;
unsigned long lastEthernetRestart = 0;
bool linkWasDown = false;

bool relayCommand = false;
bool relayActive = false;

unsigned long relayStartTime = 0;
unsigned long w5100ResetTime = 0;
bool isW5100ResetPending = false;

void writeEEPROMMagic()
{
  EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
}

void saveRelayTimeToEEPROM(uint16_t value)
{
  byte storedTimerEnabled = relayTimerEnabled ? 1 : 0;

  writeEEPROMMagic();
  EEPROM.put(EEPROM_RELAY_TIME_ADDR, value);
  EEPROM.put(EEPROM_RELAY_TIMER_ENABLE_ADDR, storedTimerEnabled);
  lastSavedRelayTimeSec = value;
  lastSavedRelayTimerEnabled = relayTimerEnabled;
}

void saveRelayTimerEnableToEEPROM(bool value)
{
  byte storedValue = value ? 1 : 0;

  writeEEPROMMagic();
  EEPROM.put(EEPROM_RELAY_TIME_ADDR, relayTimeSec);
  EEPROM.put(EEPROM_RELAY_TIMER_ENABLE_ADDR, storedValue);
  lastSavedRelayTimeSec = relayTimeSec;
  lastSavedRelayTimerEnabled = value;
}

uint16_t loadRelayTimeFromEEPROM()
{
  uint16_t magic = 0;
  uint16_t value = 0;

  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  EEPROM.get(EEPROM_RELAY_TIME_ADDR, value);

  if (magic != EEPROM_MAGIC) {
    return DEFAULT_RELAY_TIME_SEC;
  }

  if (value < MIN_RELAY_TIME_SEC || value > MAX_RELAY_TIME_SEC) {
    return DEFAULT_RELAY_TIME_SEC;
  }

  return value;
}

bool loadRelayTimerEnableFromEEPROM()
{
  uint16_t magic = 0;
  byte value = 1;

  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  EEPROM.get(EEPROM_RELAY_TIMER_ENABLE_ADDR, value);

  if (magic != EEPROM_MAGIC) {
    return true;
  }

  if (value > 1) {
    return true;
  }

  return value == 1;
}

uint16_t normalizeRelayTime(uint16_t value)
{
  if (value < MIN_RELAY_TIME_SEC) {
    return MIN_RELAY_TIME_SEC;
  }

  if (value > MAX_RELAY_TIME_SEC) {
    return MAX_RELAY_TIME_SEC;
  }

  return value;
}

void setRelayTime(uint16_t value, bool saveToEeprom)
{
  relayTimeSec = normalizeRelayTime(value);

  if (!relayTimeApplying) {
    relayTimeApplying = true;
    Mb.Hreg(HREG_RELAY_TIME_SEC, relayTimeSec);
    relayTimeApplying = false;
  }

  if (saveToEeprom && relayTimeSec != lastSavedRelayTimeSec) {
    saveRelayTimeToEEPROM(relayTimeSec);
  }
}

void setRelayTimerEnabled(bool enabled, bool saveToEeprom)
{
  relayTimerEnabled = enabled;

  if (!relayTimerEnableApplying) {
    relayTimerEnableApplying = true;
    Mb.Coil(COIL_RELAY_TIMER_ENABLE, relayTimerEnabled);
    relayTimerEnableApplying = false;
  }

  Mb.Ireg(IREG_RELAY_TIMER_ENABLED, relayTimerEnabled ? 1 : 0);

  if (relayTimerEnabled && relayActive) {
    relayStartTime = millis();
  }

  if (saveToEeprom && relayTimerEnabled != lastSavedRelayTimerEnabled) {
    saveRelayTimerEnableToEEPROM(relayTimerEnabled);
  }
}

void onHoldingWrite(word address, word value)
{
  if (address != HREG_RELAY_TIME_SEC) {
    return;
  }

  if (relayTimeApplying) {
    return;
  }

  setRelayTime(value, true);
}

void restartEthernet()
{
  Ethernet.begin(mac, ip, gateway, subnet);
  delay(500);
  MbServer.begin();
  lastEthernetRestart = millis();
}

void resetW5100()
{
  digitalWrite(W5100_RESET_PIN, LOW);
  delay(200);
  digitalWrite(W5100_RESET_PIN, HIGH);
  delay(1000);

  restartEthernet();
}

void updateEthernetWatchdog()
{
  unsigned long now = millis();

  if (now - linkCheckTimer < LINK_CHECK_PERIOD_MS) {
    return;
  }
  linkCheckTimer = now;

  EthernetLinkStatus link = Ethernet.linkStatus();

  if (link == LinkOFF) {
    if (!linkWasDown) {
      linkWasDown = true;
      linkDownTime = now;
    }
    return;
  }

  if (link == LinkON && linkWasDown) {
    if (now - linkDownTime >= LINK_RECOVER_DELAY_MS) {
      linkWasDown = false;
      restartEthernet();
    }
    return;
  }

  if (link == Unknown && (now - lastEthernetRestart >= FORCE_REINIT_PERIOD_MS)) {
    restartEthernet();
  }
}

void setRelayActive(bool active)
{
  relayActive = active;
  digitalWrite(RELAY_PIN, active ? LOW : HIGH);

  Mb.Ireg(IREG_RELAY_STATE, active ? 1 : 0);
}

void startRelay()
{
  unsigned long now = millis();

  relayStartTime = now;
  w5100ResetTime = now;
  isW5100ResetPending = true;

  setRelayActive(true);
}

void stopRelay()
{
  relayCommand = false;
  Mb.Coil(COIL_RELAY, false);
  setRelayActive(false);
}

void onCoilWrite(word address, bool value)
{
  if (address == COIL_RELAY_TIMER_ENABLE) {
    if (relayTimerEnableApplying) {
      return;
    }

    setRelayTimerEnabled(value, true);
    return;
  }

  if (address != COIL_RELAY) {
    return;
  }

  if (value == relayCommand) {
    return;
  }

  relayCommand = value;

  if (relayCommand) {
    startRelay();
  } else {
    stopRelay();
  }
}

void updateRelayTimer()
{
  if (!relayActive || !relayTimerEnabled) {
    Mb.Ireg(IREG_REMAINING_SEC, 0);
    return;
  }

  unsigned long now = millis();
  unsigned long relayIntervalMs = (unsigned long)relayTimeSec * 1000UL;
  unsigned long elapsed = now - relayStartTime;

  if (elapsed >= relayIntervalMs) {
    stopRelay();
    return;
  }

  unsigned long remaining = (relayIntervalMs - elapsed) / 1000UL;
  Mb.Ireg(IREG_REMAINING_SEC, (word)remaining);
}

void updateW5100Reset()
{
  if (!isW5100ResetPending) {
    return;
  }

  if (millis() - w5100ResetTime >= W5100_RESET_INTERVAL_MS) {
    resetW5100();
    isW5100ResetPending = false;
  }
}

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(W5100_RESET_PIN, OUTPUT);
  digitalWrite(W5100_RESET_PIN, HIGH);

  relayTimeSec = loadRelayTimeFromEEPROM();
  lastSavedRelayTimeSec = relayTimeSec;
  relayTimerEnabled = loadRelayTimerEnableFromEEPROM();
  lastSavedRelayTimerEnabled = relayTimerEnabled;

  Mb.Coil(COIL_RELAY, false);
  Mb.Coil(COIL_RELAY_TIMER_ENABLE, relayTimerEnabled);
  Mb.Ireg(IREG_RELAY_STATE, 0);
  Mb.Ireg(IREG_REMAINING_SEC, 0);
  Mb.Ireg(IREG_RELAY_TIMER_ENABLED, relayTimerEnabled ? 1 : 0);

  setRelayTime(relayTimeSec, false);
  setRelayTimerEnabled(relayTimerEnabled, false);

  Mb.onCoilWrite(onCoilWrite);
  Mb.onHoldingWrite(onHoldingWrite);

  Ethernet.init(10);
  restartEthernet();
}

void loop()
{
  Mb.MbsRun();

  updateEthernetWatchdog();
  updateRelayTimer();
  updateW5100Reset();
}
