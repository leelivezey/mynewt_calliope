/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef MYPROJ_LED_MATRIX_DRIVER_H
#define MYPROJ_LED_MATRIX_DRIVER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <nrf51.h>

#ifndef HAVE_BOOLEAN
typedef int boolean;
#endif
#ifndef FALSE			/* in case these macros already exist */
#define FALSE	0		/* values of boolean */
#endif
#ifndef TRUE
#define TRUE	1
#endif

extern int print_string(char* text, boolean loop);
extern int print_char(char ch, boolean blink);
extern int set_pixel_at_xy(uint8_t x, uint8_t y);
extern bool isScrolling();
extern void showIntAs5Digits(uint16_t value);

#define GPIO_OF_ROW_0 (13) // muss HIGH sein. 3 bits
#define GPIO_OF_COL_0 (4)  // muss LOW sein, damit LED leuchtet, 9 bits

#define ROW_MASK   0b1110000000000000 // not used
#define COL_MASK   0b0001111111110000

#define     __O     volatile             /*!< Defines 'write only' permissions                */
#define     __IO    volatile             /*!< Defines 'read / write' permissions              */
#define   __I     volatile const       /*!< Defines 'read only' permissions                 */

//typedef struct {                                    /*!< GPIO Structure                                                        */
//    __I  uint32_t  RESERVED0[321];
//    __IO uint32_t  OUT;                               /*!< Write GPIO port.                                                      */
//    __IO uint32_t  OUTSET;                            /*!< Set individual bits in GPIO port.                                     */
//    __IO uint32_t  OUTCLR;                            /*!< Clear individual bits in GPIO port.                                   */
//    __I  uint32_t  IN;                                /*!< Read GPIO port.                                                       */
//    __IO uint32_t  DIR;                               /*!< Direction of GPIO pins.                                               */
//    __IO uint32_t  DIRSET;                            /*!< DIR set register.                                                     */
//    __IO uint32_t  DIRCLR;                            /*!< DIR clear register.                                                   */
//    __I  uint32_t  RESERVED1[120];
//    __IO uint32_t  PIN_CNF[32];                       /*!< Configuration of GPIO pins.                                           */
//} NRF_GPIO_Type;

#define NRF_GPIO_BASE                   0x50000000UL
#define NRF_GPIO                        ((NRF_GPIO_Type           *) NRF_GPIO_BASE)


/* Register: GPIO_PIN_CNF */
/* Description: Configuration of GPIO pins. */

/* Bits 17..16 : Pin sensing mechanism. */
#define GPIO_PIN_CNF_SENSE_Pos (16UL) /*!< Position of SENSE field. */
#define GPIO_PIN_CNF_SENSE_Msk (0x3UL << GPIO_PIN_CNF_SENSE_Pos) /*!< Bit mask of SENSE field. */
#define GPIO_PIN_CNF_SENSE_Disabled (0x00UL) /*!< Disabled. */
#define GPIO_PIN_CNF_SENSE_High (0x02UL) /*!< Wakeup on high level. */
#define GPIO_PIN_CNF_SENSE_Low (0x03UL) /*!< Wakeup on low level. */

/* Bits 10..8 : Drive configuration. */
#define GPIO_PIN_CNF_DRIVE_Pos (8UL) /*!< Position of DRIVE field. */
#define GPIO_PIN_CNF_DRIVE_Msk (0x7UL << GPIO_PIN_CNF_DRIVE_Pos) /*!< Bit mask of DRIVE field. */
#define GPIO_PIN_CNF_DRIVE_S0S1 (0x00UL) /*!< Standard '0', Standard '1'. */
#define GPIO_PIN_CNF_DRIVE_H0S1 (0x01UL) /*!< High '0', Standard '1'. */
#define GPIO_PIN_CNF_DRIVE_S0H1 (0x02UL) /*!< Standard '0', High '1'. */
#define GPIO_PIN_CNF_DRIVE_H0H1 (0x03UL) /*!< High '0', High '1'. */
#define GPIO_PIN_CNF_DRIVE_D0S1 (0x04UL) /*!< Disconnected '0', Standard '1'. */
#define GPIO_PIN_CNF_DRIVE_D0H1 (0x05UL) /*!< Disconnected '0', High '1'. */
#define GPIO_PIN_CNF_DRIVE_S0D1 (0x06UL) /*!< Standard '0', Disconnected '1'. */
#define GPIO_PIN_CNF_DRIVE_H0D1 (0x07UL) /*!< High '0', Disconnected '1'. */

/* Bits 3..2 : Pull-up or -down configuration. */
#define GPIO_PIN_CNF_PULL_Pos (2UL) /*!< Position of PULL field. */
#define GPIO_PIN_CNF_PULL_Msk (0x3UL << GPIO_PIN_CNF_PULL_Pos) /*!< Bit mask of PULL field. */
#define GPIO_PIN_CNF_PULL_Disabled (0x00UL) /*!< No pull. */
#define GPIO_PIN_CNF_PULL_Pulldown (0x01UL) /*!< Pulldown on pin. */
#define GPIO_PIN_CNF_PULL_Pullup (0x03UL) /*!< Pullup on pin. */

/* Bit 1 : Connect or disconnect input path. */
#define GPIO_PIN_CNF_INPUT_Pos (1UL) /*!< Position of INPUT field. */
#define GPIO_PIN_CNF_INPUT_Msk (0x1UL << GPIO_PIN_CNF_INPUT_Pos) /*!< Bit mask of INPUT field. */
#define GPIO_PIN_CNF_INPUT_Connect (0UL) /*!< Connect input pin. */
#define GPIO_PIN_CNF_INPUT_Disconnect (1UL) /*!< Disconnect input pin. */

/* Bit 0 : Pin direction. */
#define GPIO_PIN_CNF_DIR_Pos (0UL) /*!< Position of DIR field. */
#define GPIO_PIN_CNF_DIR_Msk (0x1UL << GPIO_PIN_CNF_DIR_Pos) /*!< Bit mask of DIR field. */
#define GPIO_PIN_CNF_DIR_Input (0UL) /*!< Configure pin as an input pin. */
#define GPIO_PIN_CNF_DIR_Output (1UL) /*!< Configure pin as an output pin. */

#endif //MYPROJ_LED_MATRIX_DRIVER_H
