/*
 * sch-test
 * test program fro syscallh
 * Feb 23, 2019
 * root@davejingtian.org
 * https://davejingtian.org
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>


static const char *readme = "README.md";
static const char buf[64];

int main(void)
{
	int fd = open(readme, O_RDONLY);
	if ( fd < 0 )
		return -1;

	int bytes  = read(fd, buf, sizeof(buf)/2);
	printf("%s\n", buf);
	
	close(fd);
	return 0;
}
