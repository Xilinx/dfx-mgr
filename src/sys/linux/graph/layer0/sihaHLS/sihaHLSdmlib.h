typedef struct sihahls_DMConfig;


/*typedef struct QueueBuffer{
        int type;
        char filename[1024];
        Buffer_t* buffer;
        int transferSize;
        double throughput;
}QueueBuffer_t;*/

#define TASKEND     0x00FF
#define STATUS      0x00F0
#define LOOPFILE    0x00F1
#define FFTCONFIG   0x00F2
#define GETFFT      0x00F3
#define BUFFER      0x00F4
#define BUFFERLOOP  0x00F5
#define DONE        0x00F6
#define CMATRANSACTION 0x00F7

extern int sihahls_register(dm_t *datamover);
extern int sihahls_unregister(dm_t *datamover);

/*extern void *MM2S_Task(void* carg);
extern void *S2MM_Task(void* carg);
extern int TaskInit(DMConfig_t* dmconfig);
extern int TaskFinalise(DMConfig_t* dmconfig);
extern int TaskEnd(DMConfig_t *dmconfig);
extern int MM2S_Buffer(DMConfig_t *dmconfig, Buffer_t* inbuff, int insize);
extern int S2MM_Buffer(DMConfig_t *dmconfig, Buffer_t* outbuff, int outsize);
extern int S2MM_Completion(DMConfig_t *dmconfig);
extern int allocateDMConfig(DMConfig_t **dmconfig);*/
