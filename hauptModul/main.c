/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define MAX_MODULES 16
#define STRLEN_BOARD_TYPE 4 //ich denke 3 zeichen reichen 100%
#define STRLEN_POSITION   6 //Le255 + NUL => 6 Zeichen
#define STRLEN_PIN_INFO   32 //auf sicher 32
#define STRLEN_LATEST     32 //auf sicher 32
#define I2C_READ_LENGTH   32

typedef struct {
    uint8_t address;
    char board_type[STRLEN_BOARD_TYPE];
    char position[STRLEN_POSITION];
    char pin_info[STRLEN_PIN_INFO];
    char latest_values[STRLEN_LATEST];
} ModuleInfo;

ModuleInfo modules[MAX_MODULES];
uint8_t module_count = 0;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void Blink_SOS_LED1(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Hilfsfunktion: Lese String vom I2C-Slave
uint8_t i2c_read_string(uint8_t addr, char* buffer, uint16_t buflen, const char* cmd) {
    if (HAL_I2C_Master_Transmit(&hi2c1, addr << 1, (uint8_t*)cmd, strlen(cmd), 100) != HAL_OK)
        return 0;
    HAL_Delay(5);
    uint8_t rx[I2C_READ_LENGTH + 1] = {0};
    if (HAL_I2C_Master_Receive(&hi2c1, addr << 1, rx, I2C_READ_LENGTH, 100) != HAL_OK)
        return 0;
    rx[I2C_READ_LENGTH] = '\0';
    strncpy(buffer, (char*)rx, buflen - 1);
    buffer[buflen - 1] = '\0';
    return 1;
}

// Zähler für Positionen
static uint8_t right_count = 0;
static uint8_t left_count = 0;

// Sucht nach neuen Modulen an Adresse 0x25, initialisiert und fügt sie zum Array hinzu
void scan_for_new_modules(void) {
    // Prüfe, ob schon ein Modul an 0x25 existiert
    for (uint8_t i = 0; i < module_count; ++i)
        if (modules[i].address == 0x25) return;

    char tmp[STRLEN_BOARD_TYPE];
    if (!i2c_read_string(0x25, tmp, sizeof(tmp), "getBoardType"))
        return; // Kein neues Modul gefunden

    // Finde freie Adresse (0x26 bis 0x2A, Beispiel)
    uint8_t new_addr = 0x26;
    for (; new_addr < 0x25 + MAX_MODULES + 1; ++new_addr) {
        uint8_t used = 0;
        for (uint8_t i = 0; i < module_count; ++i)
            if (modules[i].address == new_addr) used = 1;
        if (!used) break;
    }
    if (new_addr > 0x25 + MAX_MODULES) return; // Kein Platz mehr

    // Setze neue Adresse
    char setAddrCmd[24];
    snprintf(setAddrCmd, sizeof(setAddrCmd), "setI2CAddr 0x%02X", new_addr);
    HAL_I2C_Master_Transmit(&hi2c1, 0x25 << 1, (uint8_t*)setAddrCmd, strlen(setAddrCmd), 100);
    HAL_Delay(10);

    // Initialisiere Struct
    ModuleInfo* m = &modules[module_count];
    m->address = new_addr;
    i2c_read_string(new_addr, m->board_type, STRLEN_BOARD_TYPE, "getBoardType");

    // getInfo auslesen und Position/Pins extrahieren
    char info_raw[STRLEN_PIN_INFO + STRLEN_POSITION + 8] = {0};
    if (i2c_read_string(new_addr, info_raw, sizeof(info_raw), "getInfo")) {
        // Erwartetes Format: P,R;A,2;D,4;
        // Position extrahieren und zählen
        if (info_raw[2] == 'R') {
            right_count++;
            snprintf(m->position, STRLEN_POSITION, "Ri%u", right_count);
        } else if (info_raw[2] == 'L') {
            left_count++;
            snprintf(m->position, STRLEN_POSITION, "Le%u", left_count);
        } else {
            strncpy(m->position, "?", STRLEN_POSITION - 1);
            m->position[STRLEN_POSITION - 1] = '\0';
        }

        // Pins extrahieren (alles nach erstem Semikolon)
        char* pins_start = strchr(info_raw, ';');
        if (pins_start && *(pins_start + 1)) {
            strncpy(m->pin_info, pins_start + 1, STRLEN_PIN_INFO - 1);
            m->pin_info[STRLEN_PIN_INFO - 1] = '\0';
        } else {
            m->pin_info[0] = '\0';
        }
    } else {
        strncpy(m->position, "?", STRLEN_POSITION - 1);
        m->position[STRLEN_POSITION - 1] = '\0';
        m->pin_info[0] = '\0';
    }

    m->latest_values[0] = '\0'; // Noch keine Werte

    module_count++;
}

// Fragt alle Module nach aktuellen Werten ab
void update_all_modules(void) {
    for (uint8_t i = 0; i < module_count; ++i) {
        i2c_read_string(modules[i].address, modules[i].latest_values, STRLEN_LATEST, "getNew");
    }
}

// Sendet Status aller Module über UART (kompaktes Textformat)
void send_status_update(void) {
  // Erst alle Module aktualisieren, dann Status senden
  update_all_modules();
  
  for (uint8_t i = 0; i < module_count; ++i) {
    if (modules[i].latest_values[0] != '\0') {
      char line[128];
      int n = snprintf(line, sizeof(line),
          "Addr:0x%02X Type:%s Pos:%s Pins:%s Values:%s\r\n",
          modules[i].address,
          modules[i].board_type,
          modules[i].position,
          modules[i].pin_info,
          modules[i].latest_values);
      if (n > 0) {
        CDC_Transmit_FS((uint8_t*)line, (uint16_t)strnlen(line, sizeof(line)));
      }
    }
  }
  // No explicit newline - let hterm handle line breaks naturally
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    static uint32_t last_usb_ms = 0;
    static uint32_t last_i2c_ms = 0;
    static uint32_t last_update_ms = 0;
    static uint32_t last_led_ms = 0;
    static uint32_t last_status_ms = 0;
    static uint8_t led_state = 0;
    uint32_t now_ms = HAL_GetTick();

    // LED1 blinkt einmal pro Sekunde (nicht-blockierend)
    if (now_ms - last_led_ms >= 1000) {
      led_state = !led_state;
      HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, led_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
      last_led_ms = now_ms;
    }

    // Update latest_values via I2C every second
    if (now_ms - last_update_ms >= 1000) {
      update_all_modules();
      last_update_ms = now_ms;
    }

    // Status output every 50ms (includes update_all_modules)
    if (now_ms - last_status_ms >= 50) {
      send_status_update();
      last_status_ms = now_ms;
    }

    if (now_ms - last_i2c_ms >= 5000) {
      scan_for_new_modules();
      last_i2c_ms = now_ms;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LeftDet_Pin|RigthDet_Pin|BotDet_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LeftDet_Pin RigthDet_Pin BotDet_Pin */
  GPIO_InitStruct.Pin = LeftDet_Pin|RigthDet_Pin|BotDet_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED1_Pin */
  GPIO_InitStruct.Pin = LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

static void BlinkDelay(uint32_t ms)
{
  HAL_Delay(ms);
}

static void Blink_Short(void)
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  BlinkDelay(200);
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  BlinkDelay(200);
}

static void Blink_Long(void)
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  BlinkDelay(600);
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  BlinkDelay(200);
}

static void Blink_SOS_LED1(void)
{
  for (int i = 0; i < 3; i++) { Blink_Short(); }
  BlinkDelay(300);
  for (int i = 0; i < 3; i++) { Blink_Long(); }
  BlinkDelay(300);
  for (int i = 0; i < 3; i++) { Blink_Short(); }
}
