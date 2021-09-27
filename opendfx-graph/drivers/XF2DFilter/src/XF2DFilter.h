// graph_api.h
#ifndef XF2DFilter_H_
#define XF2DFilter_H_

#ifdef __cplusplus
#include <iostream>

extern "C" {
#endif

//typedef void *XF2DFilter_HANDLE;

typedef struct XF2DFilterConfig {
	//XF2DFilter_HANDLE matmulHandle;
	uint8_t *inputBuffer;
	uint8_t *outputBuffer;
	cv::Size *metadata;
	int metadataSize;
;
	int inputSize;
	int outputSize;
	int status;
	
} XF2DFilterConfig_t;

#ifdef __cplusplus
}
#endif


#endif // XF2DFilter_H_
