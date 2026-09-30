/*
 * =================================================================================
 * IMPLEMENTASI LIBRARY DRIVER LAYAR JDL7158 / JDL0432B11 (IC TM1621)
 * PERANGKAT MONITORING BATERAI LiFePO4 / BMS
 * =================================================================================
 */

#include "JDL7158_TM1621.h"

// 7-Segment Bitmask Pattern: A, B, C, D, E, F, G
static const uint8_t segPattern[11] = {
  0b00111111, // 0: A,B,C,D,E,F
  0b00000110, // 1: B,C
  0b01011011, // 2: A,B,D,E,G
  0b01001111, // 3: A,B,C,D,G
  0b01100110, // 4: B,C,F,G
  0b01101101, // 5: A,C,D,F,G
  0b01111101, // 6: A,C,D,E,F,G
  0b00000111, // 7: A,B,C
  0b01111111, // 8: A,B,C,D,E,F,G
  0b01101111, // 9: A,B,C,D,F,G
  0b01111001  // 10 ('E'): A,D,E,F,G
};

JDL7158_TM1621::JDL7158_TM1621(uint8_t pinCS, uint8_t pinWR, uint8_t pinDATA, int8_t pinBLK)
  : _pinCS(pinCS), _pinWR(pinWR), _pinDATA(pinDATA), _pinBLK(pinBLK),
    _valTegangan(0.0f), _valArus(0.0f), _valSoC(0), _valSuhu(0.0f),
    _valEnergi(0), _valErrCode(0), _blkDutyCycle(0)
{
  memset(_lcdBuffer, 0, sizeof(_lcdBuffer));
}

void JDL7158_TM1621::writeGPIO(uint8_t pin, bool val) {
#if defined(ARDUINO)
  digitalWrite(pin, val ? HIGH : LOW);
#endif
}

void JDL7158_TM1621::delayUs(uint32_t us) {
#if defined(ARDUINO)
  delayMicroseconds(us);
#else
  for (volatile uint32_t i = 0; i < us * 10; i++) { __asm__("nop"); }
#endif
}

void JDL7158_TM1621::sendBits(uint32_t data, uint8_t count) {
  for (int i = count - 1; i >= 0; i--) {
    writeGPIO(_pinWR, false);
    delayUs(10);
    writeGPIO(_pinDATA, (data >> i) & 0x01);
    delayUs(10);
    writeGPIO(_pinWR, true);
    delayUs(10);
  }
}

void JDL7158_TM1621::sendCommand(uint8_t cmd) {
  writeGPIO(_pinCS, false);
  delayUs(10);
  sendBits(TM1621_ID_CMD, 3);
  sendBits(cmd, 8);
  sendBits(0, 1);
  writeGPIO(_pinCS, true);
  delayUs(10);
}

void JDL7158_TM1621::updateAllRAM() {
  writeGPIO(_pinCS, false);
  delayUs(10);
  sendBits(TM1621_ID_WRITE, 3);
  sendBits(0x00, 6);
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 4; j++) {
      writeGPIO(_pinWR, false);
      delayUs(10);
      writeGPIO(_pinDATA, (_lcdBuffer[i] >> j) & 0x01);
      delayUs(10);
      writeGPIO(_pinWR, true);
      delayUs(10);
    }
  }
  writeGPIO(_pinCS, true);
  delayUs(10);
}

void JDL7158_TM1621::clear() {
  memset(_lcdBuffer, 0, sizeof(_lcdBuffer));
  updateAllRAM();
}

void JDL7158_TM1621::begin() {
#if defined(ARDUINO)
  pinMode(_pinCS, OUTPUT);
  pinMode(_pinWR, OUTPUT);
  pinMode(_pinDATA, OUTPUT);
  if (_pinBLK >= 0) {
    pinMode(_pinBLK, OUTPUT);
    setBacklight(0); // 100% Brightness default
  }
#endif

  writeGPIO(_pinCS, true);
  writeGPIO(_pinWR, true);
  writeGPIO(_pinDATA, true);
#if defined(ARDUINO)
  delay(100);
#endif

  // Pulse Reset CS
  writeGPIO(_pinCS, false);
#if defined(ARDUINO)
  delay(10);
#endif
  writeGPIO(_pinCS, true);
#if defined(ARDUINO)
  delay(20);
#endif

  // Sequence Command Trigger TM1621
  sendCommand(TM1621_CMD_SYS_DIS);
  sendCommand(TM1621_CMD_SYS_EN);
  sendCommand(TM1621_CMD_RC256K);
  sendCommand(TM1621_CMD_BIAS);
  sendCommand(TM1621_CMD_LCD_ON);

  clear();
}

void JDL7158_TM1621::setBacklight(uint8_t duty) {
  _blkDutyCycle = duty;
  if (_pinBLK >= 0) {
#if defined(ARDUINO)
    analogWrite(_pinBLK, _blkDutyCycle);
#endif
  }
}

void JDL7158_TM1621::setTegangan(float volt)       { _valTegangan = volt; }
void JDL7158_TM1621::setArus(float ampere)         { _valArus = ampere; }
void JDL7158_TM1621::setSoC(uint8_t percent)       { _valSoC = percent; }
void JDL7158_TM1621::setSuhu(float tempC)          { _valSuhu = tempC; }
void JDL7158_TM1621::setEnergi(uint16_t kwh)       { _valEnergi = kwh; }
void JDL7158_TM1621::setErrorCode(uint8_t errCode) { _valErrCode = errCode; }

void JDL7158_TM1621::setSymbol(uint8_t pin, uint8_t bitPos, bool on) {
  uint8_t addr = pin - 5;
  if (addr < 32) {
    if (on) {
      _lcdBuffer[addr] |= (1 << bitPos);
    } else {
      _lcdBuffer[addr] &= ~(1 << bitPos);
    }
  }
}

void JDL7158_TM1621::setDigit(uint8_t pinRight, uint8_t pinLeft, uint8_t val) {
  if (val > 10) return;
  uint8_t pat = segPattern[val];

  bool a = (pat >> 0) & 1;
  bool b = (pat >> 1) & 1;
  bool c = (pat >> 2) & 1;
  bool d = (pat >> 3) & 1;
  bool e = (pat >> 4) & 1;
  bool f = (pat >> 5) & 1;
  bool g = (pat >> 6) & 1;

  uint8_t addrR = pinRight - 5;
  uint8_t addrL = pinLeft - 5;

  if (pinRight == 6 || pinRight == 8 || pinRight == 10 || pinRight == 12 || pinRight == 14 || pinRight == 16) {
    _lcdBuffer[addrR] = (d << 3) | (c << 2) | (b << 1) | (a << 0);
  } else {
    _lcdBuffer[addrR] = (a << 3) | (b << 2) | (c << 1) | (d << 0);
  }

  if (pinLeft == 32 || pinLeft == 30 || pinLeft == 22 || pinLeft == 26 || pinLeft == 24) {
    uint8_t symBit = _lcdBuffer[addrL] & 0x01;
    _lcdBuffer[addrL] = (f << 3) | (g << 2) | (e << 1) | symBit;
  }
  else if (pinLeft == 28 || pinLeft == 20 || pinLeft == 18 || pinLeft == 33 || pinLeft == 35) {
    uint8_t symBit = _lcdBuffer[addrL] & 0x08;
    _lcdBuffer[addrL] = symBit | (f << 2) | (g << 1) | (e << 0);
  }
  else if (pinLeft == 7 || pinLeft == 9 || pinLeft == 11 || pinLeft == 13) {
    uint8_t symBit = _lcdBuffer[addrL] & 0x01;
    _lcdBuffer[addrL] = (e << 3) | (g << 2) | (f << 1) | symBit;
  }
  else if (pinLeft == 5 || pinLeft == 15) {
    uint8_t symBit = _lcdBuffer[addrL] & 0x08;
    _lcdBuffer[addrL] = symBit | (e << 2) | (g << 1) | (f << 0);
  }
}

void JDL7158_TM1621::clearDigit(uint8_t pinRight, uint8_t pinLeft) {
  uint8_t addrR = pinRight - 5;
  uint8_t addrL = pinLeft - 5;

  _lcdBuffer[addrR] = 0x00;

  if (pinLeft == 32 || pinLeft == 30 || pinLeft == 22 || pinLeft == 26 || pinLeft == 24 ||
      pinLeft == 7 || pinLeft == 9 || pinLeft == 11 || pinLeft == 13) {
    _lcdBuffer[addrL] &= 0x01;
  } else {
    _lcdBuffer[addrL] &= 0x08;
  }
}

void JDL7158_TM1621::update() {
  // Simbol Dekoratif S1..S15 (Kecuali S5)
  setSymbol(33, 3, true);  // S1  (Garis Atas)
  setSymbol(28, 3, true);  // S2  (Garis V Tegangan)
  setSymbol(20, 3, true);  // S3  (Garis Energi)
  setSymbol(18, 3, true);  // S4  (Garis kWh Energi)
  setSymbol(5,  3, true);  // S6  (Garis Arus)
  setSymbol(11, 0, true);  // S7  (Garis Suhu)
  setSymbol(15, 3, true);  // S8  (Garis Bawah)
  setSymbol(32, 0, true);  // S9  (Lengkung Kiri Baterai)
  setSymbol(30, 0, true);  // S10 (Lengkung Atas Baterai)
  setSymbol(22, 0, true);  // S11 (Lengkung Kanan Baterai)
  setSymbol(9,  0, true);  // S12 (Lengkung Bawah Baterai)
  setSymbol(7,  0, true);  // S13 (Garis A Arus)
  setSymbol(24, 0, true);  // S14 (Simbol % Baterai)
  setSymbol(13, 0, true);  // S15 (Simbol °C & Titik Desimal Suhu)

  // 1. Tegangan (Digit 1, 2, 3)
  uint16_t vInt = (uint16_t)(_valTegangan * 10);
  setDigit(31, 32, (vInt / 100) % 10);
  setDigit(29, 30, (vInt / 10) % 10);
  setDigit(27, 28, vInt % 10);

  // 2. Energi (Digit 4, 5, 6)
  uint16_t kwhInt = _valEnergi;
  setDigit(21, 22, (kwhInt / 100) % 10);
  setDigit(19, 20, (kwhInt / 10) % 10);
  setDigit(17, 18, kwhInt % 10);

  // 3. Error Code (Digit 7, 8 & S5 Pentung !)
  if (_valErrCode > 0) {
    setSymbol(35, 3, true);              // S5 NYALA (Tanda Pentung !)
    setDigit(34, 33, 10);                // Digit 7 = 'E'
    setDigit(36, 35, _valErrCode % 10);  // Digit 8 = Kode Error
  } else {
    setSymbol(35, 3, false);             // S5 MATI
    clearDigit(34, 33);                  // Digit 7 MATI
    clearDigit(36, 35);                  // Digit 8 MATI
  }

  // 4. Baterai SoC % (Digit 9, 10, 11)
  uint8_t socInt = _valSoC;
  if (socInt >= 100) {
    setSymbol(26, 0, true);
  } else {
    setSymbol(26, 0, false);
  }
  setDigit(25, 26, (socInt / 10) % 10);
  setDigit(23, 24, socInt % 10);

  // 5. Arus (Digit 12, 13, 14)
  uint16_t aInt = (uint16_t)(_valArus * 10);
  setDigit(6, 5, (aInt / 100) % 10);
  setDigit(8, 7, (aInt / 10) % 10);
  setDigit(10, 9, aInt % 10);

  // 6. Suhu (Digit 15, 16, 17)
  uint16_t suhuInt = (uint16_t)(_valSuhu * 10);
  setDigit(12, 11, (suhuInt / 100) % 10);
  setDigit(14, 13, (suhuInt / 10) % 10);
  setDigit(16, 15, suhuInt % 10);

  updateAllRAM();
}

// =================================================================================
// C API WRAPPER IMPLEMENTATION
// =================================================================================
extern "C" {

void JDL7158_C_Init(JDL7158_Handler_t* handle, uint8_t cs, uint8_t wr, uint8_t data, int8_t blk) {
  if (!handle) return;
  handle->pinCS = cs;
  handle->pinWR = wr;
  handle->pinDATA = data;
  handle->pinBLK = blk;
  handle->valTegangan = 0.0f;
  handle->valArus = 0.0f;
  handle->valSoC = 0;
  handle->valSuhu = 0.0f;
  handle->valEnergi = 0;
  handle->valErrCode = 0;
  handle->blkDutyCycle = 0;

  JDL7158_TM1621 driver(cs, wr, data, blk);
  driver.begin();
}

void JDL7158_C_Update(JDL7158_Handler_t* handle) {
  if (!handle) return;
  JDL7158_TM1621 driver(handle->pinCS, handle->pinWR, handle->pinDATA, handle->pinBLK);
  driver.setTegangan(handle->valTegangan);
  driver.setArus(handle->valArus);
  driver.setSoC(handle->valSoC);
  driver.setSuhu(handle->valSuhu);
  driver.setEnergi(handle->valEnergi);
  driver.setErrorCode(handle->valErrCode);
  driver.update();
}

void JDL7158_C_SetTegangan(JDL7158_Handler_t* handle, float volt)       { if(handle) handle->valTegangan = volt; }
void JDL7158_C_SetArus(JDL7158_Handler_t* handle, float ampere)         { if(handle) handle->valArus = ampere; }
void JDL7158_C_SetSoC(JDL7158_Handler_t* handle, uint8_t percent)       { if(handle) handle->valSoC = percent; }
void JDL7158_C_SetSuhu(JDL7158_Handler_t* handle, float tempC)          { if(handle) handle->valSuhu = tempC; }
void JDL7158_C_SetEnergi(JDL7158_Handler_t* handle, uint16_t kwh)       { if(handle) handle->valEnergi = kwh; }
void JDL7158_C_SetErrorCode(JDL7158_Handler_t* handle, uint8_t errCode) { if(handle) handle->valErrCode = errCode; }
void JDL7158_C_SetBacklight(JDL7158_Handler_t* handle, uint8_t duty)    { if(handle) handle->blkDutyCycle = duty; }

}
