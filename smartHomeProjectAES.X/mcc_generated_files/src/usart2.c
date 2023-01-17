/**
  @Company
    Microchip Technology Inc.

  @Description
    This Source file provides APIs.
    Generation Information :
    Driver Version    :   1.0.0
*/
/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/


#include "../include/usart2.h"

#if defined(__GNUC__)

int USART2_printCHAR(char character, FILE *stream)
{
    USART2_Write(character);
    return 0;
}

FILE USART2_stream = FDEV_SETUP_STREAM(USART2_printCHAR, NULL, _FDEV_SETUP_WRITE);

#elif defined(__ICCAVR__)

int putchar(int outChar)
{
    USART0_Write(outChar);
    return outChar;
}
#endif

void USART2_Initialize()
{
    //set baud rate register
    USART2.BAUD = (uint16_t)USART2_BAUD_RATE(9600);
	
    //RXCIE disabled; TXCIE disabled; DREIE disabled; RXSIE disabled; LBME disabled; ABEIE disabled; RS485 OFF; 
    USART2.CTRLA = 0x00;
	
    //RXEN enabled; TXEN enabled; SFDEN disabled; ODME disabled; RXMODE NORMAL; MPCM disabled; 
    USART2.CTRLB = 0xC0;
	
    //CMODE ASYNCHRONOUS; PMODE DISABLED; SBMODE 1BIT; CHSIZE 8BIT; UDORD disabled; UCPHA disabled; 
    USART2.CTRLC = 0x03;
	
    //DBGCTRL_DBGRUN
    USART2.DBGCTRL = 0x00;
	
    //EVCTRL_IREI
    USART2.EVCTRL = 0x00;
	
    //RXPLCTRL_RXPL
    USART2.RXPLCTRL = 0x00;
	
    //TXPLCTRL_TXPL
    USART2.TXPLCTRL = 0x00;
	

#if defined(__GNUC__)
    stdout = &USART2_stream;
#endif

}

void USART2_Enable()
{
    USART2.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}

void USART2_EnableRx()
{
    USART2.CTRLB |= USART_RXEN_bm;
}

void USART2_EnableTx()
{
    USART2.CTRLB |= USART_TXEN_bm;
}

void USART2_Disable()
{
    USART2.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
}

uint8_t USART2_GetData()
{
    return USART2.RXDATAL;
}

bool USART2_IsTxReady()
{
    return (USART2.STATUS & USART_DREIF_bm);
}

bool USART2_IsRxReady()
{
    return (USART2.STATUS & USART_RXCIF_bm);
}

bool USART2_IsTxBusy()
{
    return (!(USART2.STATUS & USART_TXCIF_bm));
}

bool USART2_IsTxDone()
{
    return (USART2.STATUS & USART_TXCIF_bm);
}

uint8_t USART2_Read()
{
    while (!(USART2.STATUS & USART_RXCIF_bm))
            ;
    return USART2.RXDATAL;
}

void USART2_Write(const uint8_t data)
{
    while (!(USART2.STATUS & USART_DREIF_bm))
            ;
    USART2.TXDATAL = data;
}