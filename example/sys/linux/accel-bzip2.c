#include <acapd/accel.h>
#include <unistd.h>

int main(void)
{
	acapd_accel_t bzip2_accel;

	init_accel(&bzip2_accel);

	set_accel_pkg_img(&bzip2_accel, "system-bzip2.dtbo");

	load_accel(&bzip2_accel, 0);

	sleep(2);
	remove_accel(&bzip2_accel, 0);
	return 0;
}

