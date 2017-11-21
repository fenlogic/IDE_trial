//
// IDE
// register mode
//
//
// Values taken from ATAPI standard
// GVL 17-Oct-2014 
//

#define IDE_DATA_REG       0x0
#define IDE_ERROR_REG_RO   0x1
#define IDE_FEATURE_REG_WO 0x1
#define IDE_ERR_ABORT           0x04 // command error or aborted
#define IDE_SECT_COUNT_REG 0x2
#define IDE_SECT_NUMB_REG  0x3
#define IDE_CYL_LOW_REG    0x4
#define IDE_CYL_HIGH_REG   0x5
#define IDE_HEAD_DEV_REG   0x6
#define IDE_HDR_DEV0            0x00 // device 0 
#define IDE_HDR_DEV1            0x10 // device 1  
#define IDE_HDR_USE_LBA         0x40 //          
#define IDE_COMMAND_REG_WO 0x7
#define IDE_STATUS_REG_RO  0x7
#define IDE_STR_ERROR           0x01
#define IDE_STR_DATAREQST       0x08
#define IDE_STR_DATAREADY       0x40
#define IDE_STR_BUSY            0x80
//#define ALT_STATUS_REG_RO  0x7
//#define ALT_STATUS_REG_RO  0x7
//#define DEVCNTL_REG    0x6*
//#define DCR_DEV_RESET  0x04
//#define DCR_IRQ_ENABL  0x02

//
// Commands
//
#define IDE_CMND_ERASE_SECTOR   0xC0

#define IDE_REQ_EXT_ERROR_CODE  0x03
#define IDE_EXTERR_NONE             0x00
#define IDE_EXTERR_SELFTEST_PASSED  0x01
#define IDE_EXTERR_WRITE_ERASE_FAIL 0x03
#define IDE_EXTERR_SELFTEST_FAIL    0x05
#define IDE_EXTERR_MISC_ERROR       0x09
#define IDE_EXTERR_VENDOR_CODEB     0x0B
#define IDE_EXTERR_CORRUPT_MEDIA    0x0C
#define IDE_EXTERR_VENDOR_CODED     0x0D
#define IDE_EXTERR_VENDOR_CODEE     0x0E
#define IDE_EXTERR_VENDOR_CODEF     0x0F
#define IDE_EXTERR_ID_NOTFND_ERROR  0x10
#define IDE_EXTERR_ECC_ERROR        0x11
#define IDE_EXTERR_ID_NOT_FOUND     0x14
#define IDE_EXTERR_ECC_CORRECTED    0x18
#define IDE_EXTERR_VENDOR_CODE1D    0x1D
#define IDE_EXTERR_VENDOR_CODE1E    0x1E
#define IDE_EXTERR_DATA_TRANSFERR   0x1F // or command aborted 
#define IDE_EXTERR_INVALID_COMMAND  0x20 
#define IDE_EXTERR_INVALID_ADDRESS  0x21
#define IDE_EXTERR_VENDOR_CODE22    0x22
#define IDE_EXTERR_VENDOR_CODE23    0x23
#define IDE_EXTERR_WRITE_PROTECTED  0x27
#define IDE_EXTERR_ADRES_TOO_LARGE  0x2F
#define IDE_EXTERR_SELFTEST_FAIL30  0x30
#define IDE_EXTERR_SELFTEST_FAIL31  0x31
#define IDE_EXTERR_SELFTEST_FAIL32  0x32
#define IDE_EXTERR_SELFTEST_FAIL33  0x33
#define IDE_EXTERR_SELFTEST_FAIL34  0x34
#define IDE_EXTERR_VOLTAGE_ERROR35  0x35
#define IDE_EXTERR_VOLTAGE_ERROR36  0x36
#define IDE_EXTERR_SELFTEST_FAIL37  0x37
#define IDE_EXTERR_SELFTEST_FAIL3E  0x3E
#define IDE_EXTERR_MEDIA_CORRUPT38  0x38
#define IDE_EXTERR_VENDOR_CODE39    0x39
#define IDE_EXTERR_NO_SPARE_SECT_   0x3A // spare sectors exhausted
#define IDE_EXTERR_MEDIA_CORRUPT3B  0x3B
#define IDE_EXTERR_MEDIA_CORRUPT3C  0x3C
#define IDE_EXTERR_MEDIA_CORRUPT3F  0x3F
#define IDE_EXTERR_VENDOR_CODE3D    0x3D
// all other error codes reserved 

#define IDE_CMND_CFA_TRANSLATE  0x87
// returns in data bytes:
// CYL_MSB, CYL_LSB, HEAD, SECTOR, 
// LBA bits 23:16, 15:8, 7:0
// 12xbyte reserved
// 0x13:Sector erased flas
// 0x14-0x17: reserved
// sector count 23:16, 15:8, 7:0

#define IDE_CMND_WRITE_MULTIPLE_NOERASE 0xCD
#define IDE_CMND_WRITE_SECTORS_NOERASE  0x38
#define IDE_CMND_CHECK_POWER            0xE5
#define IDE_CMND_DEVICE_RESET           0x08
// 0x92
// 0x90
// 0xE7
// 0xDA
#define IDE_CMND_IDENTIFY   0xEC
#define IDE_CMND_IDENTIFY_PACKET_DEV 0xA1
#define IDE_CMND_IDLE_TIMED       0xE3
#define IDE_CMND_IDLE_NOW         0xE1
// 0x91
#define IDE_CMND_EJECT            0xED
#define IDE_CMND_LOCK_MEDIA       0xDE
#define IDE_CMND_UNLOCK_MEDIA     0xDF
#define IDE_CMND_NOP              0x00
#define IDE_CMND_PACKET           0xA0
#define IDE_CMND_READ_BUFFER      0xE4
#define IDE_CMND_READ_DMA         0xC8
#define IDE_CMND_READ_DMA_QUEUED  0xC7
#define IDE_CMND_READ_MULTIPLE    0xC4
#define IDE_CMND_READ_MAX_ADDRESS 0xF8
#define IDE_CMND_READ_SECTORS     0x20
#define IDE_CMND_READ_VERIFY      0x40
// 0xF6
// 0xF3
// 0xF4
// 0xF5
// 0xF1
// 0xF2
#define IDE_CMND_SEEK    0x70
// 0xA2
// 0xEF
#define IDE_CMND_SET_MULTIPLE 0xC6
#define IDE_CMND_SLEEP        0xE6
// 0xE2
// 0xE0
#define IDE_CMND_WRITE_BUFFER      0xE8 // Page 213 (PDF 227)
#define IDE_CMND_WRITE_DMA         0xCA 
#define IDE_CMND_WRITE_DMA_QUEUED  0xCC
#define IDE_CMND_WRITE_MULTIPLE    0xC5
#define IDE_CMND_WRITE_SECTORS     0x30











