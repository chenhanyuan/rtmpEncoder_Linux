
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>

#include "rtspservice.h"
#include "rtputils.h"
#include "ringfifo.h"
#include "sample_comm.h"
#include "rtmpStream.h"

extern int g_s32Quit ;

/**************************************************************************************************
**
**
**
**************************************************************************************************/
extern void *SAMPLE_VENC_1080P_CLASSIC(void *p);
RTMP *rtmp = NULL;	
RTMPMetadata MetaData;

int main(void)
{
	int s32MainFd,temp;
	struct timespec ts = { 2, 0 };
	pthread_t id;
	ringmalloc(256*1024);
	printf("RTSP server START\n");
	PrefsInit();
	printf("listen for client connecting...\n");
	signal(SIGINT, IntHandl);
	s32MainFd = tcp_listen(SERVER_RTSP_PORT_DEFAULT);
	if (ScheduleInit() == ERR_FATAL)
	{
		fprintf(stderr,"Fatal: Can't start scheduler %s, %i \nServer is aborting.\n", __FILE__, __LINE__);
		return 0;
	}
	RTP_port_pool_init(RTP_DEFAULT_PORT);


 
	rtmp = AllocRtmp();	
	if(rtmp==NULL)	
	{		
		printf("rtmp alloc failed\n");
		return -1;
	}	
	printf("Connected\n");
	ConnectToServer(rtmp, "rtmp://192.168.1.188/live/cav");


	memset(&MetaData, 0, sizeof(RTMPMetadata));
	printf("SendH264File\n");
	//SendH264File(rtmp, &MetaData, "avc.h264");
	//printf("SendH264File11\n");
  
	pthread_create(&id,NULL,SAMPLE_VENC_1080P_CLASSIC,NULL);
	while (!g_s32Quit)
	{
		nanosleep(&ts, NULL);
		EventLoop(s32MainFd);
	}
	sleep(2);
	ringfree();
	printf("The Server quit!\n");

	return 0;
}

