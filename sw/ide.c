//
// IDE command interface
//

#include "ide.h"
#include "smi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>


#define TRUE  1
#define FALSE 0

// command is global as it saves a lot of code
#define CMDSIZE 256
static char ide_cmnd[CMDSIZE],*ide_cmnd_pntr;

#define MAX_BUFFER 65536

static int16_t ide_w_buffer[MAX_BUFFER];
static int16_t ide_r_buffer[MAX_BUFFER];


typedef struct {
  char *cmnd;
  char *help;
  int (*funct)();
} command_type;

void ide_dump(uint16_t *buffer,int length);


// Need prototype of all function pointers
int dummy();
int show_status();
int show_identify();
int rattle();
int read_lba();
int write_lba();
int fill_wb();
int dr();

command_type command[] = {
 { "ss" ,     "Show Status",                       show_status     },
 { "id" ,     "Show Disk indentify data",          show_identify  },
 { "rattle" , "Rapidly seek between 0 and 0xF000",           rattle     },
 { "r" ,      "r <number> [[sectors] >file]\n           read sector(s) [to file]",   read_lba   },
 { "w" ,      "w <number> Write buffer to LBA",              write_lba  },
 { "sb",      "sb <hex numb>  Fill Write buffer",            fill_wb    }, 
 { "dr",      "Single read from data reg (debug sw)",        dr         },
 { "quit",    "Exit program",                      dummy      },
 { "",        "End of this list",                  dummy      }
 };

void ide_help()
{ int c;
  c = 0 ;
  while (command[c].cmnd[0])
  { printf("%-8s : %s\n",command[c].cmnd,command[c].help);
    c++;
  }
} // ide_help

 
//
// Get word from command line
// return lenght of word 
// 
int get_word(char *word)
{ int w;
  w=0;
  while (*ide_cmnd_pntr && *ide_cmnd_pntr<=' ')
    ide_cmnd_pntr++;
  while (*ide_cmnd_pntr && *ide_cmnd_pntr>' ')
    word[w++]=*ide_cmnd_pntr++;
  word[w]=0;
  return w;
} // get_word


//
// Main IDE loop
//
void ide()
{ int l,cmnd,status,found,err;
  char word[CMDSIZE];
  err = 0;
  cmnd = -1;
  // set default write buffer
  for (l=0; l<256; l++)
    ide_w_buffer[l] = l;
  ide_help();
  do {
    printf("ide>");
    fgets(ide_cmnd,CMDSIZE,stdin);
    ide_cmnd_pntr = ide_cmnd;
    l = get_word(word);
    if (!l && cmnd>=0)
    { // repeat last command
      status = command[cmnd].funct();
    }
    else
    { // find and run command (if found) 
      cmnd = 0 ;
      found = FALSE;
      while (!found && command[cmnd].cmnd[0])
      { if (!strcmp(word,command[cmnd].cmnd))
        { status = command[cmnd].funct();
          found = TRUE;
        }
        else
          cmnd++;
      }
      if (!found)
      { printf("???\n");
        err++;
        if (err>=3)
        { ide_help();
          err=0;
        }
        cmnd = -1;
      }
    }
  } while (strncmp(ide_cmnd,"quit",4));
} // ide

// 
// wait till disk is no longer busy
// return 1 on OK
// return 0 on time-out 
int ide_wait_done()
{ int rep,status;
  rep = 100000;
  while (rep--)
  { // read status register
    smi_direct_read(0, &status,IDE_STATUS_REG_RO);
    // check busy bit 
    if ((status & 0x80) == 0)
      return TRUE;
  }
  printf("IDE busy time-out\n");
  return FALSE;
} // wait busy 


int dummy()
{ return 0;
}

//
// Return IDE status register 
//
uint16_t ide_status()
{ unsigned int status;
  smi_direct_read(0, &status, IDE_STATUS_REG_RO);
  return (uint16_t)status;
} // ide_status

//
// Show IDE status register
// 
int show_status()
{ uint16_t status;
  status = ide_status();
  printf("Status= 0x%04X:  ",status);
  if (status & IDE_STR_ERROR)
    printf("error!  ");
  if (status & IDE_STR_DATAREQST)
    printf("data request  ");
  if (status & IDE_STR_DATAREADY)
    printf("ready  ");
  if (status & IDE_STR_BUSY)
    printf("busy!  ");
  printf("\n");
  return 0;
} // show_status

//
// Get IDE device identify data
// 
int show_identify()
{ char text[64];
  int  l,t;
  uint32_t numb;
  smi_direct_write(0, IDE_CMND_IDENTIFY, IDE_COMMAND_REG_WO);
  if (!ide_wait_done())
    return;
  // Should return 256 words
  smi_block_read(0,256,ide_r_buffer,0);
  if (ide_status() & (IDE_STR_BUSY | IDE_STR_ERROR | IDE_STR_DATAREQST))
  { printf("Status not-idle-or-error\n");
    return;
  }
  // extract some basic information
  if (ide_r_buffer[0] & 0x8000)
    printf("ATA device\n");
  if (ide_r_buffer[0] & 0x0080)
    printf("Removable media\n");
  if (ide_r_buffer[0] & 0x0040)
    printf("Non removable media/device\n");
  printf("Cylinders:   %d\n",ide_r_buffer[1]);
  printf("Heads:       %d\n",ide_r_buffer[3]);
  printf("Sect/track:  %d\n",ide_r_buffer[6]);
  t=0;
  for (l=0; l<10; l++)
  { text[t++] = (ide_r_buffer[10+l]>>8) & 0x00FF;
    text[t++] =  ide_r_buffer[10+l]     & 0x00FF;
  }
  text[t]=0;
  printf("Serial numb: %s\n",text);
  t=0;
  for (l=0; l<4; l++)
  { text[t++] = (ide_r_buffer[23+l]>>8) & 0x00FF;
    text[t++] =  ide_r_buffer[23+l]     & 0x00FF;
  }
  text[t]=0;
  printf("Firmware:    %s\n",text);
  t=0;
  for (l=0; l<20; l++)
  { text[t++] = (ide_r_buffer[27+l]>>8) & 0x00FF;
    text[t++] =  ide_r_buffer[27+l]     & 0x00FF;
  }
  text[t]=0;
  printf("Model:       %s\n",text);
  printf("Max sect/irq:%d\n",ide_r_buffer[47]& 0x00FF);
  numb = ide_r_buffer[60];
  numb += ide_r_buffer[61]<<16;
  printf("Total LBA   :%d\n",numb);

  return 0;
} // show_identify

//
// Rapid seek between cylinder 0 and 0xF000
//
int rattle()
{ int repeat;
   for (repeat=0; repeat<100; repeat++)
   {
     smi_direct_write(0, 0x00,IDE_CYL_LOW_REG);
     smi_direct_write(0, 0xF0,IDE_CYL_HIGH_REG);
     smi_direct_write(0, IDE_CMND_SEEK,IDE_COMMAND_REG_WO);        
     if (!ide_wait_done())
       return FALSE;
     smi_direct_write(0, 0,IDE_CYL_LOW_REG);
     smi_direct_write(0, 0,IDE_CYL_HIGH_REG);
     smi_direct_write(0, IDE_CMND_SEEK,IDE_COMMAND_REG_WO);         
     if (!ide_wait_done())
       return FALSE;
   }
   return TRUE;
} // rattle
 
//
// Read sector(s) using LBA
// 
int read_lba()
{ int  l,t,sectors,block;
  uint32_t lba;
  int      status;
  int      value;
  char word[CMDSIZE];
  FILE     *fp;
  l = get_word(word);
  fp = 0;
  if (!l)
  { printf("syntax: r <lba number> [sectors] >[file]\n");
    return;
  }
  l = sscanf(word,"%d",&lba);
  if (l==0)
  { printf("syntax: r <lba number>  [sectors] >[file]\n");
    return;
  }
  // optional sector count 
  l = get_word(word);
  if (l)
  { 
    l = sscanf(word,"%d",&sectors);
    if (l==0)
    { printf("syntax: r <lba number>  [sectors] >[file]\n");
      return;
    }
    
    // optional file
    l = get_word(word);
    if (word[0]=='>')
    {
       fp = fopen(word+1,"wb");
       if (!fp)
       { printf("Could not open '%s' for writing\n",word+1);
         return;
       }
    }
  }
  else
    sectors = 1;
    
  while (sectors)
  { 
    // buffer is 64K half-words which is 256 512-byte sectors
    if (sectors>=256)
      block = 256;
    else
      block = sectors;
    // 00 = 256 sectors 
    smi_direct_write(0, block & 0x00FF, IDE_SECT_COUNT_REG);
    
    //
    // LBA split over 3.5 registers
    //
     
    value = lba & 0x00FF;
    smi_direct_write(0, value, IDE_SECT_NUMB_REG);
    
    value = (lba>>8) & 0x00FF;
    smi_direct_write(0, value, IDE_CYL_LOW_REG);
    
    value = (lba>>16) & 0x00FF;
    smi_direct_write(0, value, IDE_CYL_HIGH_REG);
    
    value = (lba>>24) & 0x000F; // Only 4 bits! 
    // For now always using device 0 
    value |= IDE_HDR_DEV0 | IDE_HDR_USE_LBA;
    smi_direct_write(0, value, IDE_HEAD_DEV_REG);
     
    smi_direct_write(0, IDE_CMND_READ_DMA, IDE_COMMAND_REG_WO);
    
     // Should return block*256 words
     // using SMI channel 1 which should be set-up for DMA mode
     // No chip select is allowed: address is 8 
    if (!smi_block_read(1,256*block,ide_r_buffer,0x8))
    { printf("smi_block_read error\n");
      if (fp) fclose(fp);
      return;
    }

    status = ide_status();
    if (status & (IDE_STR_BUSY | IDE_STR_ERROR | IDE_STR_DATAREQST))
    { printf("Status error:\n");
      if (status & IDE_STR_BUSY)      printf("busy\n");
      if (status & IDE_STR_ERROR)     printf("error\n");
      if (status & IDE_STR_DATAREQST) printf("data request\n");
      if (fp) fclose(fp);
      return;
    }
    if (fp)
    { l = fwrite(ide_r_buffer,2,256*block,fp);
      if (l!=256*block)
      {
        printf("Error writing to file\n");
        fclose(fp);
        return;
      }
    }
    else
      ide_dump(ide_r_buffer,256*block);
    lba+=block;
    sectors -= block;
  } // while sectors 
  if (fp)
    fclose(fp);
} // read_lba

//
// Write current ide write buffer to LBA 
// 
int write_lba()
{ int  l,t;
  uint32_t lba;
  int      value;
  char word[CMDSIZE];
  l = get_word(word);
  if (!l)
  { printf("syntax: w <lba number>\n");
    return;
  }
  l = sscanf(word,"%d",&lba);
  if (l==0)
  { printf("syntax: w <lba number>\n");
    return;
  }
  // Single sector
  smi_direct_write(0, 1, IDE_SECT_COUNT_REG);
  
  //
  // LBA split over 3.5 registers
  //
  
  value = lba & 0x00FF;
  smi_direct_write(0, value, IDE_SECT_NUMB_REG);
  
  value = (lba>>8) & 0x00FF;
  smi_direct_write(0, value, IDE_CYL_LOW_REG);
  
  value = (lba>>16) & 0x00FF;
  smi_direct_write(0, value, IDE_CYL_HIGH_REG);
  
  value = (lba>>24) & 0x000F; // Only 4 bits! 
  // For now always using device 0 
  value |= IDE_HDR_DEV0 | IDE_HDR_USE_LBA;
  smi_direct_write(0, value, IDE_HEAD_DEV_REG);
   
  smi_direct_write(0, IDE_CMND_WRITE_SECTORS, IDE_COMMAND_REG_WO);
  
  // Need to provide 256 words
  smi_block_write(0,256,ide_w_buffer,0);
  if (!ide_wait_done())
    return;
  if (ide_status() & (IDE_STR_BUSY | IDE_STR_ERROR | IDE_STR_DATAREQST))
  { printf("Status not-idle-or-error\n");
    return;
  }
} // write_lba

//
// Dump buffer 
//
void ide_dump(uint16_t *buffer,int length)
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
} // ide_dump     

//
// Fill write buffer
//
int fill_wb()
{ int  l,value;
  char word[CMDSIZE];
  l = get_word(word);
  if (!l)
  { printf("syntax: sb <hex number>\n");
    return;
  }
  l = sscanf(word,"%x",&value);
  if (l==0)
  { printf("syntax: sb <hex number>\n");
    return;
  }
  for (l=0; l<256; l++)
    ide_w_buffer[l] = value++;
  dump(ide_w_buffer,256);
} // fill_wb

int dr()
{ int d;
  smi_direct_read(0, &d, IDE_DATA_REG); 
  printf("Datareg: 0x%04X\n",d & 0x00FFFF);
  show_status();
}
