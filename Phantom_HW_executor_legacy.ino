// =====================================================================================
// Phantom_HW_executor.ino
// Arduino Portenta H7 = HARDWARE EXECUTOR ONLY
//
// Python owns:
//   - waveform generation (pressure.py, morphology.py)
//   - AC/DC interpretation
//   - target SYS/DIA
//   - feedback controller
//   - zero calibration
//   - CSV logging
//
// Arduino owns:
//   - AD5693R DAC output
//   - ADC121C021 P1/P2 raw reads
//   - immediate command/response
//   - host-loss pump failsafe
//
// I2C:
//   P1 inlet ADC  = 0x55
//   P2 outlet ADC = 0x54
//   Pump DAC      = 0x4C or 0x4E
//
// Serial: 115200 baud, ASCII lines
//
// PC -> Arduino:
//   PING
//   CHECK
//   STEP <volts>
//   PUMP OFF
//
// Arduino -> PC:
//   #PONG
//   #CHECK,p1=OK,p2=OK,dac=OK@0x4C,vmax=5.00,system=READY
//   #OK,PUMP OFF
//   D,<t_ms>,<seq>,<raw1>,<ok1>,<raw2>,<ok2>,<pump_v>
//
// IMPORTANT:
//   Arduino DOES NOT create a sine wave and DOES NOT calculate blood pressure.
//   Python sends every pump-voltage sample with STEP.
//
// HARDWARE NOTE (AD5693R):
//   DAC_GAIN2 = true asks for 0..5 V, but gain x2 needs VDD >= VREF + 1.5 V = 4.0 V.
//   If the DAC breakout is powered from the Portenta's 3.3 V (e.g. STEMMA QT),
//   the output stops at ~3.3 V. Power the DAC VIN from 5 V to get the full range.
// =====================================================================================

#include <Wire.h>

// ---------------- Hardware settings ----------------
const uint8_t ADDR_P1 = 0x55;
const uint8_t ADDR_P2 = 0x54;

const float VMAX = 5.0f;

const bool DAC_GAIN2 = true;
// true  -> nominal 0..5 V (needs DAC VDD >= 4.0 V)
// false -> nominal 0..2.5 V

const uint8_t N_AVG = 4;

// Python normally sends STEP at ~50 Hz.
// If Python stops sending valid commands while pump is active,
// Arduino forces pump to zero.
const uint32_t HOST_FAILSAFE_MS = 500;


// ---------------- State ----------------
uint8_t dacAddr = 0;

float dacFullScale = DAC_GAIN2 ? 5.0f : 2.5f;

float pumpV = 0.0f;

uint32_t seq = 0;

uint32_t lastHostCommandMs = 0;

bool hostWasConnected = false;

String rxLine;


// =====================================================================================
// ADC121C021
// =====================================================================================

int adcRaw(uint8_t addr)
{
    // Select conversion register 0x00, repeated-start read.
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x00);

    if (Wire.endTransmission(false) != 0)
    {
        return -1;
    }

    if (Wire.requestFrom(addr, (uint8_t)2) != 2)
    {
        return -1;
    }

    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();

    uint16_t raw = (((uint16_t)msb << 8) | lsb) & 0x0FFF;

    return (int)raw;
}


float adcAvg(uint8_t addr)
{
    // Discard one read first.
    adcRaw(addr);

    long sum = 0;
    int good = 0;

    for (uint8_t i = 0; i < N_AVG; i++)
    {
        int r = adcRaw(addr);

        if (r >= 0)
        {
            sum += r;
            good++;
        }
    }

    if (good == 0)
    {
        return -1.0f;
    }

    return (float)sum / (float)good;
}


bool adcAlive(uint8_t addr)
{
    return adcRaw(addr) >= 0;
}


// Generic I2C presence. Used only for DAC.
bool i2cPresent(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}


// =====================================================================================
// AD5693R
// =====================================================================================

bool dacWrite16(uint8_t cmd, uint16_t data)
{
    if (!dacAddr)
    {
        return false;
    }

    Wire.beginTransmission(dacAddr);
    Wire.write(cmd);
    Wire.write((uint8_t)(data >> 8));
    Wire.write((uint8_t)(data & 0xFF));

    return Wire.endTransmission() == 0;
}


bool dacInit()
{
    if (i2cPresent(0x4C))
    {
        dacAddr = 0x4C;
    }
    else if (i2cPresent(0x4E))
    {
        // Alternate AD5693R address if A0 is configured differently.
        dacAddr = 0x4E;
    }
    else
    {
        dacAddr = 0;
        return false;
    }

    // Control register: normal power, internal reference, gain selected.
    uint16_t ctrl = DAC_GAIN2 ? 0x0800 : 0x0000;

    return dacWrite16(0x40, ctrl);
}


void setPumpVolts(float v)
{
    if (v < 0.0f)
    {
        v = 0.0f;
    }

    if (v > VMAX)
    {
        v = VMAX;
    }

    if (v > dacFullScale)
    {
        v = dacFullScale;
    }

    pumpV = v;

    if (!dacAddr)
    {
        return;
    }

    uint32_t code = (uint32_t)(v / dacFullScale * 65535.0f + 0.5f);

    if (code > 65535UL)
    {
        code = 65535UL;
    }

    // Write DAC register + update output.
    dacWrite16(0x30, (uint16_t)code);
}


void pumpOff()
{
    setPumpVolts(0.0f);
}


// =====================================================================================
// HARDWARE CHECK
// =====================================================================================

void printCheck()
{
    // ADC121C021 health is checked with a real conversion-register read,
    // not a generic address scan.
    bool p1 = adcAlive(ADDR_P1);
    bool p2 = adcAlive(ADDR_P2);

    if (!dacAddr)
    {
        dacInit();
    }

    bool dac = dacAddr && i2cPresent(dacAddr);

    Serial.print("#CHECK,p1=");
    Serial.print(p1 ? "OK" : "MISSING");

    Serial.print(",p2=");
    Serial.print(p2 ? "OK" : "MISSING");

    Serial.print(",dac=");
    if (dac)
    {
        Serial.print("OK@0x");
        Serial.print(dacAddr, HEX);
    }
    else
    {
        Serial.print("MISSING");
    }

    Serial.print(",vmax=");
    Serial.print(VMAX, 2);

    Serial.print(",system=");
    Serial.println((p1 && p2 && dac) ? "READY" : "FAULT");
}


// =====================================================================================
// EXECUTE ONE PUMP SAMPLE
// =====================================================================================

void executeStep(float requestedV)
{
    // 1. Apply voltage from Python.
    setPumpVolts(requestedV);

    // 2. Read inlet and outlet pressure ADCs.
    float r1 = adcAvg(ADDR_P1);
    float r2 = adcAvg(ADDR_P2);

    seq++;

    // 3. Return result to Python.
    // Format: D,time_ms,seq,p1raw,p1ok,p2raw,p2ok,pumpV
    Serial.print("D,");
    Serial.print(millis());
    Serial.print(",");
    Serial.print(seq);
    Serial.print(",");
    Serial.print(r1, 1);
    Serial.print(",");
    Serial.print(r1 >= 0.0f ? 1 : 0);
    Serial.print(",");
    Serial.print(r2, 1);
    Serial.print(",");
    Serial.print(r2 >= 0.0f ? 1 : 0);
    Serial.print(",");
    Serial.println(pumpV, 4);
}


// =====================================================================================
// COMMAND HANDLER
// =====================================================================================

void handleCommand(String cmd)
{
    cmd.trim();

    if (cmd.length() == 0)
    {
        return;
    }

    String upper = cmd;
    upper.toUpperCase();

    // Only valid commands refresh the failsafe timer.
    // Garbage or rejected commands must not keep the pump running.

    if (upper == "PING")
    {
        lastHostCommandMs = millis();
        Serial.println("#PONG");
        return;
    }

    if (upper == "CHECK")
    {
        lastHostCommandMs = millis();
        printCheck();
        return;
    }

    if (upper == "PUMP OFF")
    {
        lastHostCommandMs = millis();
        pumpOff();
        Serial.println("#OK,PUMP OFF");
        return;
    }

    if (upper.startsWith("STEP "))
    {
        String valueText = cmd.substring(5);
        valueText.trim();

        if (valueText.length() == 0)
        {
            Serial.println("#ERR,usage: STEP <volts>");
            return;
        }

        float v = valueText.toFloat();

        if (v < 0.0f || v > VMAX)
        {
            Serial.println("#ERR,STEP voltage outside 0..VMAX");
            return;
        }

        if (!dacAddr)
        {
            Serial.println("#ERR,pump DAC not found");
            return;
        }

        lastHostCommandMs = millis();
        executeStep(v);
        return;
    }

    Serial.print("#ERR,unknown command: ");
    Serial.println(cmd);
}


// =====================================================================================
// SETUP
// =====================================================================================

void setup()
{
    Serial.begin(115200);

    // Wait briefly for USB.
    while (!Serial && millis() < 4000)
    {
        delay(10);
    }

    Wire.begin();
    Wire.setClock(100000);

    delay(500);

    dacInit();

    // Safety: pump always starts OFF.
    pumpOff();

    lastHostCommandMs = millis();

    Serial.println("#READY,Phantom_HW_EXECUTOR");

    printCheck();
}


// =====================================================================================
// LOOP
// =====================================================================================

void loop()
{
    // USB disconnect safety.
    bool host = (bool)Serial;

    if (hostWasConnected && !host)
    {
        pumpOff();
    }

    hostWasConnected = host;

    // Host-loss failsafe.
    if (pumpV > 0.0f && (millis() - lastHostCommandMs) > HOST_FAILSAFE_MS)
    {
        pumpOff();
    }

    // Receive Python commands.
    while (Serial.available())
    {
        char c = (char)Serial.read();

        if (c == '\n' || c == '\r')
        {
            if (rxLine.length() > 0)
            {
                handleCommand(rxLine);
                rxLine = "";
            }
        }
        else if (rxLine.length() < 80)
        {
            rxLine += c;
        }
    }
}
