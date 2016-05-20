// size of fft fixed at compile time - faster and safer than dynamically creating arrays at runtime.
#define LED_COUNT 16
#define BLOCKSIZE (2 * LED_COUNT)
#define SIZE (2 * LED_COUNT)	

void init_trig();
void fft(int n, float * ar, float * ai);
void ifft(int n, float * ar, float * ai);
void fast_complexToComplex(int sign, int n, float * ar, float * ai);
