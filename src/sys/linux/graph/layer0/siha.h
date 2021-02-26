//#include "xrtbuffer.h"
//#include "uio.h"

typedef struct Buffers Buffers_t;
typedef struct plDevices plDevices_t;

typedef struct InBuffer{
	uint8_t* buff;
} InBuffer_t;

typedef struct OutBuffer{
	uint8_t* buff;
} OutBuffer_t;

typedef struct Accelf{
	char name[1000];
	int allocatedSlot;
	void* fallbackFunction;
	uint8_t* confBuffer; 
	InBuffer_t inBuffer[8];
	OutBuffer_t outBuffer[7];
	int inBufferCount;
	int outBufferCount;
} Accelf_t;

typedef struct QueueBufferf{
	int type;
	char filename[1024];
	uint8_t* buffer;
	int transferSize;
	double throughput;
}QueueBufferf_t;

#define TASKEND     0x00FF
#define STATUS      0x00F0
#define LOOPFILE    0x00F1
#define FFTCONFIG   0x00F2
#define GETFFT      0x00F3
#define BUFFER      0x00F4
#define BUFFERLOOP  0x00F5
#define DONE        0x00F6


extern int SIHAInitAccel(int slot, char * accel);
extern int SIHAFinaliseAccel(int slot);
extern int SIHAFreeAccel(int slot);
extern int SIHAInit(int configSize, int MM2SSize, int S2MMSize, int slot);
extern int SIHAFinalise(int slot);
extern int SIHAStartAccel(int slot);
extern int SIHAStopAccel(int slot);

extern int SIHAConfig(int slot, uint32_t* config, int size, int tid);
extern int SIHALoopFile(char* filename);
extern int SIHAExchangeFile(char* infile, char* outfile);
extern int SIHAExchangeBuffer(int slot, uint32_t* inbuff, uint32_t* outbuff, int insize, int outsize);
extern void *SIHASendTask(void* carg);
extern int SIHATaskInit();
extern int SIHATaskFinalise();
extern int SIHATaskEnd();
extern int SIHA_MM2S_Buffer(uint32_t* inbuff, int insize);
extern int SIHA_S2MM_Buffer(uint32_t* outbuff, int outsize);
extern int SIHA_S2MM_Completion();
extern long SIHAGetInThroughput();
extern long SIHAGetOutThroughput();
extern Buffers_t* SIHAGetBuffers();
extern plDevices_t* SIHAGetPLDevices();
