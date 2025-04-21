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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif

// WiFi credentials
#define WIFI_SSID "Bengez"
#define WIFI_PASSWORD "bengi123"
// ThingSpeak API key
#define THINGSPEAK_API_KEY "O9BAF9KP29MTGHRO"

void configureLEDs(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_7;  // PA4 (zelena), PA7 (crvena)
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Funkcija za slanje podataka na Nextion LCD
void Nextion_Send_Command(char *cmd) {
    char buffer[50];
    sprintf(buffer, "%s", cmd);
    HAL_UART_Transmit(&huart4, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
    // Šalje tri 0xFF bajta koji označavaju kraj komande
    uint8_t end_cmd[3] = {0xFF, 0xFF, 0xFF};
    HAL_UART_Transmit(&huart4, end_cmd, 3, HAL_MAX_DELAY);
}

PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart4, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

GETCHAR_PROTOTYPE
{
    uint8_t ch = 0;
    __HAL_UART_CLEAR_OREFLAG(&huart4);
    HAL_UART_Receive(&huart4, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart4, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// Funkcija za slanje podataka na ThingSpeak preko ESP8266
void SendToThingSpeak(float temperature, float humidity) {
    char espBuf[150] = {0};
    char tempStr[10], humStr[10];

    // Pretvori temperature i vlažnost u stringove
    sprintf(tempStr, "%.1f", temperature);
    sprintf(humStr, "%.1f", humidity);

    // Postavi WiFi mode na STA
    sprintf(espBuf, "AT+CWMODE=1\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t *)espBuf, strlen(espBuf), HAL_MAX_DELAY);
    HAL_Delay(2000);

    // Spoji se na WiFi
    sprintf(espBuf, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    HAL_UART_Transmit(&huart1, (uint8_t *)espBuf, strlen(espBuf), HAL_MAX_DELAY);
    HAL_Delay(5000);

    // Postavi multipleksnu vezu
    sprintf(espBuf, "AT+CIPMUX=1\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t *)espBuf, strlen(espBuf), HAL_MAX_DELAY);
    HAL_Delay(2000);

    // Spoji se na ThingSpeak server
    sprintf(espBuf, "AT+CIPSTART=0,\"TCP\",\"api.thingspeak.com\",80\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t *)espBuf, strlen(espBuf), HAL_MAX_DELAY);
    HAL_Delay(5000);

    // Pripremi GET zahtjev
    char getRequest[100];
    sprintf(getRequest, "GET /update?api_key=%s&field1=%s&field2=%s\r\n",
            THINGSPEAK_API_KEY, tempStr, humStr);

    // Pošalji dužinu podataka
    sprintf(espBuf, "AT+CIPSEND=0,%d\r\n", strlen(getRequest));
    HAL_UART_Transmit(&huart1, (uint8_t *)espBuf, strlen(espBuf), HAL_MAX_DELAY);
    HAL_Delay(2000);

    // Pošalji GET zahtjev
    HAL_UART_Transmit(&huart1, (uint8_t *)getRequest, strlen(getRequest), HAL_MAX_DELAY);
    HAL_Delay(5000);

    // Zatvori vezu
    sprintf(espBuf, "AT+CIPCLOSE=0\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t *)espBuf, strlen(espBuf), HAL_MAX_DELAY);
    HAL_Delay(1000);
}
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define DHT11_PORT GPIOA
#define DHT11_PIN GPIO_PIN_6
uint8_t RHI, RHD, TCI, TCD, SUM;
uint32_t pMillis, cMillis;
float tCelsius = 0;
float RH = 0;
char strCopy[15];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void microDelay (uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0);
  while (__HAL_TIM_GET_COUNTER(&htim1) < delay);
}

uint8_t DHT11_Start (void)
{
  uint8_t Response = 0;
  GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
  GPIO_InitStructPrivate.Pin = DHT11_PIN;
  GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as output
  HAL_GPIO_WritePin (DHT11_PORT, DHT11_PIN, 0);   // pull the pin low
  HAL_Delay(20);   // wait for 20ms
  HAL_GPIO_WritePin (DHT11_PORT, DHT11_PIN, 1);   // pull the pin high
  microDelay (30);   // wait for 30us
  GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as input
  microDelay (40);
  if (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))
  {
    microDelay (80);
    if ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN))) Response = 1;
  }
  pMillis = HAL_GetTick();
  cMillis = HAL_GetTick();
  while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
  {
    cMillis = HAL_GetTick();
  }
  return Response;
}

uint8_t DHT11_Read (void)
{
  uint8_t a,b;
  for (a=0;a<8;a++)
  {
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go high
      cMillis = HAL_GetTick();
    }
    microDelay (40);   // wait for 40 us
    if (!(HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)))   // if the pin is low
      b&= ~(1<<(7-a));
    else
      b|= (1<<(7-a));
    pMillis = HAL_GetTick();
    cMillis = HAL_GetTick();
    while ((HAL_GPIO_ReadPin (DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go low
      cMillis = HAL_GetTick();
    }
  }
  return b;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    setvbuf(stdin, NULL, _IONBF, 0);
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
  MX_TIM1_Init();
  MX_UART4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1);
  configureLEDs();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (DHT11_Start()) {
          RHI = DHT11_Read();
          RHD = DHT11_Read();
          TCI = DHT11_Read();
          TCD = DHT11_Read();
          SUM = DHT11_Read();

          if (RHI + RHD + TCI + TCD == SUM) {
              tCelsius = (float)TCI + (float)(TCD / 10.0);
              RH = (float)RHI + (float)(RHD / 10.0);

              printf("Temperature: %.1f C\r\n", tCelsius);
              printf("Humidity: %.1f %%\r\n\r\n", RH);

              char uartBuf[50];

              // Ažuriranje numeričkog dela n0 (temperatura)
              sprintf(uartBuf, "n0.val=%d", (int)tCelsius);
              Nextion_Send_Command(uartBuf);

              // Ažuriranje tekstualnog dela t0 (temperatura)
              sprintf(uartBuf, "t0.txt=\"%.1f C\"", tCelsius);
              Nextion_Send_Command(uartBuf);

              // Ažuriranje numeričkog dela n1 (vlažnost)
              sprintf(uartBuf, "n1.val=%d", (int)RH);
              Nextion_Send_Command(uartBuf);

              // Ažuriranje tekstualnog dela t1 (vlažnost)
              sprintf(uartBuf, "t1.txt=\"%.1f %%\"", RH);
              Nextion_Send_Command(uartBuf);

              // Kontrola LEDica na osnovu temperature
              if (tCelsius > 24) {
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);    // Crvena LED ON
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);  // Zelena LED OFF
              } else {
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);  // Crvena LED OFF
                  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);    // Zelena LED ON
              }

              // Šalji podatke na ThingSpeak
              SendToThingSpeak(tCelsius, RH);
          }
      }
      HAL_Delay(30000); // Čekaj 30 sekundi prije sljedećeg očitavanja
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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

#ifdef  USE_FULL_ASSERT
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
