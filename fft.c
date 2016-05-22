/* fft.ino 
A simple, not very optimal fft implementation 
Inspired by Dave Hale's FFTLab java program
*/
#include <math.h>
#include "fft.h"
// size of fft fixed at compile time - faster and safer than dynamically creating arrays at runtime.
static float cos_array[SIZE];
static float sine_array[SIZE];
static long Initialized = 0;

void init_trig() {
  int mmax,istep;
  int index = 0;
  int m;
  
  for (mmax=1,istep=2*mmax; mmax<SIZE; mmax=istep,istep=2*mmax) {
    float delta =(float)M_PI/(float)mmax;
    for (m=0; m<mmax; ++m) {
      float w = (float)m*delta;
      cos_array[index] = cos(w);
      sine_array[index] = sin(w);
      index++;
    }
    mmax = istep;
  }
  Initialized = 1;
}
// takes about 0.4ms for 128 point fft
// Implies maximum sampling rate of 320kHz - ok for audio.

void fft(int n, float * ar, float * ai) {
  fast_complexToComplex(-1,n,ar,ai);
}
// takes about 0.5 ms for 128 points

void ifft(int n, float * ar, float * ai) {
  fast_complexToComplex(1,n,ar,ai);
}
void fast_complexToComplex(int sign, int n, float * ar, float * ai) {
  int i,j;
  int angle_index = 0;
  int mmax,istep;         
  int m;
  if (!Initialized) {
    init_trig();
    Initialized = 1;  
  }
  for (i=j=0; i<n; ++i) {
    if (j>=i) {
      float tempr = ar[j];
      float tempi = ai[j];
      ar[j] = ar[i];
      ai[j] = ai[i];
      ar[i] = tempr;
      ai[i] = tempi;
    }
    m = n/2;
    while (m>=1 && j>=m) {
      j -= m;
      m /= 2;
    }
    j += m;
  }
  for (mmax=1,istep=2*mmax; mmax<n; mmax=istep,istep=2*mmax) {
    for (m=0; m<mmax; ++m) {
      float wr = cos_array[angle_index];
      float wi = sign * sine_array[angle_index];
      angle_index++;
      for (i=m; i<n; i+=istep) {
        j = i+mmax;
	float tr = wr*ar[j]-wi*ai[j];
	float ti = wr*ai[j]+wi*ar[j];
	ar[j] = ar[i]-tr;
	ai[j] = ai[i]-ti;
	ar[i] += tr;
	ai[i] += ti;
      }
    }
    mmax = istep;
  }
  if (sign == 1) {
    for ( i=0;i<n;i++) {
      ar[i] = ar[i] / (float)n;
      ai[i] = ai[i] / (float)n;
    }
  }
}
