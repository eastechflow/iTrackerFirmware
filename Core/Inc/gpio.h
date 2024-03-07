/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2019 Eastech Flow Controls, Inc.
*
* OWNER:
* Eastech Flow Controls, Inc.
* 4250 S. 76th E. Avenue
* Tulsa, Oklahoma 74145
*
* toll-free:   (800) 226-3569
* phone:   (918) 664-1212
* fax:   (918) 664-8494
* email:   info@eastechflow.com
* website:   http://eastechflow.com/
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

#ifndef __gpio_H
#define __gpio_H

#include "stm32l4xx_hal.h"

#define PA0   GPIOA, GPIO_PIN_0
#define PA1   GPIOA, GPIO_PIN_1
#define PA2   GPIOA, GPIO_PIN_2
#define PA3   GPIOA, GPIO_PIN_3
#define PA4   GPIOA, GPIO_PIN_4
#define PA5   GPIOA, GPIO_PIN_5
#define PA6   GPIOA, GPIO_PIN_6
#define PA7   GPIOA, GPIO_PIN_7
#define PA8   GPIOA, GPIO_PIN_8
#define PA9   GPIOA, GPIO_PIN_9
#define PA10  GPIOA, GPIO_PIN_10
#define PA11  GPIOA, GPIO_PIN_11
#define PA12  GPIOA, GPIO_PIN_12
#define PA13  GPIOA, GPIO_PIN_13
#define PA14  GPIOA, GPIO_PIN_14
#define PA15  GPIOA, GPIO_PIN_15

#define PB0   GPIOB, GPIO_PIN_0
#define PB1   GPIOB, GPIO_PIN_1
#define PB2   GPIOB, GPIO_PIN_2
#define PB3   GPIOB, GPIO_PIN_3
#define PB4   GPIOB, GPIO_PIN_4
#define PB5   GPIOB, GPIO_PIN_5
#define PB6   GPIOB, GPIO_PIN_6
#define PB7   GPIOB, GPIO_PIN_7
#define PB8   GPIOB, GPIO_PIN_8
#define PB9   GPIOB, GPIO_PIN_9
#define PB10  GPIOB, GPIO_PIN_10
#define PB11  GPIOB, GPIO_PIN_11
#define PB12  GPIOB, GPIO_PIN_12
#define PB13  GPIOB, GPIO_PIN_13
#define PB14  GPIOB, GPIO_PIN_14
#define PB15  GPIOB, GPIO_PIN_15

#define PC0   GPIOC, GPIO_PIN_0
#define PC1   GPIOC, GPIO_PIN_1
#define PC2   GPIOC, GPIO_PIN_2
#define PC3   GPIOC, GPIO_PIN_3
#define PC4   GPIOC, GPIO_PIN_4
#define PC5   GPIOC, GPIO_PIN_5
#define PC6   GPIOC, GPIO_PIN_6
#define PC7   GPIOC, GPIO_PIN_7
#define PC8   GPIOC, GPIO_PIN_8
#define PC9   GPIOC, GPIO_PIN_9
#define PC10  GPIOC, GPIO_PIN_10
#define PC11  GPIOC, GPIO_PIN_11
#define PC12  GPIOC, GPIO_PIN_12
#define PC13  GPIOC, GPIO_PIN_13
#define PC14  GPIOC, GPIO_PIN_14
#define PC15  GPIOC, GPIO_PIN_15

#define PD0   GPIOD, GPIO_PIN_0
#define PD1   GPIOD, GPIO_PIN_1
#define PD2   GPIOD, GPIO_PIN_2
#define PD3   GPIOD, GPIO_PIN_3
#define PD4   GPIOD, GPIO_PIN_4
#define PD5   GPIOD, GPIO_PIN_5
#define PD6   GPIOD, GPIO_PIN_6
#define PD7   GPIOD, GPIO_PIN_7
#define PD8   GPIOD, GPIO_PIN_8
#define PD9   GPIOD, GPIO_PIN_9
#define PD10  GPIOD, GPIO_PIN_10
#define PD11  GPIOD, GPIO_PIN_11
#define PD12  GPIOD, GPIO_PIN_12
#define PD13  GPIOD, GPIO_PIN_13
#define PD14  GPIOD, GPIO_PIN_14
#define PD15  GPIOD, GPIO_PIN_15

#define PE0   GPIOE, GPIO_PIN_0
#define PE1   GPIOE, GPIO_PIN_1
#define PE2   GPIOE, GPIO_PIN_2
#define PE3   GPIOE, GPIO_PIN_3
#define PE4   GPIOE, GPIO_PIN_4
#define PE5   GPIOE, GPIO_PIN_5
#define PE6   GPIOE, GPIO_PIN_6
#define PE7   GPIOE, GPIO_PIN_7
#define PE8   GPIOE, GPIO_PIN_8
#define PE9   GPIOE, GPIO_PIN_9
#define PE10  GPIOE, GPIO_PIN_10
#define PE11  GPIOE, GPIO_PIN_11
#define PE12  GPIOE, GPIO_PIN_12
#define PE13  GPIOE, GPIO_PIN_13
#define PE14  GPIOE, GPIO_PIN_14
#define PE15  GPIOE, GPIO_PIN_15


//#define ENABLE_USB

/***************  GEN4 PORT A  ****************/
#define GEN4_WIFI_CTS_Pin            GPIO_PIN_0
#define GEN4_WIFI_CTS_Port           GPIOA

#define GEN4_WIFI_RTS_Pin            GPIO_PIN_1
#define GEN4_WIFI_RTS_Port           GPIOA

/*  GEN45                            GPIO_PIN_2   */
/*  GEN45                            GPIO_PIN_3   */

/*                                   GPIO_PIN_4  ILIM - configured as analog, that is UNUSED */

#define GEN4_VH_SW_ADC_EN_Pin        GPIO_PIN_5
#define GEN4_VH_SW_ADC_EN_Port       GPIOA

/*                                   GPIO_PIN_6  NTC_EN - configured as analog, that is UNUSED */
/*                                   GPIO_PIN_7  SW_PWR_EN  */
/*                                   GPIO_PIN_8  uSD_DETECT  */

    /**USB_OTG_FS GPIO Configuration (Same in both Gen4 & Gen5)
    PA9      ------> USB_OTG_FS_VBUS
    PA10     ------> USB_OTG_FS_ID
    PA11     ------> USB_OTG_FS_DM
    PA12     ------> USB_OTG_FS_DP
    PA13     ------> SWD_DIO
    PA14     ------> SWD_CLK
    */

/*  UNUSED                           GPIO_PIN_15   */

/***************  GEN4 PORT B  ****************/

/*  GEN45                            GPIO_PIN_0   */
/*  GEN45                            GPIO_PIN_1   */
/*  GEN45                            GPIO_PIN_2   */
/*  GEN45                            GPIO_PIN_3   */
/*  GEN45                            GPIO_PIN_4   */
/*  GEN45                            GPIO_PIN_5   */
/*  GEN45                            GPIO_PIN_6   */
/*  GEN45                            GPIO_PIN_7   */
/*  GEN45                            GPIO_PIN_8   */
/*  GEN45                            GPIO_PIN_9   */
/*  GEN45                            GPIO_PIN_10   */

#define GEN4_Sensor_IN_Pin           GPIO_PIN_11
#define GEN4_Sensor_IN_Port          GPIOB

#define GEN4_Flash_Reset_Pin         GPIO_PIN_12     // The Gen4 firmware did not appear to use this pin
#define GEN4_Flash_Reset_Port        GPIOB


/*  GEN45                            GPIO_PIN_13   */
/*  GEN45                            GPIO_PIN_14   */

#define GEN4_CELL_Reset_Pin          GPIO_PIN_15
#define GEN4_CELL_Reset_Port         GPIOB

/***************  GEN4 PORT C  ****************/
/*  GEN45                            GPIO_PIN_0   */

#define GEN4_VH_PWR_ADC_Pin          GPIO_PIN_1
#define GEN4_VH_PWR_ADC_Port         GPIOC

#define GEN4_Temp_SNR_ADC_Pin        GPIO_PIN_2
#define GEN4_Temp_SNR_ADC_Port       GPIOC

/*  UNUSED                           GPIO_PIN_3   */
//#define GEN4_Temp_NTC_ADC_Pin        GPIO_PIN_3
//#define GEN4_Temp_NTC_ADC_Port       GPIOC

/*  GEN45                            GPIO_PIN_4   */
/*  GEN45                            GPIO_PIN_5   */
/*  GEN45                            GPIO_PIN_6   */
/*  GEN45                            GPIO_PIN_7   */
/*  GEN45                            GPIO_PIN_8   */
/*  GEN45                            GPIO_PIN_9   */
/*  GEN45                            GPIO_PIN_10   */
/*  GEN45                            GPIO_PIN_11   */
/*  GEN45                            GPIO_PIN_12   */
/*  GEN45                            GPIO_PIN_13   */  // technically connects to external port, but was never used
/*  GEN45                            GPIO_PIN_14   */
/*  GEN45                            GPIO_PIN_15   */



/***************  GEN4 PORT D  ****************/

/*  UNUSED                           GPIO_PIN_0   */
/*  UNUSED                           GPIO_PIN_1   */
/*  UNUSED                           GPIO_PIN_2   */
/*  UNUSED                           GPIO_PIN_3   */
/*  UNUSED                           GPIO_PIN_4   */
/*  UNUSED                           GPIO_PIN_5   */
/*  UNUSED                           GPIO_PIN_6   */

#define GEN4_CELL_RI_Pin             GPIO_PIN_7
#define GEN4_CELL_RI_Port            GPIOD

/*  UNUSED                           GPIO_PIN_8   */
/*  UNUSED                           GPIO_PIN_9   */
/*  UNUSED                           GPIO_PIN_10   */
/*  UNUSED                           GPIO_PIN_11   */

/*  GEN45                            GPIO_PIN_12   */

/*  UNUSED                           GPIO_PIN_13   */
/*  UNUSED                           GPIO_PIN_14   */

/*  GEN45                            GPIO_PIN_15   */


/***************  GEN4 PORT E  ****************/
#define GEN4_Zentri_OOB_Pin            GPIO_PIN_0
#define GEN4_Zentri_OOB_Port           GPIOE

#define GEN4_Zentri_Reset_Pin          GPIO_PIN_1
#define GEN4_Zentri_Reset_Port         GPIOE

#define GEN4_Zentri_WU_Pin             GPIO_PIN_2
#define GEN4_Zentri_WU_Port            GPIOE

#define GEN4_Zentri_SC_Pin             GPIO_PIN_3
#define GEN4_Zentri_SC_Port            GPIOE

#define GEN4_Zentri_CMD_STR_Pin        GPIO_PIN_4
#define GEN4_Zentri_CMD_STR_Port       GPIOE

/*  UNUSED                           GPIO_PIN_5   */   // Unused on Gen4 and Gen5 iTracker5, but used on SewerWatch

#define GEN5_SEWERWATCH_TEMP_DRV_Pin        GPIO_PIN_5
#define GEN5_SEWERWATCH_TEMP_DRV_Port       GPIOE



/*  UNUSED                           GPIO_PIN_6   */   // Unused on Gen4 and Gen5 iTracker5, but used on SewerWatch

#define GEN5_SEWERWATCH_REED_SW_INTR_Pin        GPIO_PIN_6
#define GEN5_SEWERWATCH_REED_SW_INTR_Port       GPIOE



/*  GEN45                            GPIO_PIN_7   */    // previously known as EXT_RE_Pin

#define GEN4_Sensor_PG_Pin           GPIO_PIN_8
#define GEN4_Sensor_PG_GPIO_Port     GPIOE

/*  GEN45                            GPIO_PIN_9   */   // Same as Gen5 but singled ended drive

/*  GEN45 - QSPI                     GPIO_PIN_10   */  // PINS PE10 - PE15 are used by QSPI for SD CARD
/*  GEN45 - QSPI                     GPIO_PIN_11   */
/*  GEN45 - QSPI                     GPIO_PIN_12   */
/*  GEN45 - QSPI                     GPIO_PIN_13   */
/*  GEN45 - QSPI                     GPIO_PIN_14   */
/*  GEN45 - QSPI                     GPIO_PIN_15   */

/***************  GEN5 PORT A  ****************/
#define GEN5_VEL1_TXD_Pin    GPIO_PIN_0            // UART4 - Velocity 1
#define GEN5_VEL1_TXD_Port   GPIOA

#define GEN5_VEL1_RXD_Pin    GPIO_PIN_1            // UART4 - Velocity 1
#define GEN5_VEL1_RXD_Port   GPIOA

#define GEN45_WIFI_TXD_Pin   GPIO_PIN_2             // UART2 - WiFi
#define GEN45_WIFI_TXD_Port  GPIOA

#define GEN45_WIFI_RXD_Pin   GPIO_PIN_3             // UART2 - WiFi
#define GEN45_WIFI_RXD_Port  GPIOA

/*                           GPIO_PIN_4  UNUSED */

#define GEN5_HIGH_SIG_OUT_Pin     GPIO_PIN_5
#define GEN5_HIGH_SIG_OUT_Port    GPIOA

#define GEN5_Temp_POWER_Pin       GPIO_PIN_6
#define GEN5_Temp_POWER_Port      GPIOA

#define GEN45_VH_PWR_EN_Pin       GPIO_PIN_7   // aka SENSOR_PWR_EN on schematic
#define GEN45_VH_PWR_EN_Port      GPIOA

#define GEN45_uSD_DETECT_Pin      GPIO_PIN_8
#define GEN45_uSD_DETECT_Port     GPIOA

#define SD_DETECT_PIN             GPIO_PIN_8   // For HAL library reference, see also GEN45_uSD_DETECT_Pin
#define SD_DETECT_GPIO_PORT       GPIOA


    /**USB_OTG_FS GPIO Configuration (Same in both Gen4 & Gen5)
    PA9      ------> USB_OTG_FS_VBUS
    PA10     ------> USB_OTG_FS_ID
    PA11     ------> USB_OTG_FS_DM
    PA12     ------> USB_OTG_FS_DP
    PA13     ------> SWD_DIO
    PA14     ------> SWD_CLK
    */

#define GEN5_WIFI_PWR_EN_Pin    GPIO_PIN_15
#define GEN5_WIFI_PWR_EN_Port   GPIOA



/***************  GEN5 PORT B  ****************/
#define GEN45_ON_EXT_PWR_Pin       GPIO_PIN_0           // ExtPwrIn note: PB0 is wired to PB2 as well...why?
#define GEN45_ON_EXT_PWR_Port      GPIOB

#define GEN45_RS485_DE_Pin         GPIO_PIN_1           // UART3 - RS-485
#define GEN45_RS485_DE_Port        GPIOB

/*                                 GPIO_PIN_2  UNUSED */

#define GEN45_CELL_RTS_Pin         GPIO_PIN_3           // UART1 - Cell      Note: Gen4 had CTS,RTS swapped with respect to processor pins, this code base is corrected for that
#define GEN45_CELL_RTS_Port        GPIOB

#define GEN45_CELL_CTS_Pin         GPIO_PIN_4           // UART1 - Cell      Note: Gen4 had CTS,RTS swapped with respect to processor pins, this code base is corrected for that
#define GEN45_CELL_CTS_Port        GPIOB

#define GEN5_RS485_PWR_EN_Pin      GPIO_PIN_5          // RS485_PWR_EN
#define GEN5_RS485_PWR_EN_Port     GPIOB

#define GEN45_CELL_TX_Pin          GPIO_PIN_6           // UART1 - Cell
#define GEN45_CELL_TX_Port         GPIOB

#define GEN45_CELL_RX_Pin          GPIO_PIN_7           // UART1 - Cell
#define GEN45_CELL_RX_Port         GPIOB

/*                                 GPIO_PIN_8  UNUSED */

#define GEN5_SPI2_NSS_Pin          GPIO_PIN_9          // SPI2_NSS
#define GEN5_SPI2_NSS_Port         GPIOB

#define GEN5_VEL2_RXD_Pin          GPIO_PIN_10          // LPUART1 - Velocity 2
#define GEN5_VEL2_RXD_Port         GPIOB

#define GEN5_VEL2_TXD_Pin          GPIO_PIN_11          // LPUART1 - Velocity 2
#define GEN5_VEL2_TXD_Port         GPIOB

/*                                 GPIO_PIN_12  UNUSED */

#define GEN45_I2C_SCL_Pin          GPIO_PIN_13          // I2C
#define GEN45_I2C_SCL_Port         GPIOB

#define GEN45_I2C_SDA_Pin          GPIO_PIN_14          // I2C
#define GEN45_I2C_SDA_Port         GPIOB

#define GEN5_SPI2_MOSI_Pin         GPIO_PIN_15          // SPI2_MOSI
#define GEN5_SPI2_MOSI_Port        GPIOB

// PORT B UNUSED  PB2, PB5, PB8, PB9, PB12, PB15

/***************  GEN5 PORT C  ****************/
#define GEN45_OPAMP_SD_Pin         GPIO_PIN_0       // nSHDN_OPAM
#define GEN45_OPAMP_SD_Port        GPIOC

//#define PROTOTYPE_BOARD    // For protoboard SN 3 in Zack's office (processor pin 17)

#ifdef PROTOTYPE_BOARD
#define GEN5_Temp_SNSR_Pin         GPIO_PIN_2   // For protoboard SN 3 in Zack's office (processor pin 17)
#else
#define GEN5_Temp_SNSR_Pin         GPIO_PIN_1  // For production boards that have SPI2  (processor pin 16)
#endif
#define GEN5_Temp_SNSR_Port        GPIOC

#ifdef PROTOTYPE_BOARD

#else
#define GEN5_SPI2_MISO_Pin         GPIO_PIN_2   // Scan code for this symbol when turning on again
#define GEN5_SPI2_MISO_Port        GPIOC
#endif

#define GEN5_BOARD_ID_ADC_Pin      GPIO_PIN_3       // Note: Gen4 reserved this for TEMP_NTC_ADC but never used it
#define GEN5_BOARD_ID_ADC_Port     GPIOC

#define GEN5_SEWERWATCH_TEMP_AD_Pin      GPIO_PIN_3       // Note: SewerWatch has different board id, uses this for temp sensor
#define GEN5_SEWERWATCH_TEMP_AD_Port     GPIOC

#define GEN45_RS485_TXD_Pin        GPIO_PIN_4           // UART3-TX - RS-485
#define GEN45_RS485_TXD_Port       GPIOC

#define GEN45_RS485_RXD_Pin        GPIO_PIN_5           // UART3-RX - RS-485
#define GEN45_RS485_RXD_Port       GPIOC

#define GEN45_STATUS_LED_Pin       GPIO_PIN_6
#define GEN45_STATUS_LED_Port      GPIOC

#define GEN5_STATUS2_LED_Pin       GPIO_PIN_7
#define GEN5_STATUS2_LED_Port      GPIOC

#define GEN45_uSD_D0_Pin           GPIO_PIN_8
#define GEN45_uSD_D0_Port          GPIOC

#define GEN45_uSD_D1_Pin           GPIO_PIN_9
#define GEN45_uSD_D1_Port          GPIOC

#define GEN45_uSD_D2_Pin           GPIO_PIN_10
#define GEN45_uSD_D2_Port          GPIOC

#define GEN45_uSD_D3_Pin           GPIO_PIN_11
#define GEN45_uSD_D3_Port          GPIOC

#define GEN45_uSD_CLK_Pin          GPIO_PIN_12
#define GEN45_uSD_CLK_Port         GPIOC

/*                                 GPIO_PIN_13  UNUSED */

/*                                 GPIO_PIN_14  used for oscillator */
/*                                 GPIO_PIN_15  used for oscillator */


/***************  GEN5 PORT D  ****************/
#define GEN5_SENSOR_PG_Pin      GPIO_PIN_0
#define GEN5_SENSOR_PG_Port     GPIOD

#define GEN5_SPI2_SCK_Pin       GPIO_PIN_1
#define GEN5_SPI2_SCK_Port      GPIOD

#define GEN45_uSD_CMD_Pin        GPIO_PIN_2
#define GEN45_uSD_CMD_Port       GPIOD

#define GEN5_WIFI_CTS_Pin       GPIO_PIN_3             // UART2 - WiFi
#define GEN5_WIFI_CTS_Port      GPIOD

#define GEN5_WIFI_RTS_Pin       GPIO_PIN_4             // UART2 - WiFi
#define GEN5_WIFI_RTS_Port      GPIOD

#define GEN5_VELOCITY1_EN_Pin   GPIO_PIN_5             // Velocity 1
#define GEN5_VELOCITY1_EN_Port  GPIOD

#define GEN5_VELOCITY2_EN_Pin   GPIO_PIN_6
#define GEN5_VELOCITY2_EN_Port  GPIOD

#define GEN5_WIFI_AUX1_Pin      GPIO_PIN_7
#define GEN5_WIFI_AUX1_Port     GPIOD

#define GEN5_VDD_3V3_SYNC_Pin   GPIO_PIN_8
#define GEN5_VDD_3V3_SYNC_Port  GPIOD

#define GEN5_CELL_AUX1_Pin      GPIO_PIN_9       // UNUSED in iTracker 4, but some early version COMM boards had R1 populated and would not detect SIM unless this is HI at power on
#define GEN5_CELL_AUX1_Port     GPIOD

#define GEN5_BOARD_ID_0_Pin      GPIO_PIN_10     // UNUSED in iTracker 5, was floating; First used in SewerWatch 1.0
#define GEN5_BOARD_ID_0_Port     GPIOD

#define GEN5_BOARD_ID_1_Pin      GPIO_PIN_11     // UNUSED in iTracker 5, was floating; First used in SewerWatch 1.0
#define GEN5_BOARD_ID_1_Port     GPIOD

#define GEN45_Bat_Alarm_Pin     GPIO_PIN_12      // This was never used in Gen4 firmware
#define GEN45_Bat_Alarm_Port    GPIOD

#define GEN5_BOARD_ID_3_Pin      GPIO_PIN_13     // UNUSED in iTracker 5, was floating; First used in SewerWatch 1.0
#define GEN5_BOARD_ID_3_Port     GPIOD

#define GEN5_BOARD_ID_4_Pin      GPIO_PIN_14     // UNUSED in iTracker 5, was floating; First used in SewerWatch 1.0
#define GEN5_BOARD_ID_4_Port     GPIOD

#define GEN45_CELL_PWR_EN_Pin    GPIO_PIN_15
#define GEN45_CELL_PWR_EN_Port   GPIOD



/***************  GEN5 PORT E  ****************/
#define GEN5_AUX1_Pin            GPIO_PIN_0
#define GEN5_AUX1_Port           GPIOE

#define GEN5_AUX2_Pin            GPIO_PIN_1
#define GEN5_AUX2_Port           GPIOE

#define GEN5_AUX3_Pin            GPIO_PIN_2
#define GEN5_AUX3_Port           GPIOE

#define GEN5_AUX4_Pin            GPIO_PIN_3
#define GEN5_AUX4_Port           GPIOE

#define GEN5_AUX_EN_Pin          GPIO_PIN_4
#define GEN5_AUX_EN_Port         GPIOE

/*                          GPIO_PIN_5  UNUSED */   // Used by SewerWatch TEMP_DRV
/*                          GPIO_PIN_6  UNUSED */

#define GEN45_RS485_nRE_Pin       GPIO_PIN_7           // UART3 - RS-485  (not RE) active low
#define GEN45_RS485_nRE_Port      GPIOE

#define GEN5_SENSOR_DRVn_Pin     GPIO_PIN_8
#define GEN5_SENSOR_DRVn_Port    GPIOE

#define GEN45_SENSOR_DRVp_Pin     GPIO_PIN_9
#define GEN45_SENSOR_DRVp_Port    GPIOE

// PINS PE10 - PE15 are used by QSPI for SD CARD

void MX_GPIO_Init(void);

void gpio_unused_pin(GPIO_TypeDef  *Port, uint32_t Pin);

#endif
