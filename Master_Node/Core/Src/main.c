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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
CAN_HandleTypeDef hcan1;

UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t rx_char;
char rx_buffer[64];
uint8_t rx_index = 0;
volatile uint8_t cmd_ready = 0;
CAN_TxHeaderTypeDef txHeader;
CAN_RxHeaderTypeDef rxHeader;

uint8_t txData[8];
uint8_t rxData[8];

uint32_t txMailbox;
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
  MX_CAN1_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  char banner[] =
  "\r\n=== CAN Test Node ===\r\n"
  "Format:\r\n"
  "send <id_hex> <data|remote> <dlc> <bytes..>\r\n"
  "Example:\r\n"
  "send 123 data 2 AA BB\r\n> ";

  HAL_UART_Transmit(&huart3, (uint8_t*)banner, sizeof(banner)-1, 10);

  //HAL_CAN_Stop(&hcan1);



  HAL_CAN_Start(&hcan1);
  HAL_UART_Receive_IT(&huart3, &rx_char, 1);

    // Activate the notification
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if (cmd_ready)
	  {
	      cmd_ready = 0;

	      char *token = strtok(rx_buffer, " ");
	      if (token && strcmp(token, "send") == 0)
	      {
	          /* ID */
	          token = strtok(NULL, " ");
	          uint32_t id = strtoul(token, NULL, 16);

	          /* RTR */
	          token = strtok(NULL, " ");
	          uint8_t rtr = (strcmp(token, "remote") == 0) ? CAN_RTR_REMOTE : CAN_RTR_DATA;

	          /* DLC */
	          token = strtok(NULL, " ");
	          uint8_t dlc = atoi(token);

	          /* CAN header */
	          txHeader.StdId = id;
	          txHeader.IDE = CAN_ID_STD;
	          txHeader.RTR = rtr;
	          txHeader.DLC = dlc;
	          txHeader.TransmitGlobalTime = DISABLE;

	          /* Data */
	          if (rtr == CAN_RTR_DATA)
	          {
	              for (uint8_t i = 0; i < dlc; i++)
	              {
	                  token = strtok(NULL, " ");
	                  txData[i] = strtoul(token, NULL, 16);
	              }
	          }

	          HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);

	          char ack[] = "CAN frame sent\r\n> ";
	          HAL_UART_Transmit(&huart3, (uint8_t*)ack, sizeof(ack)-1, HAL_MAX_DELAY);
	      }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 2;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_10TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_5TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */
  CAN_FilterTypeDef canfilterconfig;

  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig.FilterBank = 18;  // which filter bank to use from the assigned ones
  canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  canfilterconfig.FilterIdHigh = 0x000;
  canfilterconfig.FilterIdLow = 0x000;
  canfilterconfig.FilterMaskIdHigh = 0x000;
  canfilterconfig.FilterMaskIdLow = 0x0000;
  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
  canfilterconfig.SlaveStartFilterBank = 20;  // how many filters to assign to the CAN1 (master can)

  HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);
  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PD12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        if (rx_char == '\r' || rx_char == '\n')
        {
            rx_buffer[rx_index] = '\0';
            rx_index = 0;
            cmd_ready = 1;   // command complete
        }
        else
        {
            if (rx_index < sizeof(rx_buffer) - 1)
            {
                rx_buffer[rx_index++] = rx_char;
            }
        }

        HAL_UART_Receive_IT(&huart3, &rx_char, 1);
    }
}

uint16_t adc_buffer[3];
uint8_t  adc_valid[3] = {0, 0, 0};

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    char msg[128];
    char mssg[128];

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData);

    int len = sprintf(msg,
        "\r\n[CAN RX] ID:0x%lX DLC:%d RTR:%s Data:",
        (rxHeader.IDE == CAN_ID_STD) ? rxHeader.StdId : rxHeader.ExtId,
        rxHeader.DLC,
        (rxHeader.RTR == CAN_RTR_DATA) ? "DATA" : "REMOTE"
    );

    for (uint8_t i = 0; i < rxHeader.DLC; i++)
    {
        len += sprintf(&msg[len], "%02X ", rxData[i]);
    }

    len += sprintf(&msg[len], "\r\n> ");
    HAL_UART_Transmit(&huart3, (uint8_t*)msg, len, HAL_MAX_DELAY);

    //pour adc ////////////////////


    int lenadc;
	char adcmsg [127];

	 if (rxHeader.StdId == 0x100 && rxHeader.DLC >= 3)
	    {
	        uint8_t type = rxData[0];
	        uint16_t adc = rxData[1] | (rxData[2] << 8);

	        switch (type)
	        {
	            case 0x61:   // ADC 1
	                adc_buffer[0] = adc;
	                adc_valid[0]  = 1;
	                break;

	            case 0x62:   // ADC 2
	                adc_buffer[1] = adc;
	                adc_valid[1]  = 1;
	                break;

	            case 0x63:   // ADC 3
	                adc_buffer[2] = adc;
	                adc_valid[2]  = 1;
	                break;

	            default:
	                return;
	        }

	        /* Vérifier si les 3 valeurs sont reçues */
	        if (adc_valid[0] && adc_valid[1] && adc_valid[2])
	        {
	            char adcmsg[128];
	            int len;

	            len = sprintf(adcmsg,
	                "%u,%u,%u\r\n",
	                adc_buffer[0],
	                adc_buffer[1],
	                adc_buffer[2]);

	            /* Envoi UART (USART6 / USART7 selon ton projet) */
	            HAL_UART_Transmit(&huart3, (uint8_t*)adcmsg, len, HAL_MAX_DELAY);
	            HAL_UART_Transmit(&huart6, (uint8_t*)adcmsg, len, HAL_MAX_DELAY);
	            // ou huart7 si c’est USART7

	            /* Reset pour la prochaine série */
	            adc_valid[0] = 0;
	            adc_valid[1] = 0;
	            adc_valid[2] = 0;
	        }
	    }



    /////////////////////////////

    ////////pour temp//////////////
    int templen =0;
    if (rxHeader.StdId == 0x681 && rxHeader.DLC == 3)
    {
    	int16_t value = (rxData[1] << 8) | rxData[2];
    	int t = value;
    	templen = sprintf(mssg, "Temp = %d.%d C\r\n", t/10, abs(t%10));
    	//templen += sprintf(&mssg[templen], "\r\n> ");
        //float temp;
        //memcpy(&temp, rxData, sizeof(float));

        //len += sprintf(&msg, " | Temp=%.2f C", temp);
    }
    //////////////////////////////////////
    HAL_UART_Transmit(&huart3, (uint8_t*)mssg, templen, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart6, (uint8_t*)mssg, templen, HAL_MAX_DELAY);

    /*if (rxHeader.DLC >= 4)
    {
    	uint16_t val = rxData[0] | (rxData[1] << 8); // si 2 octets
    	float f = (float)val;


        memcpy(&f, rxData, sizeof(float));

        //len = sprintf(msg, "Float reçu = %.3f\r\n> ", f);/
    	int int_part = (int)f;
    	int frac_part = (int)((f - int_part) * 1000); // 3 décimales

    	len = sprintf(msg, "Float reçu = %d.%03d\r\n", int_part, frac_part);

        HAL_UART_Transmit(&huart3, (uint8_t*)msg, len, 1000);
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, len,1000);

    }

    else
    {
        // Prompt
        HAL_UART_Transmit(&huart3, (uint8_t*)"> ", 2, HAL_MAX_DELAY);
    }*/

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
