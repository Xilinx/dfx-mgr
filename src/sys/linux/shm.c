#include <acapd/print.h>
#include <acapd/shm.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>

static void *acapd_vfio_alloc_shm(acapd_shm_allocator_t *allocator,
				acapd_shm_t *shm, size_t size, uint32_t attr)
{
	(void)allocator;
	(void)attr;
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return NULL;
	}
	if (shm->id < 0) {
		shm->id = 0;
	}
	shm->va = mmap(0, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, shm->id, 0);

	if (shm->va == NULL) {
		acapd_perror("%s: failed to allocate memory from fd %d.\n",
			     __func__, shm->id);
	} else {
		acapd_debug("%s: allocated memory %p from fd %d, size 0x%x.\n",
			    __func__, shm->va,
			    shm->id, size);
	}
	return shm->va;
}

static void acapd_vfio_free_shm(acapd_shm_allocator_t *allocator,
			       acapd_shm_t *shm)
{
	(void)allocator;
	if (shm == NULL) {
		acapd_perror("%s: error, shm is NULL.\n", __func__);
		return;
	}
	(void)munmap(shm->va, shm->size);
}

acapd_shm_allocator_t acapd_default_shm_allocator = {
	.name = "vfio_shm_allocator",
	.priv = NULL,
	.alloc = acapd_vfio_alloc_shm,
	.free = acapd_vfio_free_shm,
};
