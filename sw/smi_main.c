//
//
// First test SMI access
//
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>

#include <stdint.h>

#include "smi.h"
#include "ide.h"

#define TRUE  1
#define FALSE 0

#define BCM2708_PERI_BASE        0x20000000
#define CLOCK_BASE               (BCM2708_PERI_BASE + 0x101000) /* Clocks */
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO   */

#ifndef SMI_BASE
  #define SMI_BASE               (BCM2708_PERI_BASE + 0x600000) /* SMI   */
#endif  // ndef SMI_BASE

#ifndef DMA_BASE
  #define DMA_BASE               (BCM2708_PERI_BASE + 0x07000) /* DMA   */
#endif // ndef DMA_BASE

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
char *clk_mem_orig, *clk_mem, *clk_map;
char *gpio_mem_orig, *gpio_mem, *gpio_map;
char *smi_mem_orig, *smi_mem, *smi_map;
char *dma_mem_orig, *dma_mem, *dma_map;

// I/O access
volatile unsigned int *gpio = 0;
volatile unsigned int *smi = 0;
volatile unsigned int *clk = 0;
volatile unsigned int *dma = 0;

#define SMICLK_CNTL  *(clk+44)  // = 0xB0
#define SMICLK_DIV   *(clk+45)  // = 0xB4

// DMA control block
// Must be placed 8-words aligned!
typedef struct {
	uint32_t info;
	uint32_t src;
	uint32_t dst;
	uint32_t length;
	uint32_t stride;
	uint32_t next;
	uint32_t pad[2]; // to fill alignment 
} dma_cb_type;

int16_t buffer[65536];
int16_t r_buffer[65536];

// DMA control block needs to be on 32 byte boundary
dma_cb_type dma_control_block __attribute__ ((aligned (32)));

//
//  GPIO
//

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET0   *(gpio+7)  // Set GPIO high bits 0-31
#define GPIO_SET1   *(gpio+8)  // Set GPIO high bits 32-53

#define GPIO_CLR0   *(gpio+10) // Set GPIO low bits 0-31
#define GPIO_CLR1   *(gpio+11) // Set GPIO low bits 32-53
#define GPIO_PULL   *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

///////////////////

// command is global as it saves a lot of code
#define CMDSIZE 256
char cmnd[CMDSIZE],*cmnd_pntr;

void setup_io();
void restore_io();

void help()
{
  printf("q : quit\n");
  printf("i            : Goto IDE access mode\n");
  printf("r adrs       : do read\n");
  printf("w adrs data  : do write\n");
  printf("W adrs data  : do block write data++\n");
  printf("R adrs       : do block read\n");
  printf("t : Set PIO timing mode\n");
  printf("l : Set data size\n");
  printf("<return>: repeat last\n");
  printf("All numbers are hex without leading 0x\n");
}

int is_hex(char c)
{
  if (c>='0' && c<='9')
    return TRUE;
  if (c>='A' && c<='F')
    return TRUE;
  if (c>='a' && c<='f')
    return TRUE;
  return FALSE;
} // is_hex

uint32_t to_hex(char c)
{
  if (c>='0' && c<='9')
    return c-'0';
  if (c>='A' && c<='F')
    return c-'A'+10;
  return c-'a'+10;
} // tohex


//
// Read hex number from command line
// (max uint32_t size)
//
// return 0 : error (not a hex number on command line)
// return 1 : OK at least 1 hex character
// 
// Flaws: No check on input overflow
int get_hex(uint32_t *hex)
{ int v;
  *hex=0;
  while (*cmnd_pntr==' ')
    cmnd_pntr++;
  if (!is_hex(*cmnd_pntr))
    return 0;
  while (is_hex(*cmnd_pntr))
  { *hex = (*hex << 4) + to_hex(*cmnd_pntr);
    cmnd_pntr++;
  }
  return 1;
} // get_hex
      

void dump(uint16_t *buffer,int length)
{ int row,col,index,b;
  char c;
  row = 0;
  index = 0;
  while (index<length)
  {
    if ((row & 0x0F)==0)
      printf("  0    1    2    3    4    5    6    7     8    9    A    B    C    D    E    F         ASCII\n");
    for (col=0; col<16; col++)
    { printf("%04X ",buffer[row*16+col]);
      if (col==7)
        putchar(' ');
    }
    for (col=0; col<16; col++)
    { b = buffer[row*16+col]>>8;
      c = (b>=0x20 && b<=0x7F) ? b : '.';
      putchar(c);
      b = buffer[row*16+col]& 0x00FF;
      c = (b>=0x20 && b<=0x7F) ? b : '.';
      putchar(c);
      if (col==7)
        putchar(' ');
    }
    putchar('\n');
    row++;
    index += 16;
  }
} // dump     

 
int main()
{ char last,timing;
  uint32_t address,data,length;
  int status,repeat;
  int last_cmnd;
  
  setup_io();
  
  /*  
  printf("DMA=0x%08X\n",(int)(&dma_control_block));
  INP_GPIO(6); 
  OUT_GPIO(6);
  for (data=0; data<10; data++)
  {
    GPIO_SET0 = 0x40;
    GPIO_CLR0 = 0x40;
  }
  */
  if (!setup_smi())
  { fprintf(stderr,"setup_smi failed\n");
    return 1;
  }
  
  for (data=0; data<65536; data++)
    buffer[data]=data;
  length = 512;
  
  last = 0;
  help();
  do {
    printf("\n>");
    fgets(cmnd,CMDSIZE,stdin);
    cmnd_pntr = cmnd;
    if (*cmnd_pntr==0x0A)
    { repeat = TRUE;
      cmnd_pntr++;
    }
    else
    { last_cmnd = *cmnd_pntr++;
      repeat = FALSE;
    }
    switch (last_cmnd)
    {
    case 'r' :
         if (!repeat)
         { if (!get_hex(&address))
           { printf("???\n");
             break;
           }
         }
         if (smi_direct_read(0, &data,address)==0)
           printf("Read failed\n");
         else
           printf("Read:0x%04X\n",data);
         break; 
     case 'w' : 
         if (!repeat)
         { if (!get_hex(&address) || !get_hex(&data))
           { printf("???\n");
             break;
           }
         }
         if (smi_direct_write(0, data,address)==0)
           printf("Write failed\n");
         break; 
     case 'W' : 
         if (!repeat)
         { if (!get_hex(&address))
           { printf("???\n");
             break;
           }
         }
         if (smi_block_write(0,length,buffer,address)==0)
           printf("Block write failed\n");
         break; 
     case 'R' : 
         if (!repeat)
         { if (!get_hex(&address))
           { printf("???\n");
             break;
           }
         }
         // set known pattern
         for (data=0; data<length; data+=2)
           r_buffer[data]=data;
         if (smi_block_read(0,length,r_buffer,address)==0)
           printf("Block read failed\n");
         else
           dump(r_buffer,length);
         break; 
     case 't' : // timing 
        do {
           printf("Pio timing 0..4? ");
           fgets(cmnd,CMDSIZE,stdin);
           timing = cmnd[0];
         } while (timing<'0' || timing>'4');
         timing -= '0';
         if (!set_pio_timing(timing))
           printf("!!! set_pio_timing failed !!!\n");
         break; 
     case 'l' : // data size  
        if (get_hex(&address))
          length = address;
        else
          printf("???\n");
        break; 
     case 'i' : // IDE access
        printf("Switching to IDE operating mode\n");
        ide();
        printf("Left IDE operating mode\n");
        break;  
     case 'q' :
        break;
     default:
         help();
     } // switch command 
        
  } while (last_cmnd!='q');
  
  restore_io();
} // main 

///////////////////////////////////////////////////////////////////


//
// Set up memory regions to access the peripherals.
//
void setup_io()
{  unsigned long extra;

   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
   {
      printf("Can't open /dev/mem\n");
      printf("Did you forgot to use 'sudo .. ?'\n");
      exit (-1);
   }

   /*
    * mmap clock
    */
   if ((clk_mem_orig = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
      fprintf(stderr,"setup_io() allocation error \n");
      exit (-1);
   }
   extra = (unsigned long)clk_mem_orig % PAGE_SIZE;
   if (extra)
     clk_mem = clk_mem_orig + PAGE_SIZE - extra;
   else
     clk_mem = clk_mem_orig;

   clk_map = (unsigned char *)mmap(
      (caddr_t)clk_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      CLOCK_BASE
   );

   if ((long)clk_map < 0) {
      printf("clk mmap error %d\n", (int)clk_map);
      exit (-1);
   }
   clk = (volatile unsigned int *)clk_map;


   /*
    * mmap GPIO
    */
   if ((gpio_mem_orig = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
      printf("allocation error \n");
      exit (-1);
   }
   extra = (unsigned long)gpio_mem_orig % PAGE_SIZE;
   if (extra)
     gpio_mem = gpio_mem_orig + PAGE_SIZE - extra;
   else
     gpio_mem = gpio_mem_orig;

   gpio_map = (unsigned char *)mmap(
      (caddr_t)gpio_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      GPIO_BASE
   );

   if ((long)gpio_map < 0) {
      printf("gpio mmap error %d\n", (int)gpio_map);
      exit (-1);
   }
   gpio = (volatile unsigned *)gpio_map;

   /*
    * mmap SMI
    */
   if ((smi_mem_orig = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
      printf("allocation error \n");
      exit (-1);
   }
   extra = (unsigned long)smi_mem_orig % PAGE_SIZE;
   if (extra)
     smi_mem = smi_mem_orig + PAGE_SIZE - extra;
   else
     smi_mem = smi_mem_orig;

   smi_map = (unsigned char *)mmap(
      (caddr_t)smi_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      SMI_BASE
   );

   if ((long)smi_map < 0) {
      printf("smi mmap error %d\n", (int)smi_map);
      exit (-1);
   }
   smi = (volatile unsigned *)smi_map;

   /*
    * mmap DMA
    */
   if ((dma_mem_orig = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
      printf("allocation error \n");
      exit (-1);
   }
   extra = (unsigned long)dma_mem_orig % PAGE_SIZE;
   if (extra)
     dma_mem = dma_mem_orig + PAGE_SIZE - extra;
   else
     dma_mem = dma_mem_orig;

   dma_map = (unsigned char *)mmap(
      (caddr_t)dma_mem,
      BLOCK_SIZE,
      PROT_READ|PROT_WRITE,
      MAP_SHARED|MAP_FIXED,
      mem_fd,
      SMI_BASE
   );

   if ((long)dma_map < 0) {
      printf("dma mmap error %d\n", (int)dma_map);
      exit (-1);
   }
   dma = (volatile unsigned *)dma_map;

} // setup_io

//
// Undo what we did above
//
void restore_io()
{
  munmap(smi_map,BLOCK_SIZE);
  munmap(gpio_map,BLOCK_SIZE);
  munmap(clk_map,BLOCK_SIZE);
  // free memory
  free(smi_mem_orig);
  free(gpio_mem_orig);
  free(clk_mem_orig);
} // restore_io

//
// Set timing channel 0 for PIO mode 0..4 
// Set timing channel 1 for PIO+DMA mode 0..4 
//
// Timing values in ns from standard
// PIO modes 0..4
static const int pio_timing[5][4] = {
  // Setup, Strobe, Hold, Period    
  {   70,    165,    5,    600  },
  {   50,    125,    5,    383  },
  {   30,    100,    5,    240  },
  {   30,     80,    5,    180  },
  {   25,     70,    5,    120  }
};
int set_pio_timing(int p)
{ unsigned int smi_clock_freq;
  int setup_cycles,strobe_cycles,hold_cycles,pace_cycles;
  double clock_period_ns,used;
  
  if (p<0 || p>4)
    return 0;
  
  // TODO: switch to higher clock freq for fast PIO modes 
  smi_clock_freq = 125000000;
  clock_period_ns = 1e9/smi_clock_freq;

  // Get nearest (rounded 'up') values but if zero use at least 1 
  setup_cycles  = (int)(pio_timing[p][0]/clock_period_ns+0.9);
  if (setup_cycles==0) setup_cycles = 1;
  if (setup_cycles==64) //**
    setup_cycles=0; 
  if (setup_cycles>64) 
    return 0; 

  strobe_cycles = (int)(pio_timing[p][1]/clock_period_ns+0.9);
  if (strobe_cycles==0) strobe_cycles = 1;
  if (strobe_cycles==128) //**
    strobe_cycles=0; 
  if (strobe_cycles>128) 
    return 0; 

  // I don't get the pace timing I expect
  // use hold to set period instead 
  used = (setup_cycles+strobe_cycles)*clock_period_ns;
  hold_cycles = (int)((pio_timing[p][3]-used)/clock_period_ns + 0.9);
  if (hold_cycles==0) hold_cycles = 1;
  if (hold_cycles==64) //**
    hold_cycles=0; 
  if (hold_cycles>64)
    return 0; 
    
  //** datasheet says values can be 1..64 or 1..128 but it does not
  // say how to get the 64, 128 value. I am assuming 0 
  
  // Not used (Actually no effect seen...)
  pace_cycles = 1;
  
  set_smi_timing(0, // channel 0 
                 1, // write 
                 setup_cycles,
                 strobe_cycles,
                 hold_cycles,
                 pace_cycles);
  set_smi_timing(0, // channel 0
                 0, // read 
                 setup_cycles,
                 strobe_cycles,
                 hold_cycles,
                 pace_cycles);

  set_smi_timing(1, // channel 0 
                 1, // write 
                 setup_cycles,
                 strobe_cycles,
                 hold_cycles,
                 pace_cycles);
  set_smi_timing(1, // channel 0
                 0, // read 
                 setup_cycles,
                 strobe_cycles,
                 hold_cycles,
                 pace_cycles);

                      
  return 1;
} // set_pio_timing
