/*
 * =================================================================================
 * IMPLEMENTASI DRIVER LAYAR JDL7158 / JDL0432B11 (IC TM1621) - STM32 CUBEIDE (C)
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

/* Microsecond Delay Helper untuk STM32 (Akurat untuk TM1621 Timing) */
static void JDL7158_DelayUs(uint32_t us) {
    uint32_t count = us * 20U;
    while (count--) {
        __asm__ volatile ("nop");
    }
}

/* GPIO Helper Functions */
static inline void writeCS(JDL7158_HandleTypeDef* lcd, bool state) {
    if (lcd->CS_Port) {
        HAL_GPIO_WritePin(lcd->CS_Port, lcd->pin_cs, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static inline void writeWR(JDL7158_HandleTypeDef* lcd, bool state) {
    if (lcd->WR_Port) {
        HAL_GPIO_WritePin(lcd->WR_Port, lcd->pin_wr, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static inline void writeDATA(JDL7158_HandleTypeDef* lcd, bool state) {
    if (lcd->DATA_Port) {
        HAL_GPIO_WritePin(lcd->DATA_Port, lcd->pin_data, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static inline void writeBLK(JDL7158_HandleTypeDef* lcd, bool state) {
    if (lcd->BLK_Port && lcd->pin_blk != 0) {
        HAL_GPIO_WritePin(lcd->BLK_Port, lcd->pin_blk, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/* Kirim Bit Serial ke TM1621 */
static void sendBits(JDL7158_HandleTypeDef* lcd, uint32_t data, uint8_t count) {
    for (int i = count - 1; i >= 0; i--) {
        writeWR(lcd, false);
        JDL7158_DelayUs(10);
        writeDATA(lcd, (data >> i) & 0x01);
        JDL7158_DelayUs(10);
        writeWR(lcd, true);
        JDL7158_DelayUs(10);
    }
}

/* Kirim Command Mode ke TM1621 */
static void sendCommand(JDL7158_HandleTypeDef* lcd, uint8_t cmd) {
    writeCS(lcd, false);
    JDL7158_DelayUs(10);
    sendBits(lcd, TM1621_ID_CMD, 3);
    sendBits(lcd, cmd, 8);
    sendBits(lcd, 0, 1);
    writeCS(lcd, true);
    JDL7158_DelayUs(10);
}

/* Kirim Seluruh Buffer RAM (32 nibble) ke TM1621 */
static void updateAllRAM(JDL7158_HandleTypeDef* lcd) {
    writeCS(lcd, false);
    JDL7158_DelayUs(10);
    sendBits(lcd, TM1621_ID_WRITE, 3);
    sendBits(lcd, 0x00, 6);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 4; j++) {
            writeWR(lcd, false);
            JDL7158_DelayUs(10);
            writeDATA(lcd, (lcd->lcdBuffer[i] >> j) & 0x01);
            JDL7158_DelayUs(10);
            writeWR(lcd, true);
            JDL7158_DelayUs(10);
        }
    }
    writeCS(lcd, true);
    JDL7158_DelayUs(10);
}

void JDL7158_Init(JDL7158_HandleTypeDef* lcd,
                  GPIO_TypeDef* csPort, uint16_t csPin,
                  GPIO_TypeDef* wrPort, uint16_t wrPin,
                  GPIO_TypeDef* dataPort, uint16_t dataPin,
                  GPIO_TypeDef* blkPort, uint16_t blkPin)
{
    if (!lcd) return;

    lcd->CS_Port   = csPort;
    lcd->pin_cs    = csPin;
    lcd->WR_Port   = wrPort;
    lcd->pin_wr    = wrPin;
    lcd->DATA_Port = dataPort;
    lcd->pin_data  = dataPin;
    lcd->BLK_Port  = blkPort;
    lcd->pin_blk   = blkPin;

    lcd->valTegangan  = 0.0f;
    lcd->valArus      = 0.0f;
    lcd->valSoC       = 0;
    lcd->valSuhu      = 0.0f;
    lcd->valEnergi    = 0;
    lcd->valErrCode   = 0;
    lcd->blkDutyCycle = 0;

    memset(lcd->lcdBuffer, 0, sizeof(lcd->lcdBuffer));

    writeCS(lcd, true);
    writeWR(lcd, true);
    writeDATA(lcd, true);
    writeBLK(lcd, false); // Active LOW -> Low = Backlight ON

    HAL_Delay(100);

    // Pulse Reset CS
    writeCS(lcd, false);
    HAL_Delay(10);
    writeCS(lcd, true);
    HAL_Delay(20);

    // Sequence Inisialisasi IC TM1621 (Wajib dengan delay 10ms antar command agar osilator stabil)
    sendCommand(lcd, TM1621_CMD_SYS_DIS); HAL_Delay(10);
    sendCommand(lcd, TM1621_CMD_SYS_EN);  HAL_Delay(10);
    sendCommand(lcd, TM1621_CMD_RC256K);  HAL_Delay(10);
    sendCommand(lcd, TM1621_CMD_BIAS);    HAL_Delay(10);
    sendCommand(lcd, TM1621_CMD_LCD_ON);  HAL_Delay(10);

    JDL7158_Clear(lcd);
}

void JDL7158_Clear(JDL7158_HandleTypeDef* lcd) {
    if (!lcd) return;
    memset(lcd->lcdBuffer, 0, sizeof(lcd->lcdBuffer));
    updateAllRAM(lcd);
}

void JDL7158_SetBacklight(JDL7158_HandleTypeDef* lcd, uint8_t state) {
    if (!lcd) return;
    // state: 0 = MATI, 1 = NYALA (Active LOW di hardware: LOW = Backlight ON)
    writeBLK(lcd, state ? false : true);
}

void JDL7158_SetTegangan(JDL7158_HandleTypeDef* lcd, float volt)       { if(lcd) lcd->valTegangan = volt; }
void JDL7158_SetArus(JDL7158_HandleTypeDef* lcd, float ampere)         { if(lcd) lcd->valArus = ampere; }
void JDL7158_SetSoC(JDL7158_HandleTypeDef* lcd, uint8_t percent)       { if(lcd) lcd->valSoC = percent; }
void JDL7158_SetSuhu(JDL7158_HandleTypeDef* lcd, float tempC)          { if(lcd) lcd->valSuhu = tempC; }
void JDL7158_SetEnergi(JDL7158_HandleTypeDef* lcd, uint16_t kwh)       { if(lcd) lcd->valEnergi = kwh; }
void JDL7158_SetErrorCode(JDL7158_HandleTypeDef* lcd, uint8_t errCode) { if(lcd) lcd->valErrCode = errCode; }

void JDL7158_SetSymbol(JDL7158_HandleTypeDef* lcd, uint8_t pin, uint8_t bitPos, bool on) {
    if (!lcd) return;
    uint8_t addr = pin - 5;
    if (addr < 32) {
        if (on) {
            lcd->lcdBuffer[addr] |= (1 << bitPos);
        } else {
            lcd->lcdBuffer[addr] &= ~(1 << bitPos);
        }
    }
}

void JDL7158_SetDigit(JDL7158_HandleTypeDef* lcd, uint8_t pinRight, uint8_t pinLeft, uint8_t val) {
    if (!lcd || val > 10) return;
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
        lcd->lcdBuffer[addrR] = (d << 3) | (c << 2) | (b << 1) | (a << 0);
    } else {
        lcd->lcdBuffer[addrR] = (a << 3) | (b << 2) | (c << 1) | (d << 0);
    }

    if (pinLeft == 32 || pinLeft == 30 || pinLeft == 22 || pinLeft == 26 || pinLeft == 24) {
        uint8_t symBit = lcd->lcdBuffer[addrL] & 0x01;
        lcd->lcdBuffer[addrL] = (f << 3) | (g << 2) | (e << 1) | symBit;
    }
    else if (pinLeft == 28 || pinLeft == 20 || pinLeft == 18 || pinLeft == 33 || pinLeft == 35) {
        uint8_t symBit = lcd->lcdBuffer[addrL] & 0x08;
        lcd->lcdBuffer[addrL] = symBit | (f << 2) | (g << 1) | (e << 0);
    }
    else if (pinLeft == 7 || pinLeft == 9 || pinLeft == 11 || pinLeft == 13) {
        uint8_t symBit = lcd->lcdBuffer[addrL] & 0x01;
        lcd->lcdBuffer[addrL] = (e << 3) | (g << 2) | (f << 1) | symBit;
    }
    else if (pinLeft == 5 || pinLeft == 15) {
        uint8_t symBit = lcd->lcdBuffer[addrL] & 0x08;
        lcd->lcdBuffer[addrL] = symBit | (e << 2) | (g << 1) | (f << 0);
    }
}

void JDL7158_ClearDigit(JDL7158_HandleTypeDef* lcd, uint8_t pinRight, uint8_t pinLeft) {
    if (!lcd) return;
    uint8_t addrR = pinRight - 5;
    uint8_t addrL = pinLeft - 5;

    lcd->lcdBuffer[addrR] = 0x00;

    if (pinLeft == 32 || pinLeft == 30 || pinLeft == 22 || pinLeft == 26 || pinLeft == 24 ||
        pinLeft == 7 || pinLeft == 9 || pinLeft == 11 || pinLeft == 13) {
        lcd->lcdBuffer[addrL] &= 0x01;
    } else {
        lcd->lcdBuffer[addrL] &= 0x08;
    }
}

void JDL7158_Update(JDL7158_HandleTypeDef* lcd) {
    if (!lcd) return;

    // Simbol Dekoratif S1..S15 (Kecuali S5)
    JDL7158_SetSymbol(lcd, 33, 3, true);  // S1  (Garis Atas)
    JDL7158_SetSymbol(lcd, 28, 3, true);  // S2  (Garis V Tegangan)
    JDL7158_SetSymbol(lcd, 20, 3, true);  // S3  (Garis Energi)
    JDL7158_SetSymbol(lcd, 18, 3, true);  // S4  (Garis kWh Energi)
    JDL7158_SetSymbol(lcd, 5,  3, true);  // S6  (Garis Arus)
    JDL7158_SetSymbol(lcd, 11, 0, true);  // S7  (Garis Suhu)
    JDL7158_SetSymbol(lcd, 15, 3, true);  // S8  (Garis Bawah)
    JDL7158_SetSymbol(lcd, 32, 0, true);  // S9  (Lengkung Kiri Baterai)
    JDL7158_SetSymbol(lcd, 30, 0, true);  // S10 (Lengkung Atas Baterai)
    JDL7158_SetSymbol(lcd, 22, 0, true);  // S11 (Lengkung Kanan Baterai)
    JDL7158_SetSymbol(lcd, 9,  0, true);  // S12 (Lengkung Bawah Baterai)
    JDL7158_SetSymbol(lcd, 7,  0, true);  // S13 (Garis A Arus)
    JDL7158_SetSymbol(lcd, 24, 0, true);  // S14 (Simbol % Baterai)
    JDL7158_SetSymbol(lcd, 13, 0, true);  // S15 (Simbol °C & Titik Desimal Suhu)

    // 1. Tegangan (Digit 1, 2, 3)
    uint16_t vInt = (uint16_t)(lcd->valTegangan * 10.0f);
    JDL7158_SetDigit(lcd, 31, 32, (vInt / 100) % 10);
    JDL7158_SetDigit(lcd, 29, 30, (vInt / 10) % 10);
    JDL7158_SetDigit(lcd, 27, 28, vInt % 10);

    // 2. Energi (Digit 4, 5, 6)
    uint16_t kwhInt = lcd->valEnergi;
    JDL7158_SetDigit(lcd, 21, 22, (kwhInt / 100) % 10);
    JDL7158_SetDigit(lcd, 19, 20, (kwhInt / 10) % 10);
    JDL7158_SetDigit(lcd, 17, 18, kwhInt % 10);

    // 3. Error Code (Digit 7, 8 & S5 Pentung !)
    if (lcd->valErrCode > 0) {
        JDL7158_SetSymbol(lcd, 35, 3, true);                    // S5 NYALA (Tanda Pentung !)
        JDL7158_SetDigit(lcd, 34, 33, 10);                      // Digit 7 = 'E'
        JDL7158_SetDigit(lcd, 36, 35, lcd->valErrCode % 10);    // Digit 8 = Kode Error
    } else {
        JDL7158_SetSymbol(lcd, 35, 3, false);                   // S5 MATI
        JDL7158_ClearDigit(lcd, 34, 33);                        // Digit 7 MATI
        JDL7158_ClearDigit(lcd, 36, 35);                        // Digit 8 MATI
    }

    // 4. Baterai SoC % (Digit 9, 10, 11)
    uint8_t socInt = lcd->valSoC;
    if (socInt >= 100) {
        JDL7158_SetSymbol(lcd, 26, 0, true);
    } else {
        JDL7158_SetSymbol(lcd, 26, 0, false);
    }
    JDL7158_SetDigit(lcd, 25, 26, (socInt / 10) % 10);
    JDL7158_SetDigit(lcd, 23, 24, socInt % 10);

    // 5. Arus (Digit 12, 13, 14)
    uint16_t aInt = (uint16_t)(lcd->valArus * 10.0f);
    JDL7158_SetDigit(lcd, 6, 5, (aInt / 100) % 10);
    JDL7158_SetDigit(lcd, 8, 7, (aInt / 10) % 10);
    JDL7158_SetDigit(lcd, 10, 9, aInt % 10);

    // 6. Suhu (Digit 15, 16, 17)
    uint16_t suhuInt = (uint16_t)(lcd->valSuhu * 10.0f);
    JDL7158_SetDigit(lcd, 12, 11, (suhuInt / 100) % 10);
    JDL7158_SetDigit(lcd, 14, 13, (suhuInt / 10) % 10);
    JDL7158_SetDigit(lcd, 16, 15, suhuInt % 10);

    updateAllRAM(lcd);
}
