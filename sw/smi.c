//==============
//
// SMI Interface 
//
//==============
//
// Basic routines to get SMI running 
// Not optimised for speed!
//


#include "smi.h"

#include <stdio.h>


//
// Set up basic SMI mode 
// channel 0 is for 'PIO' mode
// channel 1 is for 'DMA' mode 
//
// return 1 on OK
// return 0 on failure 
//
int setup_smi()
{ int i;
  volatile unsigned int *reg;
  volatile unsigned int *smi_clk;
  
  // Check if IO has been set-up
  if (smi==0 || gpio==0 || clk==0)
    return 0;
    
  // Set GPIO 0..25 to SMI mode
  for (i=0; i<26; i++)
  { INP_GPIO(i);
    SET_GPIO_ALT(i,1);
  }
   
  // Set SMI clock 
  // Use PLL-D as that is not changing
  // PLL-D is set to 2GHz. 
  // but there seems to be a divide-by-four somewhere....
  
  smi_clk = clk+(0xB0>>2); 
  *smi_clk     = 0x5A000000; // Off 
  // PIO0 mode needs cycle time of 600ns
  // 'pace' does not work so use 'hold' instead
  // But hold goes max 64 and we need 90 cycle on 250MHz
  // Solution: set SMI clock to 125MHz 
  *(smi_clk+1) = 0x5A004000; // div 4 gives correct pulse width on scope 
  *smi_clk     = 0x5A000016; // PLLD & enabled 

  //  printf("SMI clock status: 0x%08X\n",*smi_clk);
  
  // Initially set all timing registers to 16 bit intel mode
  // assuming a 125MHz SMI clock 
  // All timing is set up for PIO0 mode 
  reg = smi + SMI_READ0_REG;
  for (i=0; i<8; i++)
    reg[i] = SMI_RW_WID16   | // 16 bits
             SMI_RW_MODE80  | //intel mode
             SMI_RW_PACEALL | // always pace 
             (9<<SMI_RW_SETUP_LS)   |  //  72 ns
             (21<<SMI_RW_STROBE_LS) |  // 168 ns
             (45<<SMI_RW_HOLD_LS)   |  // 360 ns
             (1<<SMI_RW_PACE_LS)       //   8 ns (not working???)
                        // total period : 608 ns
           ;

  // Set channel 1 (index 2,3) for DMA mode
  reg[2] |= SMI_CS_EDREQ;
  reg[3] |= SMI_CS_EDREQ;

  // Using SMI D16/D17 as DREQ/DACK pins
  reg   = smi + SMI_DMAC_REG;
  *reg |= SMI_DMAC_DMAP;
        
  return 1;
} // setup_smi



//
// Setup one of eight interface timings
// 
int set_smi_timing(unsigned int channel,   // Which of the four channels to set-up
                   unsigned int read,     // 1=read timing 0=write timing
                   unsigned int setup,    // Setup cycles 
                   unsigned int strobe,   // Strobe cycle 
                   unsigned int hold,     // Hold cycles
                   unsigned int pace      // Pace cycles
                   )
{ volatile unsigned int *reg;
  // Point to timing register
  reg = smi + SMI_READ0_REG + channel*2 + (read ? 0 : 1);
  // clear the timing values
  *reg &= ~(SMI_RW_SETUP_MSK|SMI_RW_HOLD_MSK|SMI_RW_PACE_MSK|SMI_RW_STROBE_MSK);
  // place new timing values 
  *reg |= (setup << SMI_RW_SETUP_LS)  | 
          (strobe<< SMI_RW_STROBE_LS) | 
          (hold  << SMI_RW_HOLD_LS)   | 
          (pace  << SMI_RW_PACE_LS)    
        ;
} // set_smi_timing


int smi_set_pio_timing(int p)
{
  
} // smi_set_pio_timing

//
// Perform direct write
// 
int smi_direct_write(unsigned int timing,  // Timing set to use
                     unsigned int data,     // data to write
                     unsigned int address   // Address to write 
                    )
{ int time_out,status;
    
  // clear done bit if set 
  status = smi[SMI_DIRCS_REG];
  if (status & SMI_DIRCS_DONE)
    smi[SMI_DIRCS_REG] = SMI_DIRCS_DONE;
  
  // Start write transfer 
  smi[SMI_DIRADRS_REG] = ((timing<<SMI_DIRADRS_DEV_LS)&SMI_DIRADRS_DEV_MSK) | 
                         (address&SMI_DIRADRS_MSK);
  smi[SMI_DIRDATA_REG] =  data;
  smi[SMI_DIRCS_REG]   =  SMI_DIRCS_WRITE|SMI_DIRCS_START|SMI_DIRCS_ENABLE;
  
  // busy wait till done
  time_out = 50;
  do {
    status = smi[SMI_DIRCS_REG];
  } while (--time_out && !(status & SMI_DIRCS_DONE));
  if (time_out==0)
    return 0;
    
  // clear done bit  
  smi[SMI_DIRCS_REG] = SMI_DIRCS_DONE;
  
  return 1;
} // smi_direct_write

// 
// Perform direct read
// 
int smi_direct_read(unsigned int timing,  // Timing set to use
                    unsigned int *data,    // read result 
                    unsigned int address   // Address to read 
                   )
{ int time_out,status;
  
  // clear done bit if set 
  status = smi[SMI_DIRCS_REG];
  if (status & SMI_DIRCS_DONE)
    smi[SMI_DIRCS_REG] = SMI_DIRCS_DONE;
  
  // Start read transfer 
  smi[SMI_DIRADRS_REG] = ((timing<<SMI_DIRADRS_DEV_LS)&SMI_DIRADRS_DEV_MSK) | 
                         (address&SMI_DIRADRS_MSK);
  smi[SMI_DIRCS_REG]   = SMI_DIRCS_START|SMI_DIRCS_ENABLE; 
  
  // Busy wait till done
  time_out = 50;
  // debug
  do {
    status = smi[SMI_DIRCS_REG];
  } while (--time_out && ((status & SMI_DIRCS_DONE)==0));
  if (time_out==0)
    return 0;     
  
  // get the data 
  *data = smi[SMI_DIRDATA_REG];

  // clear done bit  
  smi[SMI_DIRCS_REG] = SMI_DIRCS_DONE;
  
  // report success
  return 1;
} // smi_direct_read


//
// Perform block write
// 
int smi_block_write(unsigned int timing,  // Timing set to use
                    unsigned int words,   // Words to write
                    int16_t *data,   // data to write
                    unsigned int address  // Address to write 
                    )
{ int time_out,status;
  register volatile uint32_t *data_reg,*status_reg;
  
  status_reg = &(smi[SMI_CS_REG]);
  data_reg   = &(smi[SMI_DATA_REG]);
  
  // debug: clear FIFO 
  *status_reg = SMI_CS_FFCLR;
  
  // clear done bit, enable for writes  
  *status_reg = SMI_CS_WRITE|SMI_CS_DONE|SMI_CS_ENABLE; 
  // Set address & length 
  smi[SMI_ADRS_REG]   = ((timing<<SMI_ADRS_DEV_LS)&SMI_ADRS_DEV_MSK) | 
                         (address&SMI_ADRS_MSK);
  smi[SMI_LENGTH_REG] =  words;
   
  // pre-pump data into FIFO 
  // until full or run our of data 
  status = *status_reg;
  while (words && (status & SMI_CS_TXFF_SPACE))
  { 
    *data_reg = (int)*data++;
    words--;
    status = *status_reg;
  }  
  
  // Start transfer 
  *status_reg     =  SMI_CS_WRITE|SMI_CS_START|SMI_CS_ENABLE;
  
  // busy wait, pumping data into FIFO 
  time_out = 500;
  while (--time_out && words)
  { status = *status_reg;
    if (status & SMI_CS_TXFF_SPACE)
    { *data_reg = (int)*data++;
      words--;
      time_out = 500;
    }
  }
  if (time_out==0)
    return 0;     
  
  // Busy wait till done
  time_out = 500;
  do {
    status = *status_reg;
  } while (--time_out && (status & SMI_CS_BUSY));
  if (time_out==0)
    return 0;     
     
  // clear done bit, keep enabled 
  *status_reg = SMI_CS_DONE|SMI_CS_ENABLE;
  
  return 1;
} // smi_block_write


//
// Perform block read
// When calling this the SMI immediately starts reading
// Thus your device must have data available to read 
// Or the smi channel must be set up to use DMA mode (DREQ/DACK)
// This code need optimisation for the CPU to keep up at high speed.
// Or needs a DMA channel assigned
// 
int smi_block_read (unsigned int timing,  // Timing set to use
                    unsigned int words,   // Words to read
                    int16_t  *data,       // place data read
                    unsigned int address  // Address to write 
                    )
{ int time_out,status;
  register volatile uint32_t *data_reg,*status_reg;
  
  status_reg = &(smi[SMI_CS_REG]);
  data_reg   = &(smi[SMI_DATA_REG]);
   
  // clear done bit, enable for reads, clear FIFO  
  *status_reg = SMI_CS_DONE|SMI_CS_ENABLE|SMI_CS_FFCLR; 
  
  // Set address & length 
  smi[SMI_ADRS_REG]   = ((timing<<SMI_ADRS_DEV_LS)&SMI_ADRS_DEV_MSK) | 
                         (address&SMI_ADRS_MSK);
  smi[SMI_LENGTH_REG] =  words;
     
  // Start transfer 

  *status_reg     =  SMI_CS_START|SMI_CS_ENABLE;
 
 /* 
  // busy wait, read data from FIFO 
  time_out = words<<6;
  while (--time_out && words)
  { status = *status_reg;
    if (status & SMI_CS_RXFF_DATA)
    { *data++ = (uint16_t)*data_reg;
      words--;
    }
  }
  if (time_out==0)
    return 0;     
      
*/
  // speed version
  while (words)
  { 
    while (*status_reg & SMI_CS_RXFF_DATA)
    {
      *data++ = (uint16_t)*data_reg;
      words--;
    }
    printf("%d\n",words);
  }


  // Busy wait till done
  time_out = 500;
  do {
    status = *status_reg;
  } while (--time_out && (status & SMI_CS_BUSY));
  if (time_out==0)
    return 0;     
   
  // clear done bit, keep enabled 
  *status_reg = SMI_CS_DONE|SMI_CS_ENABLE;
  
  return 1;
} // smi_block_read

/*
// *****************************************
// Simple DMA Setup Routine
// *****************************************


void common_dma_setup  ( const uint32_t channel,                         // dma channel 0 to 15
                        const uint32_t source_addr,                     // 30 bit source address
                        const uint32_t dest_addr,                       // 30 bit destination address
                        const uint32_t src_stride,                      // 2d source stride
                        const uint32_t dst_stride,                      // 2d destination stride
                        const uint32_t transfer_length,                 // length in bytes
                        const uint32_t control_block_addr,              // address of 256 bit aligned block of memory 256 bits long to store control block in
                        const uint32_t next_control_block_addr,         // address of the next control block to load (0 if you want the dma to stop)
                        const uint32_t transfer_info                    // transfer info control reg
                      )
{

   // write the control block
   *((int*)control_block_addr + 0) = transfer_info;
   *((int*)control_block_addr + 1) = source_addr;
   *((int*)control_block_addr + 2) = dest_addr;
   *((int*)control_block_addr + 3) = transfer_length;
   *((int*)control_block_addr + 4) = ((dst_stride & 0xffff) << 16) | src_stride & 0xffff;
   *((int*)control_block_addr + 5) = next_control_block_addr;

   // channel 15 is in the VPU
   if (channel == 15) {
      DMA15_CS        = 0x0;
      DMA15_CS        = (1<<DMA15_CS_INT_LSB) | (1<<DMA15_CS_END_LSB);
      DMA15_CONBLK_AD = (int)control_block_addr;
   } else {
      DMA_CS(channel)        = 0x0 ;                                        // make sure dma is stopped
      DMA_CS(channel)        = (1<<DMA_0_CS_INT_LSB) | (1<<DMA_0_CS_END_LSB); // clear interrupts and end flag by writing a 1 to them
      DMA_CONBLK_AD(channel) = (int)control_block_addr;                     // give the dma the control block
   }


   // it wont start until we set the active bit

}


// *****************************************
// start the dma channel
// *****************************************
void common_dma_start ( const uint32_t channel ) {

   if (channel == 15) {
      DMA15_CS |= (1<<DMA15_CS_ACTIVE_LSB);
   } else {
      DMA_CS(channel) |= (1<<DMA_8_CS_ACTIVE_LSB);
   }

}

*/

/*
// Find out what revision board the Raspberry Pi is
// Using the file '/proc/cpuinfo' for that.
// returns 
//  0 : could not tell 
//  1 : rev 1
//  2 : rev 2
//  3 : B+
//
int pi_revision()
{
   FILE *fp;
   int  revision, match, number;
   char text[128]; // holds one line of file

   revision = 0;
   // Open the file with the CPU info
   fp = fopen("/proc/cpuinfo","r");
   if (fp)
   { // Find the line which starts 'Revision'
     while (fgets(text, 128, fp)) // get one line of text from file
     {
       if (!strncmp(text,"Revision",8)) // strncmp returns 0 if string matches
       { // Get the revision number from the text
      	 match = sscanf(text,"%*s : %0X",&number); // rev num is after the :
      	 if (match == 1)
      	   { // Yes, we have a revision number
      	     if (number >= 10)
      	       revision = 3;
      	     else
      	     if (number >= 4)
      	       revision = 2;
      	     else
      	       revision = 1;
      	   } // have number
                break; // no use in reading more lines, so break out of the while
       } // have revision text
     } // get line from file
     fclose(fp);
   } // Have open file

   return revision;
} // pi_revision
 */
 
 