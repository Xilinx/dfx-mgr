#include <unistd.h>
#include<stdlib.h>
#include "shm.h"

int acapd_alloc_shm(char *shm_allocator_name, acapd_shm_t *shm, size_t size, uint32_t attr){
	
	shm->va = mmap(0, size, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	shm->refcount = 0;
}
int acapd_free_shm(acapd_shm_t *shm){
}

int acapd_attach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm){
	chnl->dma_ops->mmap(shm->va, 0, shm->size, chnl);
	shm->refcount++;
	//metal_list_add_tail(shm->refs,);
}

int acapd_detach_shm(acapd_chnl_t *chnl, acapd_shm_t *shm){
	//metal_list_del(node);
	shm->refcount--;
}
int acapd_sync_shm_device(acapd_shm_t *shm, acapd_chnl_t *chnl){

}

