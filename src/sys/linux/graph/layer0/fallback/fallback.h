typedef struct dm dm_t;
typedef struct Buffer Buffer_t;
typedef struct Accel  Accel_t;

typedef int (*FALLBACKFUNCTION)(void*, int, void*, int, void*, int);

typedef struct fallback_DMConfig{
        uint8_t* InputChannelReq[10]; 
        int InputChannelSize[10]; 
        uint8_t InputChannelCount; 
        uint8_t* OutputChannelReq[10]; 
        int OutputChannelSize[10]; 
        uint8_t OutputChannelCount; 
        uint64_t size; 
        uint64_t status; 
	FALLBACKFUNCTION fallbackfunction;
}fallback_DMConfig_t;

extern int fallback_config(void* dmconfig_a, Accel_t *accel);
extern int fallback_MM2SStatus(void* dmconfig_a);
extern int fallback_S2MMStatus(void* dmconfig_a);
extern int fallback_MM2SData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size, uint8_t tid);
extern int fallback_S2MMData(void* dmconfig_a, Buffer_t* data, uint64_t offset, uint64_t size);
extern int fallback_S2MMDone(void* dmconfig_a, Buffer_t* data);
extern int fallback_MM2SDone(void* dmconfig_a, Buffer_t* data);
extern int fallback_MM2SAck(void* dmconfig_a);
extern int fallback_S2MMAck(void* dmconfig_a);

extern int fallback_register(dm_t *datamover, uint8_t InputChannelCount, 
			uint8_t OutputChannelCount, FALLBACKFUNCTION fallbackfunction);
extern int fallback_unregister(dm_t *datamover);

