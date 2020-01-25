#include <acapd/print.h>
#include <acapd/shm.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int acapd_vfio_alloc_shm(acapd_shm_allocator_t *allocator,
				acapd_shm_t *shm, size_t size, uint32_t attr)
{
	(void)allocator;
	(void)attr;
	if (shm == NULL) {
		acapd_perror("%s: shm is NULL.\n", __func__);
		return -EINVAL;
	}
	if (shm->id < 0) {
		shm->id = 0;
	}

	if (shm->flags == 0) {
		shm->flags = MAP_PRIVATE | MAP_ANONYMOUS;
	}
	shm->va = mmap(NULL, size, PROT_READ | PROT_WRITE,
		       shm->flags, shm->id, 0);

	if (shm->va == MAP_FAILED) {
		acapd_perror("%s: failed to allocate memory from fd %d, %s.\n",
			     __func__, shm->id, strerror(errno));
	} else {
		acapd_debug("%s: allocated memory %p from fd %d, size 0x%x.\n",
			    __func__, shm->va,
			    shm->id, size);
	}
	shm->size = size;
	return 0;
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
