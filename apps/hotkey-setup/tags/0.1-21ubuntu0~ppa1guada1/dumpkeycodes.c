#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

int main() {
	int sc;
	int fd;
	fd = open("/dev/console", O_RDWR);
	
	if (fd == -1) {
		return 1;
	}

	struct kbkeycode a;
	for (sc=0; sc<256; sc++) {
		a.scancode = sc;
		a.keycode = 0;
		if (ioctl(fd,KDGETKEYCODE,&a)) {
			if (errno == EINVAL) {
				a.keycode = 256;
			} else {
				perror("dumpkeycodes");
				return 2;
			}
		}
		if (sc < 128) {
			printf("%x %d\n",a.scancode,a.keycode);
		} else {
			printf("e0%02x %d\n",sc-128,a.keycode);
		}
	}
	return 0;
}
