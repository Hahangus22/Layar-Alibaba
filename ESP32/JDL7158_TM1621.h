/*
 * =================================================================================
 * LIBRARY DRIVER LAYAR JDL7158 / JDL0432B11 (IC TM1621)
 * PERANGKAT MONITORING BATERAI LiFePO4 / BMS
 * =================================================================================
 * Kompatibel dengan:
 * - Arduino Framework (ESP32, ESP32-C3, ESP32-S3, ESP8266, STM32 Arduino)
 * - STM32CubeIDE / STM32 HAL C & C++
 * =================================================================================
 */

#ifndef JDL7158_TM1621_H
#define JDL7158_TM1621_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#if defined(ARDUINO)
  #include <Arduino.h>
#endif

// Perintah TM1621
#define TM1621_ID_CMD       0b100  
#define TM1621_ID_WRITE     0b101  
#define TM1621_CMD_SYS_DIS  0x00   
#define TM1621_CMD_SYS_EN   0x01   
#define TM1621_CMD_LCD_OFF  0x02   
#define TM1621_CMD_LCD_ON   0x03   
#define TM1621_CMD_RC256K   0x18   
#define TM1621_CMD_BIAS     0x29   // 1/3 Bias, 4 Commons

#ifdef __cplusplus
class JDL7158_TM1621 {
public:
  /**
   * @brief Konstruktor Driver JDL7158 / TM1621
   * @param pinCS   Pin GPIO Chip Select (Active LOW)
   * @param pinWR   Pin GPIO Write Clock
   * @param pinDATA Pin GPIO Data Serial Input
   * @param pinBLK  Pin GPIO Backlight PWM (Active LOW, opsional, default = -1)
   */
  JDL7158_TM1621(uint8_t pinCS, uint8_t pinWR, uint8_t pinDATA, int8_t pinBLK = -1);

  /**
   * @brief Inisialisasi awal TM1621 & Setup GPIO
   */
  void begin();

  /**
   * @brief Hapus seluruh tampilan RAM LCD (Clear All)
   */
  void clear();

  /**
   * @brief Render ulang data ke buffer RAM & kirim ke layar TM1621
   */
  void update();

  /**
   * @brief Set Kecerahan Backlight PWM (Active LOW)
   * @param duty 0 = 100% Terang, 255 = Mati Total
   */
  void setBacklight(uint8_t duty);

  // --- API Setter Monitoring BMS ---
  void setTegangan(float volt);
  void setArus(float ampere);
  void setSoC(uint8_t percent);
  void setSuhu(float tempC);
  void setEnergi(uint16_t kwh);
  
  /**
   * @brief Set Error Code
   * @param errCode 0 = Normal (S5 Pentung ! MATI), 1..8 = Error E-0x (S5 Pentung ! NYALA)
   */
  void setErrorCode(uint8_t errCode);

  // --- Low-Level Functions ---
  void setSymbol(uint8_t pin, uint8_t bitPos, bool on);
  void setDigit(uint8_t pinRight, uint8_t pinLeft, uint8_t val);
  void clearDigit(uint8_t pinRight, uint8_t pinLeft);

  // Access Buffer RAM
  uint8_t* getBuffer() { return _lcdBuffer; }

private:
  uint8_t _pinCS;
  uint8_t _pinWR;
  uint8_t _pinDATA;
  int8_t  _pinBLK;

  uint8_t _lcdBuffer[32];

  float    _valTegangan;
  float    _valArus;
  uint8_t  _valSoC;
  float    _valSuhu;
  uint16_t _valEnergi;
  uint8_t  _valErrCode;
  uint8_t  _blkDutyCycle;

  void sendBits(uint32_t data, uint8_t count);
  void sendCommand(uint8_t cmd);
  void updateAllRAM();
  void writeGPIO(uint8_t pin, bool val);
  void delayUs(uint32_t us);
};
#endif

// =================================================================================
// C API WRAPPER (Untuk STM32CubeIDE / Standard C Project)
// =================================================================================
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t pinCS;
  uint8_t pinWR;
  uint8_t pinDATA;
  int8_t  pinBLK;
  uint8_t lcdBuffer[32];
  float   valTegangan;
  float   valArus;
  uint8_t valSoC;
  float   valSuhu;
  uint16_t valEnergi;
  uint8_t valErrCode;
  uint8_t blkDutyCycle;
} JDL7158_Handler_t;

void JDL7158_C_Init(JDL7158_Handler_t* handle, uint8_t cs, uint8_t wr, uint8_t data, int8_t blk);
void JDL7158_C_Update(JDL7158_Handler_t* handle);
void JDL7158_C_SetTegangan(JDL7158_Handler_t* handle, float volt);
void JDL7158_C_SetArus(JDL7158_Handler_t* handle, float ampere);
void JDL7158_C_SetSoC(JDL7158_Handler_t* handle, uint8_t percent);
void JDL7158_C_SetSuhu(JDL7158_Handler_t* handle, float tempC);
void JDL7158_C_SetEnergi(JDL7158_Handler_t* handle, uint16_t kwh);
void JDL7158_C_SetErrorCode(JDL7158_Handler_t* handle, uint8_t errCode);
void JDL7158_C_SetBacklight(JDL7158_Handler_t* handle, uint8_t duty);

#ifdef __cplusplus
}
#endif

#endif // JDL7158_TM1621_H
