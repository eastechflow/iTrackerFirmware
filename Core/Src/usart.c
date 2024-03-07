/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2019 Eastech Flow Controls, Inc.
*
* OWNER:
* Eastech Flow Controls, Inc.
* 4250 S. 76th E. Avenue
* Tulsa, Oklahoma 74145
*
* toll-free: 	(800) 226-3569
* phone: 	(918) 664-1212
* fax: 	(918) 664-8494
* email: 	info@eastechflow.com
* website: 	http://eastechflow.com/
*
* OBLIGATORY: All information contained herein is, and remains
* the property of EASTECH FLOW CONTROLS, INC. The intellectual
* and technical concepts contained herein are proprietary to
* EASTECH FLOW CONTROLS, INC. and its suppliers and may be covered
* by U.S. and Foreign Patents, patents in process, and are protected
* by trade secret or copyright law.
*
* Dissemination, transmission, forwarding or reproduction of this
* content in any way is strictly forbidden.
*************************************************************************/
#include "common.h"

#include "usart.h"
#include "gpio.h"
#include "uart1.h"
#include "uart2.h"
#include "uart3.h"
#include "uart4.h"
#include "lpuart1.h"
#include "factory.h"
#include "debug_helper.h"


#define WIFI_BAUD       115200   // WiFi is on uart2
#define CELL_BAUD      	115200   // Cell is on uart1
#define UART3_BAUD      115200

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart4;
UART_HandleTypeDef hlpuart1;




// Cell on UART1
void MX_USART1_UART_Init(uint32_t BaudRate, uint32_t StopBits, uint32_t HwFlowCtl)
{
	HAL_NVIC_DisableIRQ(USART1_IRQn);

	huart1.Instance = USART1;
	huart1.Init.BaudRate = BaudRate;  // must pre-configure the XBee to this baud before installing.  Xbee default is 9600.
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = StopBits;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = HwFlowCtl;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	if (HAL_UART_DeInit(&huart1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}


	int status = HAL_UART_Init(&huart1);

	if (status != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	//  /* polling idle frame Transmission */
	while ((USART1->ISR & USART_ISR_TC) != USART_ISR_TC) {
		/* add time out here for a robust application */
	}

	USART1->ICR = USART_ICR_TCCF;/* clear TC flag  0x65;// */
	USART1->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;/* enable TC interrupt */
	uint32_t isrflags = READ_REG(huart1.Instance->ISR);

	if ((isrflags & USART_ISR_ORE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart1, UART_CLEAR_OREF);
	}

	if ((isrflags & USART_ISR_FE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart1, UART_CLEAR_FEF);
	}

	NVIC_SetPriority(USART1_IRQn, 0);
	NVIC_EnableIRQ(USART1_IRQn);
}

// WiFi on UART2 - Initialization
void MX_USART2_UART_Init(uint32_t BaudRate, uint32_t StopBits, uint32_t HwFlowCtl)
{

	HAL_NVIC_DisableIRQ(USART2_IRQn);

	huart2.Instance = USART2;
	huart2.Init.BaudRate = BaudRate;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = StopBits;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = HwFlowCtl;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

#if 1
	if (HAL_UART_DeInit(&huart2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}
#endif


	if (HAL_UART_Init(&huart2) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}


// zg this may be the place to disable TX interrupts but keep RX interrupts
	USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

#if 1   // Bug Hunt
	/* polling idle frame Transmission */
	while ((USART2->ISR & USART_ISR_TC) != USART_ISR_TC) {
		/* add time out here for a robust application */
	}
#endif

	USART2->ICR = USART_ICR_TCCF;/* clear TC flag  0x65;// */
	USART2->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;/* enable TC interrupt */
	uint32_t isrflags = READ_REG(huart2.Instance->ISR);

	if ((isrflags & USART_ISR_ORE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart2, UART_CLEAR_OREF);
	}

	if ((isrflags & USART_ISR_FE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart2, UART_CLEAR_FEF);
	}

	NVIC_SetPriority(USART2_IRQn, 0);
	NVIC_EnableIRQ(USART2_IRQn);


}


// USART3 init function - RS485 and interrupts
// The PGA460 UART supports up to 115.2 kBaud, and is always formatted in 8 data bits, two stop bits,
// no parity, and no flow control.
void MX_USART3_UART_Init(uint32_t BaudRate, uint32_t StopBits)
{

	HAL_NVIC_DisableIRQ(USART3_IRQn);

	huart3.Instance = USART3;
	huart3.Init.BaudRate = BaudRate;

	huart3.Init.WordLength = UART_WORDLENGTH_8B;

	huart3.Init.StopBits = StopBits;  // to support the PGA460 on the level board in pass-thru mode

	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	/* bit time = 1 / baud_rate  in microseconds
	 * bit_time = 1 / 115200 = 8.68 us
	 * The SN65HVD111DR data sheet indicates (on page 7) that tPZH and tPZL are a max of 55 ns.
	 * The oversampling is 16, so the time units are 1/16 of a bit time.
	 * So  55 ns = N * (1/16 * 8.68 us)   N = 0.1.  Just use 11
	 */

	//if (HAL_RS485Ex_Init(&huart3, UART_DE_POLARITY_HIGH, 12, 12) != HAL_OK) {  // works but odd errors ?? requires delay
	if (HAL_RS485Ex_Init(&huart3, UART_DE_POLARITY_HIGH, 12, 0) != HAL_OK) {  //
	//if (HAL_RS485Ex_Init(&huart3, UART_DE_POLARITY_HIGH, 1, 0) != HAL_OK) {  // General UART error
	//if (HAL_RS485Ex_Init(&huart3, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK) {  // does not seem to work
		_Error_Handler(__FILE__, __LINE__);
	}

	USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	//  /* polling idle frame Transmission */
	while ((USART3->ISR & USART_ISR_TC) != USART_ISR_TC) {
		/* add time out here for a robust application */
	}

	USART3->ICR = USART_ICR_TCCF;/* clear TC flag  0x65;// */
	USART3->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;/* enable TC interrupt */
	uint32_t isrflags = READ_REG(huart3.Instance->ISR);

	if ((isrflags & USART_ISR_ORE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart3, UART_CLEAR_OREF);
	}

	if ((isrflags & USART_ISR_FE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart3, UART_CLEAR_FEF);
	}

	NVIC_SetPriority(USART3_IRQn, 0);
	NVIC_EnableIRQ(USART3_IRQn);
}

#ifdef ENABLE_UART4    // Velocity 1
void MX_UART4_UART_Init(uint32_t BaudRate, uint32_t StopBits)
{
	HAL_NVIC_DisableIRQ(UART4_IRQn);

	huart4.Instance = UART4;

	huart4.Init.BaudRate = BaudRate;  // must pre-configure the XBee to this baud before installing.  Xbee default is 9600.

	huart4.Init.WordLength = UART_WORDLENGTH_8B;

	huart4.Init.StopBits = StopBits;


	huart4.Init.Parity = UART_PARITY_NONE;
	huart4.Init.Mode = UART_MODE_TX_RX;
	huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;


	if (HAL_UART_DeInit(&huart4) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}


	int status = HAL_UART_Init(&huart4);

	if (status != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	UART4->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	//  /* polling idle frame Transmission */
	while ((UART4->ISR & USART_ISR_TC) != USART_ISR_TC) {
		/* add time out here for a robust application */
	}

	UART4->ICR = USART_ICR_TCCF;/* clear TC flag  0x65;// */
	UART4->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;/* enable TC interrupt */
	uint32_t isrflags = READ_REG(huart4.Instance->ISR);

	if ((isrflags & USART_ISR_ORE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart4, UART_CLEAR_OREF);
	}

	if ((isrflags & USART_ISR_FE) != RESET) {
		__HAL_UART_CLEAR_IT(&huart4, UART_CLEAR_FEF);
	}

	NVIC_SetPriority(UART4_IRQn, 0);
	NVIC_EnableIRQ(UART4_IRQn);
}
#endif

#ifdef ENABLE_LPUART1
/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_LPUART1_UART_Init(uint32_t BaudRate, uint32_t StopBits)
{
	HAL_NVIC_DisableIRQ(LPUART1_IRQn);

	hlpuart1.Instance = LPUART1;

	hlpuart1.Init.BaudRate = BaudRate;  // must pre-configure the XBee to this baud before installing.  Xbee default is 9600.

	hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;

	hlpuart1.Init.StopBits = StopBits;


	hlpuart1.Init.Parity = UART_PARITY_NONE;
	hlpuart1.Init.Mode = UART_MODE_TX_RX;
	hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	hlpuart1.Init.OverSampling = UART_OVERSAMPLING_16;
	hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;


	if (HAL_UART_DeInit(&hlpuart1) != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}


	int status = HAL_UART_Init(&hlpuart1);

	if (status != HAL_OK) {
		_Error_Handler(__FILE__, __LINE__);
	}

	LPUART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

	//  /* polling idle frame Transmission */
	while ((LPUART1->ISR & USART_ISR_TC) != USART_ISR_TC) {
		/* add time out here for a robust application */
	}

	LPUART1->ICR = USART_ICR_TCCF;/* clear TC flag  0x65;// */
	LPUART1->CR1 |= USART_CR1_TCIE | USART_CR1_RXNEIE;/* enable TC interrupt */
	uint32_t isrflags = READ_REG(hlpuart1.Instance->ISR);

	if ((isrflags & USART_ISR_ORE) != RESET) {
		__HAL_UART_CLEAR_IT(&hlpuart1, UART_CLEAR_OREF);
	}

	if ((isrflags & USART_ISR_FE) != RESET) {
		__HAL_UART_CLEAR_IT(&hlpuart1, UART_CLEAR_FEF);
	}

	NVIC_SetPriority(LPUART1_IRQn, 0);
	NVIC_EnableIRQ(LPUART1_IRQn);

}
#endif






void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};


	if (uartHandle->Instance == USART1)
	{
		__HAL_RCC_USART1_CLK_ENABLE();

		GPIO_InitStruct.Pin = GEN45_CELL_TX_Pin | GEN45_CELL_RX_Pin;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;           // From Mark Watson 7-15-2021
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		if (uartHandle->Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS)
		{
			GPIO_InitStruct.Pin =  GEN45_CELL_RTS_Pin | GEN45_CELL_CTS_Pin;
			HAL_GPIO_Init(GEN45_CELL_RTS_Port, &GPIO_InitStruct);
		}

	} else if (uartHandle->Instance == USART2)
	{
		__HAL_RCC_USART2_CLK_ENABLE();

		// Must be careful about pins and ports for the Gen 4 vs Gen 5.
		// In Gen 5, not all pins are on the same port.
		GPIO_InitStruct.Pin = GEN45_WIFI_TXD_Pin | GEN45_WIFI_RXD_Pin;

		GPIO_InitStruct.Pull  = GPIO_PULLUP;           // From Mark Watson 7-15-2021
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
		HAL_GPIO_Init(GEN45_WIFI_TXD_Port, &GPIO_InitStruct);

		if (isGen4())
		{
		  if (uartHandle->Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS)
		  {
			GPIO_InitStruct.Pin = GEN4_WIFI_RTS_Pin | GEN4_WIFI_CTS_Pin;
			HAL_GPIO_Init(GEN4_WIFI_RTS_Port, &GPIO_InitStruct);
		  }
		}

		if (isGen5())
		{
		  if (uartHandle->Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS)
		  {
			GPIO_InitStruct.Pin = GEN5_WIFI_RTS_Pin | GEN5_WIFI_CTS_Pin;
			HAL_GPIO_Init(GEN5_WIFI_RTS_Port, &GPIO_InitStruct);
		  }
		}

	} else if (uartHandle->Instance == USART3)
	{
		/* USART3 clock enable */
		__HAL_RCC_USART3_CLK_ENABLE();

		GPIO_InitStruct.Pin = GEN45_RS485_TXD_Pin | GEN45_RS485_RXD_Pin;   // PC4 and PC5
		GPIO_InitStruct.Pull  = GPIO_PULLUP;             // From Mark Watson 7-15-2021
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;    // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init(GEN45_RS485_TXD_Port, &GPIO_InitStruct);

        // for RS485 - PB1 = DE
		//HAL_GPIO_WritePin(RS485_DE_Port, RS485_DE_Pin, GPIO_PIN_RESET);  // disable TX

		GPIO_InitStruct.Pin = GEN45_RS485_DE_Pin;
		GPIO_InitStruct.Pull = GPIO_NOPULL;   // The SN65 has an internal pull-up already
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init(GEN45_RS485_DE_Port, &GPIO_InitStruct);

		// for RS485 - PE7 = (not RE)
		HAL_GPIO_WritePin(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin, GPIO_PIN_RESET);  // enable RX (active low)  Why?  Don't need to listen on this port until send something to PGA460...
		GPIO_InitStruct.Pin = GEN45_RS485_nRE_Pin;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;    // NOTE: There is NOT an alternate function for this pin
		GPIO_InitStruct.Alternate = 0;
		HAL_GPIO_Init(GEN45_RS485_nRE_Port, &GPIO_InitStruct);

	}
#ifdef ENABLE_UART4
	else if (uartHandle->Instance == UART4)
	{
		__HAL_RCC_UART4_CLK_ENABLE();

		GPIO_InitStruct.Pin = GEN5_VEL1_TXD_Pin | GEN5_VEL1_RXD_Pin;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;           // From Mark Watson 7-15-2021
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
		HAL_GPIO_Init(GEN5_VEL1_TXD_Port, &GPIO_InitStruct);

	}
#endif
#ifdef ENABLE_LPUART1
	else if (uartHandle->Instance == LPUART1)
	{
		__HAL_RCC_LPUART1_CLK_ENABLE();


		GPIO_InitStruct.Pin = GEN5_VEL2_RXD_Pin | GEN5_VEL2_TXD_Pin;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;           // From Mark Watson 7-15-2021
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // From Mark Watson 7-15-2021
		GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
		HAL_GPIO_Init(GEN5_VEL2_RXD_Port, &GPIO_InitStruct);
	}
#endif  // ENABLE_LPUART1
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
  /* Check the UART handle allocation */
  if (uartHandle == NULL) return;

	if (uartHandle->Instance == USART1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART1_CLK_DISABLE();
		NVIC_DisableIRQ(USART1_IRQn);

		gpio_unused_pin(GEN45_CELL_TX_Port,  GEN45_CELL_TX_Pin);
		gpio_unused_pin(GEN45_CELL_TX_Port,  GEN45_CELL_RX_Pin);
		gpio_unused_pin(GEN45_CELL_RTS_Port, GEN45_CELL_RTS_Pin);
		gpio_unused_pin(GEN45_CELL_RTS_Port, GEN45_CELL_CTS_Pin);



	} else if (uartHandle->Instance == USART2)
	{
		__HAL_RCC_USART2_CLK_DISABLE();
		NVIC_DisableIRQ(USART2_IRQn);

		//Gen5 pins and ports not the same as Gen4

		gpio_unused_pin(GEN45_WIFI_TXD_Port, GEN45_WIFI_TXD_Pin);  // WIFI_TXD
		gpio_unused_pin(GEN45_WIFI_TXD_Port, GEN45_WIFI_RXD_Pin);  // WIFI_RXD


		if (isGen4())
		{
			gpio_unused_pin(GEN4_WIFI_RTS_Port, GEN4_WIFI_RTS_Pin);
			gpio_unused_pin(GEN4_WIFI_RTS_Port, GEN4_WIFI_CTS_Pin);
		}

		if (isGen5())
		{
			gpio_unused_pin(GEN5_WIFI_RTS_Port, GEN5_WIFI_RTS_Pin);
			gpio_unused_pin(GEN5_WIFI_RTS_Port, GEN5_WIFI_CTS_Pin);
		}



	} else if (uartHandle->Instance == USART3)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART3_CLK_DISABLE();
		NVIC_DisableIRQ(USART3_IRQn);

		gpio_unused_pin(GEN45_RS485_TXD_Port, GEN45_RS485_TXD_Pin);
		gpio_unused_pin(GEN45_RS485_TXD_Port, GEN45_RS485_RXD_Pin);


		HAL_GPIO_WritePin(GEN45_RS485_DE_Port, GEN45_RS485_DE_Pin, GPIO_PIN_RESET);  // disable TX here for low-power mode
		HAL_GPIO_WritePin(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin, GPIO_PIN_SET);    // disable RX here for low-power mode

#if 0 // for RS485
		// Don't de-init the RE and DE pins to keep the RS485 chip in a low power mode
		HAL_GPIO_DeInit(GEN45_RS485_DE_Port, GEN45_RS485_DE_Pin);
		HAL_GPIO_DeInit(GEN45_RS485_nRE_Port, GEN45_RS485_nRE_Pin);
#endif
	}
#ifdef ENABLE_UART4
	else if (uartHandle->Instance == UART4)
	{
		/* Peripheral clock disable */
		__HAL_RCC_UART4_CLK_DISABLE();
		NVIC_DisableIRQ(UART4_IRQn);

		gpio_unused_pin(GEN5_VEL1_TXD_Port, GEN5_VEL1_TXD_Pin);
		gpio_unused_pin(GEN5_VEL1_TXD_Port, GEN5_VEL1_RXD_Pin);

	}
#endif
#ifdef ENABLE_LPUART1
	else if (uartHandle->Instance == LPUART1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_LPUART1_CLK_DISABLE();
		NVIC_DisableIRQ(LPUART1_IRQn);

		gpio_unused_pin(GEN5_VEL2_TXD_Port, GEN5_VEL2_TXD_Pin);
		gpio_unused_pin(GEN5_VEL2_TXD_Port, GEN5_VEL2_RXD_Pin);

	}
#endif
}


void check_and_report_uart_buffer_overflow(void)
{
	char msg[75];

	if (uart1_inbuf_max > UART1_INBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart1_inbuf (%d, %d)\n", uart1_inbuf_max, UART1_INBUF_SIZE);
		AppendStatusMessage(msg);
		uart1_inbuf_max = 0;  // clear the error to avoid future error messages
	}
	if (uart1_outbuf_max > UART1_OUTBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart1_outbuf (%d, %d)\n", uart1_outbuf_max, UART1_OUTBUF_SIZE);
		AppendStatusMessage(msg);
		uart1_outbuf_max = 0;  // clear the error to avoid future error messages
	}

	if (uart2_inbuf_max > UART2_INBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart2_inbuf (%d, %d)\n", uart2_inbuf_max, UART2_INBUF_SIZE);
		AppendStatusMessage(msg);
		uart2_inbuf_max = 0;  // clear the error to avoid future error messages
	}
	if (uart2_outbuf_max > UART2_OUTBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart2_outbuf (%d, %d)\n", uart2_outbuf_max, UART2_OUTBUF_SIZE);
		AppendStatusMessage(msg);
		uart2_outbuf_max = 0;  // clear the error to avoid future error messages
	}


	if (uart4_inbuf_max > UART3_INBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart4_inbuf (%d, %d)\n", uart4_inbuf_max, UART3_INBUF_SIZE);
		AppendStatusMessage(msg);
		uart4_inbuf_max = 0;  // clear the error to avoid future error messages
	}
	if (uart4_outbuf_max > UART3_OUTBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart4_outbuf (%d, %d)\n", uart4_outbuf_max, UART3_OUTBUF_SIZE);
		AppendStatusMessage(msg);
		uart4_outbuf_max = 0;  // clear the error to avoid future error messages
	}


	if (uart4_inbuf_max > UART4_INBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart4_inbuf (%d, %d)\n", uart4_inbuf_max, UART4_INBUF_SIZE);
		AppendStatusMessage(msg);
		uart4_inbuf_max = 0;  // clear the error to avoid future error messages
	}
	if (uart4_outbuf_max > UART4_OUTBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: uart4_outbuf (%d, %d)\n", uart4_outbuf_max, UART4_OUTBUF_SIZE);
		AppendStatusMessage(msg);
		uart4_outbuf_max = 0;  // clear the error to avoid future error messages
	}

	if (lpuart1_inbuf_max > LPUART1_INBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: lpuart1_inbuf (%d, %d)\n", lpuart1_inbuf_max, LPUART1_INBUF_SIZE);
		AppendStatusMessage(msg);
		lpuart1_inbuf_max = 0;  // clear the error to avoid future error messages
	}
	if (lpuart1_outbuf_max > LPUART1_OUTBUF_SIZE)
	{
		sprintf(msg, "\nBUFFER OVERFLOW ERROR: lpuart1_outbuf (%d, %d)\n", lpuart1_outbuf_max, LPUART1_OUTBUF_SIZE);
		AppendStatusMessage(msg);
		lpuart1_outbuf_max = 0;  // clear the error to avoid future error messages
	}

}
