#ifndef _RTMPSTREAM_H_
#define _RTMPSTREAM_H_

#include "Common.h"

RTMP * AllocRtmp();//rtmp初始化

int ConnectToServer(RTMP *rtmp, char *url);

void DisconnectToServer();

int SendAMFPacket(RTMP *rtmp, LPRTMPMetadata lpMetaData);

int SendPacket(RTMP *rtmp, unsigned int nTagType,unsigned char *data,unsigned int size,unsigned int nTimestamp);  

int SendVideoSpsPpsPacket(RTMP *rtmp, unsigned char *pps,int pps_len,unsigned char * sps,int sps_len);

int SendH264Packet(RTMP *rtmp, LPRTMPMetadata lpMetaData, unsigned char *data,unsigned int size,int bIsKeyFrame,unsigned int nTimeStamp);

//file referent 可读取h264文件进行发送
int SendH264File(RTMP *rtmp, LPRTMPMetadata lpMetaData, char *filename);

int Geth264FileSpsPpsData(unsigned char *fileData, int fileLength, LPRTMPMetadata pMetaData);
	//bool parseSpsData(LPRTMPMetadata pMetaData);

int ReadOneNaluFromBuf(NaluUnit *nalu, unsigned char *data, int dataLength);//h264数据获取每一个Nalu，可以处理文件也可以处理缓存

//H264 缓存数据的操作
int putH264BufferToRtmpStream(RTMP *rtmp, LPRTMPMetadata lpMetaData, unsigned char *h264Buffer, unsigned int h264size);

#endif

