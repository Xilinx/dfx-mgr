typedef struct fds{
	int s2mm_fd;
	int mm2s_fd;
	int config_fd;
	int accelconfig_fd;
	int dma_hls_fd;
	uint64_t mm2s_pa;
	uint64_t mm2s_size;
	uint64_t s2mm_pa;
	uint64_t s2mm_size;
	uint64_t config_pa;
	uint64_t config_size;

} fds_t;

//extern int initSocket();
//extern int finaliseSocket();
extern int loadpdi(char* pdifilename);
extern int removepdi(char* argvalue);
extern int getFD(char* argvalue);
extern int getPA(char* argvalue);
extern int getShellFD();
extern int getClockFD();
//extern int testInit();
//extern int testFinalise();
extern int socketGetFd(int slot, fds_t *fds);
extern int socketGetPA(int slot, fds_t *fds);
extern int test();

