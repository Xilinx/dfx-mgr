
typedef struct sihaDmaConfig {
	int slot;
	int AccelConfig_fd;
	int dma_hls_fd;
	uint8_t* AccelConfig;
	uint8_t* dma_hls;
	
	volatile uint8_t* s2mm_baseAddr; 
	volatile uint8_t* s2mm_APCR; 
	volatile uint8_t* s2mm_GIER; 
	volatile uint8_t* s2mm_IIER; 
	volatile uint8_t* s2mm_IISR; 
	volatile uint8_t* s2mm_ADDR_LOW; 
	volatile uint8_t* s2mm_ADDR_HIGH; 
	volatile uint8_t* s2mm_SIZE_LOW; 
	volatile uint8_t* s2mm_SIZE_HIGH; 
	volatile uint8_t* s2mm_TID; 
	volatile uint8_t* mm2s_baseAddr; 
	volatile uint8_t* mm2s_APCR; 
	volatile uint8_t* mm2s_GIER; 
	volatile uint8_t* mm2s_IIER; 
	volatile uint8_t* mm2s_IISR; 
	volatile uint8_t* mm2s_ADDR_LOW; 
	volatile uint8_t* mm2s_ADDR_HIGH; 
	volatile uint8_t* mm2s_SIZE_LOW; 
	volatile uint8_t* mm2s_SIZE_HIGH; 
	volatile uint8_t* mm2s_TID;
	int start;
} sihaDmaConfig_t;

// HLS control
#define APCR       0x00
#define GIER       0x04
#define IIER       0x08
#define IISR       0x0c
#define ADDR_LOW   0x10
#define ADDR_HIGH  0x14
#define SIZE_LOW   0x1c
#define SIZE_HIGH  0x20
#define TID        0x24
//Control signals
#define AP_START       0
#define AP_DONE        1
#define AP_IDLE        2
#define AP_READY       3
#define AP_AUTORESTART 7

#define S2MM      0x00000
#define MM2S      0x10000
