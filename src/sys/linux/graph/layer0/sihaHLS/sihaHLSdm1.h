#define TASKEND     0x00FF
#define STATUS      0x00F0
#define LOOPFILE    0x00F1
#define FFTCONFIG   0x00F2
#define GETFFT      0x00F3
#define BUFFER      0x00F4
#define BUFFERLOOP  0x00F5
#define DONE        0x00F6
#define CMATRANSACTION 0x00F7

struct DMConfig;
typedef struct DMConfig DMConfig_t;

typedef struct Buffer Buffer_t;

extern int configDMSlots(DMConfig_t* dmconfig, volatile uint8_t* s2mm_base);
extern int S2MMStatus(DMConfig_t* dmconfig);
extern int MM2SStatus(DMConfig_t* dmconfig);
extern int MM2SData(DMConfig_t* dmconfig, uint64_t data, uint64_t size, uint8_t tid);
extern int S2MMData(DMConfig_t* dmconfig, uint64_t data, uint64_t size);
extern int S2MMDone(DMConfig_t* dmconfig);
extern int MM2SDone(DMConfig_t* dmconfig);
extern int S2MMAck(DMConfig_t *dmconfig);
extern int MM2SAck(DMConfig_t* dmconfig);

/*extern void *MM2S_Task(void* carg);
extern void *S2MM_Task(void* carg);
extern int TaskInit(DMConfig_t* dmconfig);
extern int TaskFinalise(DMConfig_t* dmconfig);
extern int TaskEnd(DMConfig_t *dmconfig);
extern int MM2S_Buffer(DMConfig_t *dmconfig, Buffer_t* inbuff, int insize);
extern int S2MM_Buffer(DMConfig_t *dmconfig, Buffer_t* outbuff, int outsize);
extern int S2MM_Completion(DMConfig_t *dmconfig);
extern int allocateDMConfig(DMConfig_t **dmconfig);*/
