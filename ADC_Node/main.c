#include "stm32f4xx.h"
#include "stm32f4xx_can.h"
#include "ADC_Dma.h"
#include <stdio.h>

extern void config_EXTI();
extern void config_TIM2();
extern void config_ADC1();
extern void	config_DMA2();


/*extern void EXTI0_IRQHandler();
extern void DMA2_Stream0_IRQHandler(); // shnoua l far9 mebin () et (void)? o extern?
*/


extern volatile uint16_t ADC_VALUE[3];
char ADC_value_char[4] = {0};

// Define constants for request and response IDs
#define FILTRE_ID 0x60
#define RESPONSE_ID 0x100
#define FRAME_DLC 3U
//Define constants for ADC channels
#define channel_4 0x61
#define channel_5 0x62
#define channel_6 0x63


// Define constants for LED pins
#define LED_GREEN GPIO_ODR_ODR_12  // Reception
#define LED_ORANGE GPIO_ODR_ODR_13  // Transmission
#define LED_RED GPIO_ODR_ODR_14     // Transmission failure
//#define LED_BLUE GPIO_ODR_ODR_15   // Transmission 

void SystemClock_Config(void ); // Initialize HSE clock
void GPIO_Init(void ); // Initialize GPIO
void CAN_config(void ); // Configure CAN
void Filter_Config(void ); // Configure CAN filter
void NVIC_Config(void ); // Add this line to configure NVIC
uint8_t CAN1_Transmit(void ); // Transmit the message
void CAN1_Recieve (void) ;


CAN_InitTypeDef CAN_InitStruct; //À cet instant, on a créer en memoire la variable CAN_InitStruct contenant des valeurs aléatoiresde de type CAN_InitTypeDef  qui est une  structure (déclaré ds can.h )  
CAN_FilterInitTypeDef Filter_InitStruct;


CanTxMsg TxMessage;
CanRxMsg RxMessage;



/*void HSE_CLK(void)  // Function to enable the High-Speed External (HSE) clock
{
    // The HSE clock operates at 8MHz, providing lower energy consumption and improved stability compared to the High-Speed Internal (HSI) clock.
    // Note: The HSI clock, which relies on an RC circuit, may generate heat during operation.

    // Step 1: Enable the HSE clock
    RCC->CR |= RCC_CR_HSEON; // Set HSEON bit to turn on HSE

    // Step 2: Wait until the HSE is ready
    while (!(RCC->CR & RCC_CR_HSERDY)); // Polling the HSERDY flag to ensure HSE is stable

    // Step 3: Select HSE as the system clock source
    RCC->CFGR = RCC_CFGR_SW_0; // Set SW bits to select HSE as system clock (multiplexer configuration)

    // Step 4: Wait until the system clock switch is complete
    while (!(RCC->CFGR & RCC_CFGR_SWS_0)); // Polling the SWS bits to confirm the system clock is now HSE
}*/

void SystemClock_Config(void)
{
    /* 1. Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 2. Enable Power interface clock */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

    /* 3. Voltage regulator Scale 1 */
    PWR->CR |= PWR_CR_VOS;

    /* 4. Configure Flash
       - 5 wait states
       - Prefetch, Instruction cache, Data cache enable
    */
    FLASH->ACR =
        FLASH_ACR_ICEN |
        FLASH_ACR_DCEN |
        FLASH_ACR_PRFTEN |
        FLASH_ACR_LATENCY_5WS;

    /* 5. Configure PLL
       PLLM = 4
       PLLN = 168
       PLLP = 2
       PLL source = HSE
    */
    RCC->PLLCFGR =
        (4 << RCC_PLLCFGR_PLLM_Pos) |
        (168 << RCC_PLLCFGR_PLLN_Pos) |
        (0 << RCC_PLLCFGR_PLLP_Pos) |   // PLLP = 2
        RCC_PLLCFGR_PLLSRC_HSE |
        (4 << RCC_PLLCFGR_PLLQ_Pos);

    /* 6. Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 7. Configure prescalers
       AHB  = SYSCLK /1
       APB1 = HCLK /4
       APB2 = HCLK /2
    */
    RCC->CFGR |=
        RCC_CFGR_HPRE_DIV1 |
        RCC_CFGR_PPRE1_DIV4 |
        RCC_CFGR_PPRE2_DIV2;

    /* 8. Select PLL as system clock */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}




void GPIO_Init(void)
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; //GPIOA EN 
	 GPIOB->MODER &= ~((3 << 16) | (3 << 18));
	GPIOB->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1; // Configure (Pb9) and RX(Pb9) en AF (10 ds moder12 et moder11)
  GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4));
	GPIOB->AFR[1] |= (0x9 << 0) | (0x9 << 4); //TX de CAN1 lié au PB8 et RX a PB9: AF9
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // Enable clock for GPIOD
  GPIOD->MODER |= GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0; // Configure PD12, PD13, and PD14 as digital outputs
	GPIOB->OSPEEDR |= (0x3 << 16) | (0x3 << 18);
}

/*void NVIC_Config(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN; // Enable clock for CAN1
    // Configure interrupt priorities
    NVIC_SetPriority(CAN1_RX0_IRQn, 1); // Set interrupt priority
    NVIC_EnableIRQ(CAN1_RX0_IRQn); // Enable interrupt
}*/

void CAN_config(void)
{
	// Enable CAN clock
  RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
	CAN_StructInit(&CAN_InitStruct) ; // remplir la structure CAN_initstruct avec les valeurs par défaut en appelant la fct CAN_StructInit du driver can.c
	// a ce stade on va changer qq valeur par défaut pour adapter l'aaplication du périph CAN 
	CAN_InitStruct.CAN_Mode = CAN_Mode_Normal;
	// Cet exemple est pour un PCLK1 de 8MHz, pour obtenir 250 kbit/s
  // 8MHz / (Prescaler=2 * (1 + BS1_10tq + BS2_5tq)) = 250 kbit/s
	CAN_InitStruct.CAN_Prescaler = 2; // Prescaler for frequency
  CAN_InitStruct.CAN_BS1 = CAN_BS1_10tq; // Bit time segment 1
  CAN_InitStruct.CAN_BS2 = CAN_BS2_5tq; // Bit time segment 2
  CAN_InitStruct.CAN_SJW = CAN_SJW_1tq; // Synchronization jump width  SJW = 1 TQ
	// Le reste des paramètres (TTCM, ABOM, etc.) est laissé à DISABLE par la fonction CAN_StructInit
	//Appliquer la configuration logiciel au matériel CAN1 avec la fct CAN_Init du driver can.c
	if (CAN_Init(CAN1, &CAN_InitStruct) == CAN_InitStatus_Success)
	{
			GPIOD->ODR |= LED_ORANGE;
	}
	else
	{
			GPIOD->ODR |= LED_RED;
	}
	//CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE); // Enable interrupt for pending messages in FIFO 0
}	


void Filter_Config(void)
{
	Filter_InitStruct.CAN_FilterNumber = 18; // Filter number
  Filter_InitStruct.CAN_FilterMode = CAN_FilterMode_IdMask;
	Filter_InitStruct.CAN_FilterScale = CAN_FilterScale_32bit;
	Filter_InitStruct.CAN_FilterIdHigh = (FILTRE_ID << 5);
  Filter_InitStruct.CAN_FilterIdLow = 0x0000;
  //Filter_InitStruct.CAN_FilterMaskIdHigh = 0x0000 << 5; // Mask to filter one message qui est  filtre_id
  Filter_InitStruct.CAN_FilterMaskIdHigh = 0x7FC << 5; // Mask to filter 3 id
  Filter_InitStruct.CAN_FilterMaskIdLow = 0x0000;
  Filter_InitStruct.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
  Filter_InitStruct.CAN_FilterActivation = ENABLE;
	CAN_SlaveStartBank(20);
  CAN_FilterInit(&Filter_InitStruct);
	

	
}






uint8_t CAN1_Transmit(void)
{
	  TxMessage.StdId = RESPONSE_ID ; // Standard ID
    TxMessage.ExtId = 0x00; // No extended ID
    TxMessage.IDE = CAN_Id_Standard; // Use standard ID
    TxMessage.RTR = CAN_RTR_Data; // Data frame
    TxMessage.DLC = FRAME_DLC; // Data length
		return CAN_Transmit(CAN1, &TxMessage);
   /* uint8_t  i;
    for (i = 0; i < FRAME_DLC; i++)
    {
        TxMessage.Data[i] = data_to_send[i]; // Set the data
    }
		uint8_t transmit_mailbox = CAN_Transmit(CAN1, &TxMessage); // Transmit the CAN message
    uint8_t status = CAN_TransmitStatus(CAN1, transmit_mailbox);
		GPIOD->ODR |= LED_GREEN; // Turn on the green LED to indicate transmission
		 if (status!=0)
    {
        GPIOD->ODR |= LED_BLUE; // Turn on the green LED to indicate successful transmission
    }
    else
    {
        GPIOD->ODR |= LED_RED; // Turn on the red LED to indicate transmission failure
    }*/
}


int main(void)



{
    SystemClock_Config();        // Horloge
    GPIO_Init();      // GPIO + LEDs + CAN pins

    config_ADC1();    // ADC
    config_DMA2();    // DMA
    config_TIM2();    // Trigger ADC  

    CAN_config();     // CAN init
		Filter_Config();  // CAN filter
		CAN_OperatingModeRequest(CAN1, CAN_OperatingMode_Normal);
    
    //NVIC_Config();    // Interruptions CAN
			             
		//CAN_TransmitStatus(CAN1, 0);
    while (1)
    {
				uint32_t id = (CAN1->sFIFOMailBox[0].RIR >> 21) & 0x7FF;
        if (CAN_MessagePending(CAN1,CAN_FIFO0))
			  // if ( CAN1 ->RF0R & CAN_RF0R_FMP0 != 0) 
			
    {
        // Read the received CAN frame
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
			  //GPIOD->ODR |= LED_ORANGE;			
        if (id == channel_4)
        {
          
          TxMessage.Data[0] = channel_4;       // Identifier for channel 4
          TxMessage.Data[1] = (uint8_t)(ADC_VALUE[0] & 0xFF);        // LSB
					TxMessage.Data[2] = (uint8_t)(ADC_VALUE[0] >> 8);          // MSB
          
      
          
					uint8_t mailbox = CAN1_Transmit();

						if (CAN_TransmitStatus(CAN1, mailbox) == CANTXOK)
						{
								GPIOD->ODR |= LED_ORANGE;
						}
						else
						{
								GPIOD->ODR |= LED_RED;
						}
					}
					/* CAN_Transmit(CAN1, &TxMessage);
					GPIOD->ODR |= LED_ORANGE;	*/
					else if (id == channel_5)
					{
						TxMessage.Data[0] = channel_5;
						TxMessage.Data[1] = (uint8_t)(ADC_VALUE[1] & 0xFF);        // LSB
					  TxMessage.Data[2] = (uint8_t)(ADC_VALUE[1] >> 8);          // MSB
            uint8_t mailbox = CAN1_Transmit();
					}
					else if (id == channel_6)
					{
						TxMessage.Data[0] = channel_6;
						TxMessage.Data[1] = (uint8_t)(ADC_VALUE[2] & 0xFF);        // LSB
					  TxMessage.Data[2] = (uint8_t)(ADC_VALUE[2] >> 8);          // MSB
            uint8_t mailbox = CAN1_Transmit();
					}       
			}
    }
}




/*void CAN1_RX0_IRQHandler(void)
{
    // Check if a message is pending in FIFO 0
    if (CAN_MessagePending(CAN1,CAN_FIFO0))
    {
        // Read the received CAN frame
        CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
			  //GPIOD->ODR |= LED_ORANGE;
				
        if (RxMessage.Data[0] == channel_4)
        {
          // Request for ADC Channel 4
          sprintf(ADC_value_char, "%04u", ADC_VALUE[0]); // Convert ADC value to string with 4 digits

          // Prepare CAN message with ASCII characters
          TxMessage.Data[0] = channel_4;       // Identifier for channel 4
          TxMessage.Data[1] = ADC_value_char[0]; // Most significant digit
          TxMessage.Data[2] = ADC_value_char[1];
          TxMessage.Data[3] = ADC_value_char[2];
          TxMessage.Data[4] = ADC_value_char[3]; // Least significant digit
					CAN_Transmit(CAN1, &TxMessage);
					GPIOD->ODR |= LED_ORANGE;		
        }
        else if (RxMessage.Data[0] == channel_5)
        {
          // Request for ADC Channel 5
          sprintf(ADC_value_char, "%04u", ADC_VALUE[1]); // Convert ADC value to string with 4 digits

          // Prepare CAN message with ASCII characters
          TxMessage.Data[0] = channel_5;       // Identifier for channel 14
          TxMessage.Data[1] = ADC_value_char[0]; // Most significant digit
          TxMessage.Data[2] = ADC_value_char[1];
          TxMessage.Data[3] = ADC_value_char[2];
          TxMessage.Data[4] = ADC_value_char[3]; // Least significant digit
					CAN_Transmit(CAN1, &TxMessage);
					GPIOD->ODR |= LED_GREEN;		
        }
        else if (RxMessage.Data[0] == channel_6)
        {
          // Request for ADC Channel 6
          sprintf(ADC_value_char, "%04u", ADC_VALUE[2]); // Convert ADC value to string with 4 digits

          // Prepare CAN message with ASCII characters
          TxMessage.Data[0] = channel_6;       // Identifier for channel 15
          TxMessage.Data[1] = ADC_value_char[0]; // Most significant digit
          TxMessage.Data[2] = ADC_value_char[1];
          TxMessage.Data[3] = ADC_value_char[2];
          TxMessage.Data[4] = ADC_value_char[3]; // Least significant digit
					CAN_Transmit(CAN1, &TxMessage);
					GPIOD->ODR |= LED_BLUE;		
				}
			}			
		 CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
}*/