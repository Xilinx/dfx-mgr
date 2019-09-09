#include <acapd/accel.h>
#include <acapd/assert.h>
#include <stdio.h>
#include <string.h>

void init_accel(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	memset(accel, 0, sizeof(*accel));
	accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
}

void set_accel_pkg_img(acapd_accel_t *accel, const char *img)
{
	acapd_assert(accel != NULL);
	accel->pkg.path = img;
}

void set_accel_pkg_data(acapd_accel_t *accel, const void *data)
{
	acapd_assert(accel != NULL);
	accel->pkg.data = data;
}

int load_accel(acapd_accel_t *accel, unsigned int async)
{
	int ret;

	acapd_assert(accel != NULL);
	/* TODO: Check if the accel is valid */
	/* For now, for now assume it is always PDI/DTB */
	ret = sys_load_accel(accel, async);
	if (ret == ACAPD_ACCEL_SUCCESS) {
		accel->status = ACAPD_ACCEL_STATUS_INUSE;
	} else if (ret == ACAPD_ACCEL_INPROGRESS) {
		accel->status = ACAPD_ACCEL_STATUS_LOADING;
	} else {
		accel->load_failure = ret;
	}
	return ret;
}

int accel_load_status(acapd_accel_t *accel)
{
	acapd_assert(accel != NULL);
	if (accel->load_failure != ACAPD_ACCEL_SUCCESS) {
		return accel->load_failure;
	} else if (accel->status != ACAPD_ACCEL_STATUS_INUSE) {
		return ACAPD_ACCEL_INVALID;
	} else {
		return ACAPD_ACCEL_SUCCESS;
	}
}

int remove_accel(acapd_accel_t *accel, unsigned int async)
{
	acapd_assert(accel != NULL);
	if (accel->status == ACAPD_ACCEL_STATUS_UNLOADED) {
		return ACAPD_ACCEL_SUCCESS;
	} else if (accel->status == ACAPD_ACCEL_STATUS_UNLOADING) {
		return ACAPD_ACCEL_INPROGRESS;
	} else {
		int ret;

		ret = sys_remove_accel(accel, async);
		if (ret == ACAPD_ACCEL_SUCCESS) {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADED;
		} else if (ret == ACAPD_ACCEL_INPROGRESS) {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADING;
		} else {
			accel->status = ACAPD_ACCEL_STATUS_UNLOADING;
		}
		return ret;
	}
}
