#ifndef _ACAPD_ACCEL_H
#define _ACAPD_ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

#define ACAPD_ACCEL_STATUS_UNLOADED	0U
#define ACAPD_ACCEL_STATUS_LOADING	1U
#define ACAPD_ACCEL_STATUS_INUSE	2U
#define ACAPD_ACCEL_STATUS_UNLOADING	3U

#define ACAPD_ACCEL_SUCCESS		0
#define ACAPD_ACCEL_FAILURE		(-1)
#define ACAPD_ACCEL_TIMEDOUT		(-2)
#define ACAPD_ACCEL_MISMATCHED		(-3)
#define ACAPD_ACCEL_RSCLOCKED		(-4)
#define ACAPD_ACCEL_NOTSUPPORTED	(-5)
#define ACAPD_ACCEL_INVALID		(-6)
#define ACAPD_ACCEL_LOAD_INUSE		(-7)

#define ACAPD_ACCEL_INPROGRESS		1

#include <acapd/sys/@PROJECT_SYSTEM@/accel.h>

/**
 * @brief accel package information structure
 */
typedef struct {
	const char *path; /**< path of package image */
	const void *data; /**< pointer to package image data */
} acapd_accel_pkg_t;

/**
 * @brief accel structure
 */
typedef struct {
	acapd_accel_pkg_t pkg; /**< pointer to the package */
	acapd_accel_sys_t sys_info; /**< system specific accel information */
	unsigned int type; /**< type of the accelarator */
	unsigned int status; /**< status of the accelarator */
	int load_failure; /**< load failure */
} acapd_accel_t;

/**
 *
 */
void init_accel(acapd_accel_t *accel);

void set_accel_pkg_img(acapd_accel_t *accel, const char *img);

void set_accel_pkg_data(acapd_accel_t *accel, const void *data);

int load_accel(acapd_accel_t *accel, unsigned int async);

int accel_load_status(acapd_accel_t *accel);

/*
 * TODO: Do we want stop accel for accel swapping?
 */
int remove_accel(acapd_accel_t *accel, unsigned int async);

#ifdef ACAPD_INTERNAL
int sys_load_accel(acapd_accel_t *accel, unsigned int async);

int sys_remove_accel(acapd_accel_t *accel, unsigned int async);
#endif /* ACAPD_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* _ACAPD_ACCEL_H */
