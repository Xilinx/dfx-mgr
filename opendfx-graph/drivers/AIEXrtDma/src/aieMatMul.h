// graph_api.h
#ifndef AIEMATMUL_H_
#define AIEMATMUL_H_

#ifdef __cplusplus
#include <iostream>
class AIEMatmul {
		public:
			explicit AIEMatmul(const std::string &xclbinFilename);
			
			std::string xclbinFilename;
			xrtDeviceHandle dhdl;
	};

extern "C" {
#endif

typedef void *AIEMATMUL_HANDLE;

typedef struct aieMatMulConfig {
	AIEMATMUL_HANDLE matmulHandle;
	
	int *arrayA;
	int *arrayB;
	int *arrayC;
	int status;
	
} aieMatMulConfig_t;

#ifdef __cplusplus
}
#endif


#endif // AIEMATMUL_H_
