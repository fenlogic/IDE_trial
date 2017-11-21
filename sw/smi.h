//
// BCM283x SMI interface 
// Derived from Documentation
// GVL 15-Oct-2014 
//

#include <stdint.h>

// Registers offset in CPU map 
//#define SMI_BASE            0x7e600000 VPU
// Assuming we access the register through a 4K MMU mapped address range
// As such the register addresses are the offsets in that range
//======
#define SMI_CS_REG           0          //** Control & Status register 
#define SMI_CS_RXFF_FULL        0x80000000  // RX FIFO full
#define SMI_CS_TXFF_EMPTY       0x40000000  // TX FIFO not empty
#define SMI_CS_RXFF_DATA        0x20000000  // RX FIFO holds data
#define SMI_CS_TXFF_SPACE       0x10000000  // TX FIFO can accept data
#define SMI_CS_RXFF_HIGH        0x08000000  // RX FIFO >= 3/4 full
#define SMI_CS_TXFF_LOW         0x04000000  // TX FIFO <  1/4 full
#define SMI_CS_AFERR            0x02000000  // AXI FIFO unsderflow/overflow
                                            // Write with bit set to clear
#define SMI_CS_EDREQ            0x00008000  // External DREQ received
#define SMI_CS_PXLDAT           0x00004000  // Pixel mode on
#define SMI_CS_SETERR           0x00002000  // Setup reg. written & used error
#define SMI_CS_PVMODE           0x00001000  // Pixel Valve mode on
#define SMI_CS_RXIRQ            0x00000800  // Receive interrupt
#define SMI_CS_TXIRQ            0x00000400  // Transmit interrupt
#define SMI_CS_DONIRQ           0x00000200  // Done interrupt
#define SMI_CS_TEEN             0x00000100  // Tear enable
#define SMI_CS_BYTPAD           0x000000C0  // Number of PAD byte
#define SMI_CS_WRITE            0x00000020  // 1= Write to ext. devices
                                            // 0= Read from ext. devices
#define SMI_CS_FFCLR            0x00000010  // Write to 1 to clear FIFO
#define SMI_CS_START            0x00000008  // Write to 1 to start transfer(s)
#define SMI_CS_BUSY             0x00000004  // Set if transfer is taking place
#define SMI_CS_DONE             0x00000002  // Set if transfer has finished
                                            // For reads only set after FF emptied
                                            // Write with bit set to clear
#define SMI_CS_ENABLE           0x00000001  // Set to enable SMI 
                                         
#define SMI_LENGTH_REG       1          //** transfer length register 
                                        
#define SMI_ADRS_REG         2          //** transfer address register
#define SMI_ADRS_DEV_MSK        0x00000300  // Which device timing to use 
#define SMI_ADRS_DEV_LS         8
#define SMI_ADRS_DEV0           0x00000000  // Use read0/write0 timing  
#define SMI_ADRS_DEV1           0x00000100  // Use read1/write1 timing 
#define SMI_ADRS_DEV2           0x00000200  // Use read2/write2 timing 
#define SMI_ADRS_DEV3           0x00000300  // Use read3/write3 timing 
#define SMI_ADRS_MSK            0x0000003F  // Address bits 0-5 to use 
#define SMI_ADRS_LS             0           // Address LS bit

#define SMI_DATA_REG         3          //** transfer data register
                                        
#define SMI_READ0_REG        4          //** Read  settings 0 reg  
#define SMI_WRITE0_REG       5          //** Write settings 0 reg  
#define SMI_READ1_REG        6          //** Read  settings 1 reg  
#define SMI_WRITE1_REG       7          //** Write settings 1 reg  
#define SMI_READ2_REG        8          //** Read  settings 2 reg  
#define SMI_WRITE2_REG       9          //** Write settings 2 reg  
#define SMI_READ3_REG       10          //** Read  settings 3 reg  
#define SMI_WRITE3_REG      11          //** Write settings 3 reg  

// SMI read and write register fields applicable
// to the 8 register listed above 
// Beware two fields are different between read and write 
#define SMI_RW_WIDTH_MSK        0xC0000000  // Data width mask 
#define SMI_RW_WID8             0x00000000  // Data width 8 bits
#define SMI_RW_WID16            0x40000000  // Data width 16 bits
#define SMI_RW_WID18            0x80000000  // Data width 18 bits
#define SMI_RW_WID9             0xC0000000  // Data width 9 bits
#define SMI_RW_SETUP_MSK        0x3F000000  // Setup cycles (6 bits)
#define SMI_RW_SETUP_LS         24          // Setup cycles LS bit (shift) 
#define SMI_RW_MODE68           0x00800000  // Run cycle motorola mode 
#define SMI_RW_MODE80           0x00000000  // Run cycle intel mode 
#define SMI_READ_FSETUP         0x00400000  //++ READ! Setup only for first cycle
#define SMI_WRITE_SWAP          0x00400000  //++ Write! swap pixel data
#define SMI_RW_HOLD_MSK         0x003F0000  // Hold cycles (6 bits)
#define SMI_RW_HOLD_LS          16          // Hold cycles LS bit (shift) 
#define SMI_RW_PACEALL          0x00008000  // Apply pacing always 
          // Clear: no pacing if switching to a different read/write settings
#define SMI_RW_PACE_MSK         0x00007F00  // Pace cycles (7 bits)
#define SMI_RW_PACE_LS          8           // Pace cycles LS bit (shift) 
#define SMI_RW_DREQ             0x00000080  // Use DMA req on read/write
                                            // Use with SMI_DMAC_DMAP
#define SMI_RW_STROBE_MSK       0x0000007F  // Strobe cycles (7 bits)
#define SMI_RW_STROBE_LS        0           // Strobe cycles LS bit (shift) 

// DMA control register 
#define SMI_DMAC_REG        12          //** DMA control register
#define SMI_DMAC_ENABLE         0x10000000  // DMA enable 
#define SMI_DMAC_DMAP           0x01000000  // D16/17 are resp DMA_REQ/DMA_ACK
#define SMI_DMAC_RXPANIC_MSK    0x00FC0000  // Read Panic threshold
#define SMI_DMAC_TXPANIC_MSK    0x0003F000  // Write Panic threshold
#define SMI_DMAC_RXTHRES_MSK    0x00000FC0  // Read DMA threshold
#define SMI_DMAC_TXTHRES_MSK    0x0000003F  // Write DMA threshold 

//
// Direct access registers
//
#define SMI_DIRCS_REG       13          //** Direct control register
// The following fields are identical in the SMI_DC register
#define SMI_DIRCS_WRITE         0x00000008  // 1= Write to ext. devices
#define SMI_DIRCS_DONE          0x00000004  // Set if transfer has finished
                                            // Write with bit set to clear
#define SMI_DIRCS_START         0x00000002  // Write to 1 to start transfer(s)
// Found to make no difference!!
#define SMI_DIRCS_ENABLE        0x00000001  // Set to enable SMI 
                                            // 0= Read from ext. devices 

#define SMI_DIRADRS_REG     14          //** transfer address register
#define SMI_DIRADRS_DEV_MSK     0x00000300  // Which device timing to use 
#define SMI_DIRADRS_DEV_LS      8
#define SMI_DIRADRS_DEV0        0x00000000  // Use read0/write0 timing  
#define SMI_DIRADRS_DEV1        0x00000100  // Use read1/write1 timing 
#define SMI_DIRADRS_DEV2        0x00000200  // Use read2/write2 timing 
#define SMI_DIRADRS_DEV3        0x00000300  // Use read3/write3 timing 
#define SMI_DIRADRS_MSK         0x0000003F  // Address bits 0-5 to use 
#define SMI_DIRADRS_LS          0           // Address LS bit

#define SMI_DIRDATA_REG     15          //** Direct access data register

// FIDO debug register
// There is terse and ambigous documentation
// e.g. How/when is high level reset
#define SMI_FIFODBG_REG     16          //** FIFO debug register
#define SMI_FIFODBG_HIGH_MSK    0x00003F00  // High level during last transfers
#define SMI_FIFODBG_HIGH_LS     8           // High level LS bit
#define SMI_FIFODBG_LEVL_MSK    0x0000003F  // Current level
#define SMI_FIFODBG_LEVL_LS     0           // Current level LS bit (shift)

//
//
//

//
//  GPIO
//

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))


// 
// Main access pointer to SMI peripheral
//
extern volatile unsigned int *smi;
extern volatile unsigned int *gpio;
extern volatile unsigned int *clk;


// Setup SMI
// return 0 on fail
// return 1 on OK 
int setup_smi();

//
// Setup one of four interface timings
// read or write
//
// return 1 on OK
// return 0 on failure 
// 
int set_smi_timing(unsigned int channel,  // Which of the four channels to set-up
                   unsigned int read,     // 1=read timing 0=write timing
                   unsigned int setup,    // Setup cycles 
                   unsigned int strobe,   // Strobe cycle 
                   unsigned int hold,     // Hold cycles
                   unsigned int pace      // Pace cycles
                   );

//
// Perform direct write
//
// return 0 on fail
// return 1 on OK 
// 
int smi_direct_write(unsigned int channel,  // Timing set to use 
                     unsigned int data,     // data to write
                     unsigned int address   // Address to write 
                    );

//
// Perform direct read
// 
// return 0 on fail
// return 1 on OK 
// 
int smi_direct_read(unsigned int channel,  // Timing set to use
                    unsigned int *data,    // data read return
                    unsigned int address   // Address to read 
                   );


//
// Perform block write
// 
int smi_block_write(unsigned int channel,  // Timing set to use
                     unsigned int words,   // Words to write
                     int16_t *data,        // data to write
                     unsigned int address  // Address to write 
                    );
                    
//
// Perform block read
// 
int smi_block_read (unsigned int timing,  // Timing set to use
                    unsigned int words,   // Words to write
                    int16_t  *data,       // place data read
                    unsigned int address  // Address to write 
                    );

