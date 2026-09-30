/*
 * =================================================================================
 * DRIVER LAYAR JDL7158 / JDL0432B11 (IC TM1621) - STM32 CUBEIDE (BAHASA C)
 * PERANGKAT MONITORING BATERAI LiFePO4 / BMS
 * =================================================================================
 * Didesain khusus untuk STM32CubeIDE (HAL C Driver)
 * Support STM32 Series: STM32F1, STM32F4, STM32G0, STM32F0, STM32G4, dll.
 * =================================================================================
 */

#ifndef JDL7158_TM1621_H
#define JDL7158_TM1621_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Otomatis include header STM32 HAL dari STM32CubeIDE
#if __has_include("main.h")
  #include "main.h"
#elif defined(STM32F1xx)
  #include "stm32f1xx_hal.h"
#elif defined(STM32F4xx)
  #include "stm32f4xx_hal.h"
#elif defined(STM32G0xx)
  #include "stm32g0xx_hal.h"
#elif defined(STM32F0xx)
  #include "stm32f0xx_hal.h"
#elif defined(STM32F3xx)
  #include "stm32f3xx_hal.h"
#elif defined(STM32L4xx)
  #include "stm32l4xx_hal.h"
#elif defined(STM32G4xx)
  #include "stm32g4xx_hal.h"
#elif defined(STM32H7xx)
  #include "stm32h7xx_hal.h"
#else
  // Fallback definisi GPIO jika di-compile tanpa HAL secara independen
  typedef struct {
    uint32_t dummy;
  } GPIO_TypeDef;
  #define GPIO_PIN_SET 1
  #define GPIO_PIN_RESET 0
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

/**
 * @brief Struktur Handle LCD JDL7158 (TM1621) STM32 HAL
 */
typedef struct {
    GPIO_TypeDef* CS_Port;    // Port CS (misal: GPIOB)
    uint16_t      pin_cs;     // Pin CS (misal: GPIO_PIN_6)
    GPIO_TypeDef* WR_Port;    // Port WR (misal: GPIOB)
    uint16_t      pin_wr;     // Pin WR (misal: GPIO_PIN_5)
    GPIO_TypeDef* DATA_Port;  // Port DATA (misal: GPIOB)
    uint16_t      pin_data;   // Pin DATA (misal: GPIO_PIN_4)
    GPIO_TypeDef* BLK_Port;   // Port BLK Backlight (misal: GPIOB)
    uint16_t      pin_blk;    // Pin BLK Backlight (misal: GPIO_PIN_7)

    uint8_t  lcdBuffer[32];

    float    valTegangan;
    float    valArus;
    uint8_t  valSoC;
    float    valSuhu;
    uint16_t valEnergi;
    uint8_t  valErrCode;
    uint8_t  blkDutyCycle;
} JDL7158_HandleTypeDef;

/* --- API Inisialisasi & Kontrol Utama --- */
void JDL7158_Init(JDL7158_HandleTypeDef* lcd,
                  GPIO_TypeDef* csPort, uint16_t csPin,
                  GPIO_TypeDef* wrPort, uint16_t wrPin,
                  GPIO_TypeDef* dataPort, uint16_t dataPin,
                  GPIO_TypeDef* blkPort, uint16_t blkPin);

void JDL7158_Clear(JDL7158_HandleTypeDef* lcd);
void JDL7158_Update(JDL7158_HandleTypeDef* lcd);
void JDL7158_SetBacklight(JDL7158_HandleTypeDef* lcd, uint8_t state);

/* --- API Setter Data Parameter BMS --- */
void JDL7158_SetTegangan(JDL7158_HandleTypeDef* lcd, float volt);
void JDL7158_SetArus(JDL7158_HandleTypeDef* lcd, float ampere);
void JDL7158_SetSoC(JDL7158_HandleTypeDef* lcd, uint8_t percent);
void JDL7158_SetSuhu(JDL7158_HandleTypeDef* lcd, float tempC);
void JDL7158_SetEnergi(JDL7158_HandleTypeDef* lcd, uint16_t kwh);
void JDL7158_SetErrorCode(JDL7158_HandleTypeDef* lcd, uint8_t errCode);

/* --- Fungsi Low-Level Render LCD --- */
void JDL7158_SetSymbol(JDL7158_HandleTypeDef* lcd, uint8_t pin, uint8_t bitPos, bool on);
void JDL7158_SetDigit(JDL7158_HandleTypeDef* lcd, uint8_t pinRight, uint8_t pinLeft, uint8_t val);
void JDL7158_ClearDigit(JDL7158_HandleTypeDef* lcd, uint8_t pinRight, uint8_t pinLeft);

#ifdef __cplusplus
}
#endif

#endif // JDL7158_TM1621_H
