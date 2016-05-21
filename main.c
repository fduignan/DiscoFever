#include "serial.h"
#include "spi.h"
#include "stm32f303xx.h"
#include "fft.h"
#define LED_COUNT 16
#define BLOCKSIZE (2 * LED_COUNT)
#define Fs 32768
unsigned SysTickCounter=0;
float InputBuffer1[BLOCKSIZE];
float InputBuffer2[BLOCKSIZE];
float Cplx[BLOCKSIZE];
float *InputBuffer=InputBuffer1;
void configPins();
void initClock();
void delay(unsigned dly);
void writeDMABuffer(int DeviceNumber, unsigned long Value);
unsigned long getRainbow();
void latchWS2812B();
void initSysTick();
void initADC();
int readADC();
int DataReady;
uint8_t DMABuffer[LED_COUNT * 9]; // This will be used as a DMA Buffer for the SPI bus

int main()
{
	unsigned Index=0;
	unsigned Colour[3];
	unsigned swap;	
	initClock();
	initSysTick();
	initUART(9600);  // Set serial port to 9600,n,8,1
	configPins(); // Set up the pin to drive the onboard LDE
	initADC();
	initSPI(); // set up the SPI bus
    Colour[0]=0xff0000;
    Colour[1]=0x00ff00;
    Colour[2]=0x0000ff;
    
	while(1)
	{	
		
		// Output a long serial message to ensure interrupts are 
		// happening at the same time as DMA transfers
		eputs(" Interrupts are happening!\r\n");  
		writeDMABuffer(0,Colour[0]); // Output a colour Format: GGRRBB					
		writeDMABuffer(1,Colour[1]); // Output a colour Format: GGRRBB					
		writeDMABuffer(2,Colour[2]); // Output a colour Format: GGRRBB)
		// Now send out the bits to the SPI bus
		writeSPI(DMABuffer,sizeof(DMABuffer));		
		//eputLong(SPI1_SR);
		latchWS2812B(); // latch the values to the LED's		
	    Index++;
	    // The following causes each of the WS2812B's to ramp up
	    // one colour component to maximum and then reset again
	    if (Index > 5) 
	    {
			Index = 0;
			swap=Colour[2];
			Colour[2]=Colour[1];
			Colour[1]=Colour[0];
			Colour[0]=swap;
		}
		while(usart_tx_busy()); // wait for last Serial TX to finish
		sleep(); // save power when idle
		if (DataReady)  // new block of data is ready
		{
			if (InputBuffer == InputBuffer1) // Which one is ready?
			{
				fft(BLOCKSIZE,InputBuffer2,Cplx);
			}
			else
			{	
				fft(BLOCKSIZE,InputBuffer1,Cplx);
			}
			
			DataReady = 0; // Signal completion of block processing
		}
	} 
	return 0;
}

void configPins()
{
	// Power up PORTB
	RCC_AHBENR |= BIT18;
	GPIOB_MODER |= BIT6; // make bit3  an output
	GPIOB_MODER &= ~BIT7; // make bit3  an output
	
}
void initClock()
{
	// Assumption: Using HSI clock (8MHz)
	// This clock is provided by the STLink debug interface
	// PLL is driven by HSI/2 = 4MHz
	// Multiply x 16  = 64MHz
	
	// Must set flash latency (2 wait states) to allow
	// for higher speed clock	
	FLASH_ACR |= 2; // add two wait states
	
	RCC_CR &= ~BIT24; // turn off PLL
	delay(10);	
	RCC_CFGR |= ((0xf) << 18);	// PLL multiply = 16	
	RCC_CFGR |= BIT10;  // APB1 Low speed Clock = HCLK/2 (32MHz)	
	delay(10);
	RCC_CR |= BIT24; // turn on PLL
	delay(10);
	RCC_CFGR |= BIT1;  // switch to PLL as clock
}


void delay(unsigned dly)
{
  while( dly--);
}


void writeDMABuffer(int DeviceNumber, unsigned long Value)
{
    // have to expand each bit to 3 bits
    // Can then output 110 for WS2812B logic '1'
    // and 100 for WS2812B logic '0'
    uint32_t Encoding=0;
    uint8_t SPI_Data[9];
    int Index;
    
    // Process the GREEN byte
    Index=0;
    Encoding=0;
    while (Index < 8)
    {
        Encoding = Encoding << 3;
        if (Value & BIT23)
        {
            Encoding |= 0b110;
        }
        else
        {
            Encoding |= 0b100;
        }
        Value = Value << 1;
        Index++;
        
    }    
    SPI_Data[0] = ((Encoding >> 16) & 0xff);
    SPI_Data[1] = ((Encoding >> 8) & 0xff);
    SPI_Data[2] = (Encoding & 0xff);
    
    // Process the RED byte
    Index=0;
    Encoding=0;
    while (Index < 8)
    {
        Encoding = Encoding << 3;
        if (Value & BIT23)
        {
            Encoding |= 0b110;
        }
        else
        {
            Encoding |= 0b100;
        }
        Value = Value << 1;
        Index++;
        
    }    
    SPI_Data[3] = ((Encoding >> 16) & 0xff);
    SPI_Data[4] = ((Encoding >> 8) & 0xff);
    SPI_Data[5] = (Encoding & 0xff);
    
    // Process the BLUE byte
    Index=0;
    Encoding=0;
    while (Index < 8)
    {
        Encoding = Encoding << 3;
        if (Value & BIT23)
        {
            Encoding |= 0b110;
        }
        else
        {
            Encoding |= 0b100;
        }
        Value = Value << 1;
        Index++;
        
    }    
    SPI_Data[6] = ((Encoding >> 16) & 0xff);
    SPI_Data[7] = ((Encoding >> 8) & 0xff);
    SPI_Data[8] = (Encoding & 0xff);
    
    Index=0;
	while(Index < 9)
    {
		DMABuffer[Index+DeviceNumber*9]=SPI_Data[Index];
		Index++;		
	}              
}
unsigned long getRainbow()
{   // Cycle through the colours of the rainbow (non-uniform brightness however)
	// Inspired by : http://academe.co.uk/2012/04/arduino-cycling-through-colours-of-the-rainbow/
	static unsigned Red = 255;
	static unsigned Green = 0;
	static unsigned Blue = 0;
	static int State = 0;
	switch (State)
	{
		case 0:{
			Green++;
			if (Green == 255)
				State = 1;
			break;
		}
		case 1:{
			Red--;
			if (Red == 0)
				State = 2;
			break;
		}
		case 2:{
			Blue++;
			if (Blue == 255)
				State = 3;			
			break;
		}
		case 3:{
			Green--;
			if (Green == 0)
				State = 4;
			break;
		}
		case 4:{
			Red++;
			if (Red == 255)
				State = 5;
			break;
		}
		case 5:{
			Blue --;
			if (Blue == 0)
				State = 0;
			break;
		}		
	}
	return (Green << 16) + (Red << 8) + Blue;
}
void latchWS2812B()
{
	delay(1000); // This is about 80us at this clock speed; enough to
			    // be considered as a latch signal by the WS2812B's	
}			   
void initSysTick()
{
        
	STK_CSR |= ( BIT2 | BIT1 | BIT0); // enable systick, source = cpu clock, enable interrupt
// SysTick clock source = 64MHz.  Divide this down to create sample period
	STK_RVR = (64000000/Fs);   
	STK_CVR = 10; // don't want long wait for counter to count down from initial high unknown value
}
void SysTick()
{
// This should occur at a rate of 20000Hz.	
	readADC();
	InputBuffer[SysTickCounter]=((float)readADC()/2048.0f - 1.0f); // normalize the input signal
	SysTickCounter++;
	if (SysTickCounter == BLOCKSIZE)
	{ 	// at the end of a block: swap the buffers around
		if (InputBuffer==InputBuffer1)
		{
			InputBuffer=InputBuffer2;
		}	
		else
		{
			InputBuffer=InputBuffer1;
		}
		SysTickCounter  = 0;
		DataReady = 1;
	}
}
void initADC()
{
	
	// Power up PORTA
	RCC_AHBENR |= BIT17;
	GPIOA_MODER |= BIT1+BIT0; // put PA0 into analog mode
	// Will be using PA0 as an analog input which corresponds
	// to ADC1_IN1
	RCC_AHBENR |= BIT28; // turn on ADC clock
	// Set ADC clock = HCLK / 2=32MHz
	ADC12_CCR |= BIT17;
	// disable the ADC
	ADC1_CR &= ~BIT0;	
	// enable the ADC voltage regulator
	ADC1_CR &= ~BIT29;
	ADC1_CR |= BIT28;
	// start ADC calibration cycle
	ADC1_CR |= BIT31; 
	// wait for calibration to complete
	while (ADC1_CR & BIT31);
	
	// enable the ADC
	ADC1_CR |= BIT0;	
	
	// Select ADC Channel 1
	ADC1_SQR1 = (1 << 6);
}

int readADC()
{
	ADC1_CR |= BIT2; // start conversion
	while (ADC1_CR & BIT2); // wait for end of conversion
	return ADC1_DR;
}
