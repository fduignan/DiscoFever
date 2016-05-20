#include "stm32f303xx.h"
#include "serial.h"
// Serial comms routine for the stm32f303 nucleo board.
// makes use of usart2.  Pins PA2 and PA3 are used for transmission/reception
// Defines a new version of puts: e(mbedded)puts and egets
// Similar to puts and gets in standard C however egets checks the size
// of the input buffer.  This could be extended to include a timeout quite easily.
// Written by Frank Duignan
// 

// define the size of the communications buffer (adjust to suit)
#define MAXBUFFER   64
typedef struct tagComBuffer{
    unsigned char Buffer[MAXBUFFER];
    unsigned Head,Tail;
    unsigned Count;
} ComBuffer;

static ComBuffer ComRXBuffer,ComTXBuffer;

int PutBuf(ComBuffer  *Buf,unsigned char Data);
unsigned char GetBuf(ComBuffer  *Buf);
unsigned GetBufCount(ComBuffer  *Buf);
int ReadCom(int Max,unsigned char *Buffer);
int WriteCom(int Count,unsigned char *Buffer);


void usart_tx (void);
void usart_rx (void);
static unsigned ComOpen;
static unsigned ComError;
static unsigned ComBusy;


int ReadCom(int Max,unsigned char *Buffer)
{
// Read up to Max bytes from the communications buffer
// into Buffer.  Return number of bytes read
	unsigned i;
  	if (!ComOpen)
    	return (-1);
	i=0;
	while ((i < Max-1) && (GetBufCount(&ComRXBuffer)))
		Buffer[i++] = GetBuf(&ComRXBuffer);
	if (i>0)
	{
		Buffer[i]=0;
		return(i);
	}
	else {
		return(0);
	}	
};
int WriteCom(int Count,unsigned char *Buffer)
{
// Writes Count bytes from Buffer into the the communications TX buffer
// returns -1 if the port is not open (configured)
// returns -2 if the message is too big to be sent
// If the transmitter is idle it will initiate interrupts by 
// writing the first character to the hardware transmit buffer
	unsigned i,BufLen;
	if (!ComOpen)
		return (-1);
	BufLen = GetBufCount(&ComTXBuffer);
	if ( (MAXBUFFER - BufLen) < Count )
		return (-2); 
	for(i=0;i<Count;i++)
		PutBuf(&ComTXBuffer,Buffer[i]);
	
	if ( (USART2_CR1 & BIT3)==0)
	{ // transmitter was idle, turn it on and force out first character
	  USART2_CR1 |= BIT3;
	  USART2_CR1 |= BIT7; // enable txe interrupts
	  USART2_TDR = GetBuf(&ComTXBuffer);		
	} 
	return 0;
};


void initUART(int BaudRate) {
	int BaudRateDivisor;
	disable_interrupts();
	ComRXBuffer.Head = ComRXBuffer.Tail = ComRXBuffer.Count = 0;
	ComTXBuffer.Head = ComTXBuffer.Tail = ComTXBuffer.Count = 0;
	ComOpen = 1;
	ComError = 0;
// Turn on the clock for GPIOA (usart 2 uses it) - not sure if I need this
	RCC_AHBENR  |= BIT17;
// Turn on the clock for the USART2 peripheral	
	RCC_APB1ENR |= BIT17;
	

	
// Configure the I/O pins.  Will use PA2 as TX and PA15 as RX
	GPIOA_MODER |= (BIT5 | BIT31);
	GPIOA_MODER &= ~BIT4;
	GPIOA_MODER &= ~BIT30;
// The alternate function number for PA2 and PA15 is AF7 (see datasheet, reference manual)
	GPIOA_AFRL  &= ~(BIT11); 
	GPIOA_AFRL  |=  BIT10 | BIT9 | BIT8;
	GPIOA_AFRH  &= ~(BIT31); 
	GPIOA_AFRH  |= BIT30 | BIT29 | BIT28;	
	
	BaudRateDivisor = 32000000;   // assuming 32MHz Low speed PCLK 
	BaudRateDivisor = BaudRateDivisor / (long) BaudRate;

// De-assert reset of USART2 
	RCC_APB1RSTR &= ~BIT17;
	USART2_CR1 = 0; 
 
// Don't want anything from CR2Transmitter
	USART2_CR2 = 0;

// Don't want anything from CR3
	USART2_CR3 = 0;

// Set the baud rate
	USART2_BRR = BaudRateDivisor;

// Clear any pending interrupts
	USART2_ICR = 0xffffffff;

// Turn on Receiver, Transmit and Receive interrupts.
	USART2_CR1 |= (BIT2  |  BIT5 | BIT7 ); 


// Enable the USART
	USART2_CR1 |= BIT0;
// Enable EXTIMR
	EXTI_IMR1 |= BIT26; // enable USART2 IRQ
	EXTI_EMR1 |= BIT26; // enable USART2 Event
// Enable USART2 interrupts in NVIC	
	ISER1 |= (1 << (38-32));
// and enable interrupts 

	enable_interrupts();
}
void isr_usart2() 
{
// check which interrupt happened.
    if (USART2_ISR & BIT7) // is it a TXE interrupt?
		usart_tx();
	if (USART2_ISR & BIT5) // is it an RXNE interrupt?
		usart_rx();

}
void usart_rx (void)
{
// Handles serial comms reception
// simply puts the data into the buffer and sets the ComError flag
// if the buffer is fullup
	if (PutBuf(&ComRXBuffer,USART2_RDR) )
		ComError = 1; // if PutBuf returns a non-zero value then there is an error
}


void usart_tx (void)
{
// Handles serial comms transmission
// When the transmitter is idle, this is called and the next byte
// is sent (if there is one)
	if (GetBufCount(&ComTXBuffer))
		USART2_TDR=GetBuf(&ComTXBuffer);
	else
	{
	  // No more data, disable the transmitter 
	  USART2_CR1 &= ~BIT3;
	  USART2_CR1 &= ~BIT7; // disable txe interrupts
	  if (USART2_ISR & BIT6)
	  // Write TCCF to USART_ICR
	    USART2_ICR |= BIT6;
	  if (USART2_ISR & BIT7)
	  // Write TXFRQ to USART_RQR
	    USART2_RQR |= BIT4;
	}
}
int PutBuf(ComBuffer *Buf,unsigned char Data)
{
	if ( (Buf->Head==Buf->Tail) && (Buf->Count!=0))
		return(1);  /* OverFlow */
	disable_interrupts();
	Buf->Buffer[Buf->Head++] = Data;
	Buf->Count++;
	if (Buf->Head==MAXBUFFER)
		Buf->Head=0;
	enable_interrupts();
	return(0);
};
unsigned char GetBuf(ComBuffer *Buf)
{
    unsigned char Data;
    if ( Buf->Count==0 )
		return (0);
    disable_interrupts();
    Data = Buf->Buffer[Buf->Tail++];
    if (Buf->Tail == MAXBUFFER)
		Buf->Tail = 0;
    Buf->Count--;
    enable_interrupts();
    return (Data);
};
unsigned int GetBufCount(ComBuffer *Buf)
{
    return Buf->Count;
};
int eputs(char *s)
{
  // only writes to the comms port at the moment
  if (!ComOpen)
    return -1;
  while (*s) 
    WriteCom(1,s++);
  return 0;
}
int available()
{
	// return the number of bytes available for reading from 
	// the serial receive buffer
	GetBufCount(&ComRXBuffer);
}
int egets(char *s,int Max)
{
  // read from the comms port until end of string
  // or newline is encountered.  Buffer is terminated with null
  // returns number of characters read on success
  // returns 0 or -1 if error occurs
  // Warning: This is a blocking call.
  int Len;
  char c;
  if (!ComOpen)
    return -1;
  Len=0;
  c = 0;
  while ( (Len < Max-1) && (c != NEWLINE) )
  {   
    while (!GetBufCount(&ComRXBuffer)); // wait for a character
    c = GetBuf(&ComRXBuffer);
    s[Len++] = c;
  }
  if (Len>0)
  {
    s[Len]=0;
  }	
  return Len;
}
char HexDigit(int digitvalue) {
  if (digitvalue < 10)
    return(digitvalue + '0');
  else
    return(digitvalue + 'A' - 10);
}
void eputByte(unsigned char theByte) {
	char HexBuffer[3];
	HexBuffer[2] = 0;
	HexBuffer[1] = HexDigit(theByte & 0x000f);
	theByte = theByte >> 4;
	HexBuffer[0] = HexDigit(theByte & 0x000f);
	eputs(HexBuffer);
}
void eputShort(unsigned short theShort)
{
	char HexBuffer[5];
	HexBuffer[4] = 0;
	HexBuffer[3] = HexDigit(theShort & 0x000f);
	theShort = theShort >> 4;
	HexBuffer[2] = HexDigit(theShort & 0x000f);
	theShort = theShort >> 4;
	HexBuffer[1] = HexDigit(theShort & 0x000f);
	theShort = theShort >> 4;
	HexBuffer[0] = HexDigit(theShort & 0x000f);
	eputs(HexBuffer);
}
void eputLong(unsigned long theLong)
{
	char HexBuffer[9];
	HexBuffer[8] = 0;
	HexBuffer[7] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[6] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[5] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[4] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[3] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[2] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[1] = HexDigit(theLong & 0x000f);
	theLong = theLong >> 4;
	HexBuffer[0] = HexDigit(theLong & 0x000f);
	eputs(HexBuffer);
}
int usart_tx_busy()
{
    if ( (GetBufCount(&ComTXBuffer)) || (USART2_CR1 & BIT3) )
        return 1;
    else
        return 0;
}
