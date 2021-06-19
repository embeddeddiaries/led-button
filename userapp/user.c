#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>


char dev[] = "/dev/key-led-dev";


char rbuf[20];
char wbuf[] = "ABCDEFGHIJK";

int main(void)
{
	int fd,ret;
	struct pollfd pfd;

	fd = open(dev,O_RDWR|O_NONBLOCK);	

	if(fd < 0){
	    return -1;
	}
	
	pfd.fd = fd;
	pfd.events = (POLLIN | POLLRDNORM);

	while(1)
	{
	    ret = poll(&pfd,(unsigned long)1,-1);

	    read(fd,rbuf,sizeof(rbuf));
	    printf("From kernel = %s\n",rbuf);
	}
	return 0;
}


