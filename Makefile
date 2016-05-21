# Specify the compiler to use
CC=arm-none-eabi-gcc
# Specify the assembler to use
AS=arm-none-eabi-as
# Specity the linker to use
LD=arm-none-eabi-ld

CCFLAGS=-mcpu=cortex-m4 -mthumb -g -mfloat-abi=hard -fsingle-precision-constant -mfpu=fpv4-sp-d16 -D ARM_MATH_CM4  -D __FPU_PRESENT=1

# Tell the linker where to find the libraries -> important: use thumb versions 
LIBSPEC=-L /usr/local/gcc-arm-none-eabi/lib/gcc/arm-none-eabi/5.2.1/armv7-m -L /usr/local/gcc-arm-none-eabi-5_2-2015q4/arm-none-eabi/lib/fpu/
# List the object files involved in this project
OBJS=	init.o \
	serial.o \
	spi.o \
	fft.o \
	main.o 

# The default 'target' (output) is main.elf and it depends on the object files being there.
# These object files are linked together to create main.elf
main.elf : $(OBJS)
	$(LD) $(OBJS) $(LIBSPEC) -lgcc -lm -T linker_script.ld --cref -Map main.map -nostartfiles -o main.elf
# The object file main.o depends on main.c.  main.c is compiled to make main.o
main.o: main.c
	$(CC) -c $(CCFLAGS) main.c -o main.o

init.o: init.c
	$(CC) -c $(CCFLAGS) init.c -o init.o
serial.o: serial.c
	$(CC) -c $(CCFLAGS) serial.c -o serial.o
spi.o: spi.c
	$(CC) -c $(CCFLAGS) spi.c -o spi.o
fft.o: fft.c
	$(CC) -c $(CCFLAGS) fft.c -o fft.o

# if someone types in 'make clean' then remove all object files and executables
# associated wit this project
clean: 
	rm $(OBJS) 
	rm main.elf 
