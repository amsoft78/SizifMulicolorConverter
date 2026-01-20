#pragma output REGISTER_SP = 0xbfff
#pragma output CLIB_MALLOC_HEAP_SIZE = 16
#pragma output CLIB_STDIO_HEAP_SIZE = 0
#pragma define CLIB_EXIT_STACK_SIZE = 1
#pragma output CRT_ENABLE_CLOSE = 0

#include <arch/z80.h>
#include <arch/zx.h>
#include <features.h>
#include <input.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

__sfr __at 0xFF IOFF; 
__sfr __banked __at 0xBF3B IOUP;   // ULA+ command
__sfr __banked __at 0xFF3B IOUPD;  // ULA+ data

#include "car16.h"

void SetColor (unsigned char entry, unsigned char value)
{
    IOUP = entry;
    IOUPD = value;
}

int main(void)
{
    // enable ULA+
    IOUP = 0b01000000;
    IOUPD = 0b00000001;
    
    IOFF = 8; // CGA16 mode,
    
    zx_border(car16col0);
    car16_show();

    z80_delay_ms(2000);
    in_WaitForKey();
    return 0;
}
  