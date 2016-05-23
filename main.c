#include "serial.h"
#include "spi.h"
#include "stm32f303xx.h"
#include "fft.h"
#define LED_COUNT 16
#define Fs 32768
volatile unsigned SysTickCounter=0;
short InputBuffer[SIZE];
float Real[SIZE];
float Cplx[SIZE];
void configPins();
void initClock();
void delay(unsigned dly);
void writeDMABuffer(int DeviceNumber, unsigned long Value);
unsigned long getRainbow();
void latchWS2812B();
void initSysTick();
void initADC();
int readADC();
void tic(void);
void toc(void);
unsigned long getColourGradient(float Value,float Maximum);
volatile int DataReady;
uint8_t DMABuffer[LED_COUNT * 9]; // This will be used as a DMA Buffer for the SPI bus

int main()
{
	unsigned Index=0;
	unsigned AverageIndex;
	unsigned Average;
	unsigned FrequencyComponent = 0;		
	initClock();
	initSysTick();
	initUART(9600);  // Set serial port to 9600,n,8,1
	configPins(); // Set up the pin to drive the onboard LDE
	initADC();
	initSPI(); // set up the SPI bus    
    
	while(1)
	{	
		
		// Output a long serial message to ensure interrupts are 
		// happening at the same time as DMA transfers
		eputs("SysTickCounter: ");  	    
		eputLong(SysTickCounter);		
		eputs("\r\n");  	    		
		while(usart_tx_busy()); // wait for last Serial TX to finish
		//sleep(); // save power when idle
		if (DataReady)  // new block of data is ready
		{
			tic();
			for (Index = 0; Index < SIZE; Index++)
			{
					Real[Index]=InputBuffer[Index];
					Cplx[Index]=0; 
			}					
			// Do FFT
			fft(SIZE,Real,Cplx);
			// Work out power spectrum
			// Only need to do half of it - no need to calculate
			// beyond Nyquist frequency.
			for (Index=0;Index < SIZE/2 ; Index++)
			{
				Real[Index]=Real[Index]*Real[Index]+Cplx[Index]*Cplx[Index];			
				Real[Index]=Real[Index] / (SIZE * (2048));
			}
			// Zero out the DC component
			Real[0]=0;			
			// Now average all of the frequency components into 16 bins i.e. 1 for each LED
			for (Index = 0; Index < LED_COUNT; Index++)
			{
				FrequencyComponent = 0;
				for (AverageIndex=0; AverageIndex < (SIZE/16) / LED_COUNT; AverageIndex ++)
				{
					FrequencyComponent += Real[Index*((SIZE/16)/LED_COUNT)+AverageIndex];
				}					
				writeDMABuffer(Index,getColourGradient(FrequencyComponent,5000));
				
			}						
			DataReady = 0; // Signal completion of block processing
			toc();
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
// The following lookup table approximates a colour gradient ranging from blue to red.  It is encoded GGRRBB format to suit the WS2812's
const unsigned long GRBColourGradient[]= {0,0xAF00E5,0xE301E6,0xE803B9,0xEA0488,0xEB0657,0xED0826,0xEF1E09,0xF1520B,0xF2860D,0xF4BB0E,0xF6EF10,0xCCF812,0x9BF914,0x6AFB16,0x39FD18,0x19FF2A};
#define GRADIENT_STEPS (sizeof(GRBColourGradient)/sizeof(unsigned long))
unsigned long getColourGradient(float Value,float Maximum)
{
	Value = GRADIENT_STEPS * Value / Maximum;
	if (Value < 0)
		Value = 0;
	if (Value > GRADIENT_STEPS-1)
		Value = GRADIENT_STEPS-1;
	return GRBColourGradient[(int)Value];
	
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
// This should occur at a rate of Fs Hz.		
	static int DMA_UpdateCount = 0;
	if (DMA_UpdateCount++ > 40)
	{
		writeSPI(DMABuffer,sizeof(DMABuffer));		
		DMA_UpdateCount = 0;

	}
	if (DataReady)
		return; // don't move on to next batch until last batch done
	InputBuffer[SysTickCounter]=readADC()-2048;
	SysTickCounter++;
	if (SysTickCounter == SIZE)	
	{ 	// If finished with block then set the DataReady flag
		SysTickCounter  = 0;		
		DataReady = 1;
		// Send out the bits to the SPI bus 
		
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
void tic()
{
	GPIOB_ODR |= BIT3;
}
void toc()
{
	GPIOB_ODR &= ~BIT3;
}
