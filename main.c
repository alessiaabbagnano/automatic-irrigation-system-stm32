/* USER CODE BEGIN Header */
/**
  * ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include "stm32f3xx_hal.h"
#include "liquidcrystal_i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t secondi;
    uint8_t minuti;
    uint8_t ore;
} DS1307_Time;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
volatile uint8_t test_pioggia = 0;  // 0=non piove     1=piove
uint16_t adc_valori[1];             // Array destinazione DMA
volatile uint16_t test_umidita = 0; // valore umidità cosi come lo da il sensore
volatile uint8_t umidita_percentuale = 0;
DS1307_Time orologio;               // Struttura dati dell'orologio RTC


volatile uint8_t rtc_ore = 0;
volatile uint8_t rtc_minuti = 0;
volatile uint8_t rtc_secondi = 0;


volatile uint8_t ult_irr_ore = 0;
volatile uint8_t ult_irr_minuti = 0;
volatile uint8_t ult_irr_secondi = 0;
volatile uint8_t mai_irrigato = 1;

volatile uint32_t irr_start_time = 0; // Registra il millisecondo in cui si accende il buzzer
volatile uint8_t irr_on = 0;      // stato irrigazione

volatile uint8_t cont ;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
uint8_t DecToBcd(uint8_t val);
uint8_t BcdToDec(uint8_t val);
void DS1307_SetTime(uint8_t sec, uint8_t min, uint8_t hour);
void DS1307_ReadTime(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define DS1307_I2C_ADDRESS (0x68 << 1) // Indirizzo I2C dell'RTC shiftato per l'HAL per lasciare l'ultimo bit per 0/1 Read/Write

// Funzioni di conversione BCD per RTC RTC conta in binario bisogna avere decimale
uint8_t DecToBcd(uint8_t val) {
    return ((val / 10 * 16) + (val % 10));  //es: 25/10=2 -> *16 shifta 0010 0000 + 25%10=2 0000 0010
}

uint8_t BcdToDec(uint8_t val) {
    return ((val / 16 * 10) + (val % 16));
}

// Funzione per impostare l'ora iniziale dell'RTC
void DS1307_SetTime(uint8_t sec, uint8_t min, uint8_t hour) {
    uint8_t dati_da_scrivere[3];
    dati_da_scrivere[0] = DecToBcd(sec);
    dati_da_scrivere[1] = DecToBcd(min);
    dati_da_scrivere[2] = DecToBcd(hour);

    HAL_I2C_Mem_Write(&hi2c1, DS1307_I2C_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, dati_da_scrivere, 3, HAL_MAX_DELAY);
}

// Funzione per leggere l'ora corrente dall'RTC
void DS1307_ReadTime(void) {
    uint8_t buffer_lettura[7];

    HAL_I2C_Mem_Read(&hi2c1, DS1307_I2C_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, buffer_lettura, 3, HAL_MAX_DELAY);
    //puntatore periferica i2C, indirizzo periferica, indirizzo registro interno RTC, grandezza valori, destinazione, numero byte, tempo massimo attesa

    orologio.secondi = BcdToDec(buffer_lettura[0] & 0x7F);
    orologio.minuti = BcdToDec(buffer_lettura[1]);
    orologio.ore = BcdToDec(buffer_lettura[2] & 0x3F);

    rtc_ore = orologio.ore;
    rtc_minuti = orologio.minuti;
    rtc_secondi = orologio.secondi;
}


//funzione bloccante buzzer, da non usare
/*
void buzzer_bip(uint16_t durata_ms) {
    int cicli = durata_ms * 2;

    for (int i = 0; i < cicli; i++) {
        // Semionda ALTA
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
        for (volatile int d = 0; d < 350; d++);

        // Semionda BASSA
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
        for (volatile int d = 0; d < 350; d++);
    }
}
*/

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  //disabilitp interrupt DMA, lavora in silenzio per non avere troppe interrupt.
  //Come se periferica lavorasse in Collegamento Passivo
  HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);

  //resetto interrupt flag a 0 per avvio
  __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);

  //Timer 3 in modalità Interrupt
  HAL_TIM_Base_Start_IT(&htim3);

  //l'ADC in modalità DMA continuo
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_valori, 1);

  //setting del RTC
  //DS1307_SetTime(0, 28, 13);


  // Inizializzazione dello schermo LCD
  lcd_init();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char riga1_display[20]; // Buffer sicuro per l'orario
  char riga2_display[20]; // Buffer sicuro per i sensori

  while (1)
  {

	  if(irr_on==1) {

		  if (( HAL_GetTick()-irr_start_time>30000) || (test_pioggia==1))
				{
					irr_on = 0;
					ult_irr_ore = rtc_ore;
					ult_irr_minuti = rtc_minuti;
					ult_irr_secondi = rtc_secondi;
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);

					char msg_seriale[80];

					int lunghezza = sprintf(msg_seriale, "STATO: Irrigazione Terminata [ %02d:%02d:%02d ] \r\n", rtc_ore, rtc_minuti, rtc_secondi);


					HAL_UART_Transmit(&huart1, (uint8_t*)msg_seriale, lunghezza, 100);


					}
	  }


      if (HAL_GPIO_ReadPin(Sensore_Piogga_GPIO_Port, Sensore_Piogga_Pin) == GPIO_PIN_RESET)
      {test_pioggia = 1;}
      else
      {test_pioggia = 0;}


      test_umidita = adc_valori[0];
      umidita_percentuale=(1800-test_umidita)*100/(1500); //(VAL_ASCIUTTO-VALATTUALE)*100/(VALASCIUTTO-VALBAGNATO)


      DS1307_ReadTime();

      sprintf(riga1_display, "%02d:%02d:%02d", rtc_ore, rtc_minuti, rtc_secondi);
      sprintf(riga2_display, "P=%d     U:%02d%%  ", test_pioggia, umidita_percentuale);


      lcd_put_cur(0, 0);
      lcd_send_string(riga1_display);
      lcd_put_cur(1, 0);
      lcd_send_string(riga2_display);


      HAL_Delay(700);  //SERVE PER I2C e aggiornamento schermo. Bloccante
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_ADC12;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Adc12ClockSelection = RCC_ADC12PLLCLK_DIV1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hi2c1.Init.Timing = 0x00201D2B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 23999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 59999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE6 PE8 PE9 PE10
                           PE11 PE12 PE13 PE14
                           PE15 */
  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : Sensore_Piogga_Pin */
  GPIO_InitStruct.Pin = Sensore_Piogga_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Sensore_Piogga_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// La funzione callback del Timer 3 scatta automaticamente in background ogni 10 secondi esatti
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {

    	char msg_seriale[80];
    	int lunghezza = sprintf(msg_seriale, "STATO: Interruzione Partita [ %02d:%02d:%02d ] \r\n", rtc_ore, rtc_minuti, rtc_secondi);
    	HAL_UART_Transmit(&huart1, (uint8_t*)msg_seriale, lunghezza, 100);
    	cont=0;


        // Calcolo dei minuti passati correggendo l'eventuale passaggio dell'ora (es. da minuto 59 a minuto 01)
        int8_t minuti_passati = rtc_minuti - ult_irr_minuti;
        int8_t secondi_passati = rtc_secondi - ult_irr_secondi;
        if (minuti_passati < 0) minuti_passati += 60;

        // Se non piove E (è passato almeno 1 minuto OPPURE è la prima volta in assoluto)
        if (test_pioggia == 0 && (mai_irrigato == 1 || minuti_passati>1 || (minuti_passati >= 1 && secondi_passati>28)))
        {
            if (umidita_percentuale < 35)
            {


                mai_irrigato = 0;

                //Accendo i LED
                //Salvo i millisecondi attuali
                //Alzo segnale di irrigazione

                //HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                irr_start_time = HAL_GetTick();
                irr_on = 1;
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);

                int lunghezza = sprintf(msg_seriale, "STATO: Irrigazione Partita [ %02d:%02d:%02d ] \r\n", rtc_ore, rtc_minuti, rtc_secondi);
                HAL_UART_Transmit(&huart1, (uint8_t*)msg_seriale, lunghezza, 100);

}
        }else{
        	irr_on=0;
        }
    }
}


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
