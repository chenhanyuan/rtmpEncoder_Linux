#include "rtmpStream.h"

RTMP * AllocRtmp()
{
	RTMP *rtmp=NULL;
	rtmp = RTMP_Alloc();
	if (rtmp == NULL)
		return NULL;
	RTMP_Init(rtmp);
	return rtmp;
}

int ConnectToServer(RTMP *rtmp, char *url)
{
	if (RTMP_SetupURL(rtmp, url) < 0)
	{
		return -1;
	}
	RTMP_EnableWrite(rtmp);
	if (RTMP_Connect(rtmp, NULL) < 0)
	{
		return -1;
	}
	if (RTMP_ConnectStream(rtmp, 0) < 0)
	{
		return -1;
	}

	return 0;
}

void DisconnectToServer()
{

}

int SendAMFPacket(RTMP *rtmp, LPRTMPMetadata lpMetaData)
{
	if(lpMetaData == NULL)  
	{  
		return -1;  
	}  
	char body[1024] = {0};;  

	char * p = (char *)body; 
	p = put_byte(p, AMF_STRING );
	p = put_amf_string(p , "@setDataFrame" );

	p = put_byte( p, AMF_STRING );
	p = put_amf_string( p, "onMetaData" ); 

	//p = put_byte(p, AMF_ECMA_ARRAY );
	//p = put_be32(p, 5 ); 
	p = put_byte(p, AMF_OBJECT );
	p = put_amf_string( p, "copyright" );
	p = put_byte(p, AMF_STRING );
	p = put_amf_string( p, "firehood" ); 

	p =put_amf_string( p, "duration");
	p =put_amf_double( p, 0);

	p =put_amf_string( p, "fileSize");
	p =put_amf_double( p, 0);

	p =put_amf_string( p, "width");
	p =put_amf_double( p, lpMetaData->nWidth);

	p =put_amf_string( p, "height");
	p =put_amf_double( p, lpMetaData->nHeight);

	p =put_amf_string( p, "framerate" );
	p =put_amf_double( p, lpMetaData->nFrameRate); 

	//p =put_amf_string( p, "videocodecid" );
	//p =put_byte( p, 0x02);
	//p =put_amf_string( p, "avc1" );

	p =put_amf_string( p, "videocodecid" );
	p =put_amf_double( p, FLV_CODECID_H264 );

	//p =put_amf_string( p, "videodatarate" );
	//p =put_amf_double( p, 2500 );
	//p =put_amf_double( p, FLV_CODECID_H264 );

	p =put_amf_string( p, "" );
	p =put_byte( p, AMF_OBJECT_END  );

	int index = p-body;

	return SendPacket(rtmp, RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);

}

int SendPacket(RTMP *rtmp, unsigned int nTagType,unsigned char *data,unsigned int size,unsigned int nTimestamp)
{
	RTMPPacket* packet;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
	memset(packet,0,RTMP_HEAD_SIZE);
	/*包体内存*/
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body,data,size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nTagType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = rtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO ==nTagType && size !=4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;
	/*发送*/
	int nRet =0;
	if (RTMP_IsConnected(rtmp))
	{
		nRet = RTMP_SendPacket(rtmp, packet, TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
	return nRet; 
}


int SendVideoSpsPpsPacket(RTMP *rtmp, unsigned char *pps,int pps_len,unsigned char * sps,int sps_len)
{
	RTMPPacket * packet=NULL;//rtmp包结构
	unsigned char * body=NULL;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet,0,RTMP_HEAD_SIZE+1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i],pps,pps_len);
	i +=  pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = rtmp->m_stream_id;

	/*调用发送接口*/
	int nRet = RTMP_SendPacket(rtmp,packet,TRUE);
	free(packet);    //释放内存
	return nRet;
}


int SendH264Packet(RTMP *rtmp, LPRTMPMetadata lpMetaData, unsigned char *data,unsigned int size,int bIsKeyFrame,unsigned int nTimeStamp)
{
	if(data == NULL && size<11)
	{  
		return -1;  
	}  

	unsigned char *body = (unsigned char*)malloc(size+9);  
	memset(body,0,size+9);

	int i = 0; 
	if(bIsKeyFrame)
	{  
		body[i++] = 0x17;// 1:Iframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;  


		// NALU size   
		body[i++] = size>>24 &0xff;  
		body[i++] = size>>16 &0xff;  
		body[i++] = size>>8 &0xff;  
		body[i++] = size&0xff;
		// NALU data   
		memcpy(&body[i],data,size);  
		SendVideoSpsPpsPacket(rtmp, lpMetaData->Pps,lpMetaData->nPpsLen,lpMetaData->Sps,lpMetaData->nSpsLen);
	}
	else
	{  
		body[i++] = 0x27;// 2:Pframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;  


		// NALU size   
		body[i++] = size>>24 &0xff;  
		body[i++] = size>>16 &0xff;  
		body[i++] = size>>8 &0xff;  
		body[i++] = size&0xff;
		// NALU data   
		memcpy(&body[i],data,size);  
	}  


	int bRet = SendPacket(rtmp, RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);  

	free(body);  

	return bRet;  

}


int SendH264File(RTMP *rtmp, LPRTMPMetadata lpMetaData, char *filename)
{
	unsigned char *h264FileBuffer;
	int h264FileLength;

	int hLength=0;
	FILE *fp = NULL;
	if (filename == NULL)
	{
		DebugPrint("%s is not error", filename);
		return -1;
	}
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		DebugPrint("%s is not exist", filename);
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	h264FileLength = ftell(fp);
	h264FileBuffer = (unsigned char *)malloc(h264FileLength);
	fseek(fp, 0, SEEK_SET);
	fread(h264FileBuffer, 1, h264FileLength, fp);
	fclose(fp);

	
	Geth264FileSpsPpsData(h264FileBuffer, h264FileLength, lpMetaData);
	//parseSpsData();//解析sps，取出宽高及其他信息
	//h264_decode_sps(lpMetaData->Sps, lpMetaData->nSpsLen, &lpMetaData->nWidth,  &lpMetaData->nHeight, &lpMetaData->nFrameRate);
	lpMetaData->nWidth = 1280;
	lpMetaData->nHeight = 720;
	lpMetaData->nFrameRate = 25;
	DebugPrint("Width : %d; Heigth %d; FrameRate %d", lpMetaData->nWidth, lpMetaData->nHeight, lpMetaData->nFrameRate);
	if (lpMetaData->nFrameRate==0)
	{
		lpMetaData->nFrameRate = 25;
	}

	unsigned int tick = 0;  
	unsigned int tick_gap = 1000/lpMetaData->nFrameRate; 
	int m_CurPos = 0;
	//unsigned char *p = h264FileBuffer;
	NaluUnit nalu;
	int readSize = 0;
	while (m_CurPos<h264FileLength)
	{
		unsigned char *p = h264FileBuffer+m_CurPos;
		readSize = ReadOneNaluFromBuf(&nalu, p, h264FileLength-m_CurPos); 
		m_CurPos += readSize;
		if (nalu.type == 0x07||nalu.type == 0x08)
		{
			continue;
		}
		int bKeyframe  = (nalu.type == 0x05) ? TRUE : FALSE;
		SendH264Packet(rtmp, lpMetaData, nalu.data, nalu.size, bKeyframe, tick);
		tick += tick_gap;
		if (nalu.data != NULL)
		{
			free(nalu.data);
		}
		DebugPrint("m_Cur Pos : %d, tick : %d", m_CurPos, tick);
		usleep(tick_gap);
	}
	return 0;
}

int Geth264FileSpsPpsData(unsigned char *fileData, int fileLength, LPRTMPMetadata pMetaData)
{
	if (fileData == NULL)
	{
		DebugPrint("data is null");
		return -1;
	}
	int readNaluType=0;//设置获取标志
	int m_CurPos=0;
	int i = 0;
	unsigned char *p = fileData;
	while (i<fileLength)
	{
		int j = i;
		if (p[j++]==0x00&&p[j++]==0x00)
		{
			if ((p[j]==0x01) || (p[j++]==0x00 && p[j]==0x01))
			{
				j++;
				if ((p[j]&0x1f)==NALU_TYPE_SPS||readNaluType==NALU_TYPE_SPS)//sps
				{
					if (readNaluType ==0 )
					{
						readNaluType = NALU_TYPE_SPS;
						m_CurPos = j;//获取sps的位置
						i = j;
						continue;
					}
					else if (readNaluType == NALU_TYPE_SPS)
					{
						//拷贝数据
						pMetaData->nSpsLen = i-m_CurPos;
						pMetaData->Sps = (unsigned char *)malloc(pMetaData->nSpsLen);
						memcpy(pMetaData->Sps, &p[m_CurPos], pMetaData->nSpsLen);
						m_CurPos = 0;
						readNaluType = 0;
						i--;
					}
				}
				else if ((p[j]&0x1f)==NALU_TYPE_PPS||readNaluType==NALU_TYPE_PPS)//pps
				{
					if (readNaluType == 0 )
					{
						readNaluType = NALU_TYPE_PPS;
						m_CurPos = j;//获取sps的位置
						i = j;
						continue;
					}
					else if (readNaluType == NALU_TYPE_PPS)
					{
						//拷贝数据
						pMetaData->nPpsLen = i-m_CurPos;
						pMetaData->Pps = (unsigned char *)malloc(pMetaData->nPpsLen);
						memcpy(pMetaData->Pps, &p[m_CurPos], pMetaData->nPpsLen);
						m_CurPos = 0;
						readNaluType = 0;
						return 0;
					}
				}
			}
		}
		i++;
	}

	return 0;

}

int ReadOneNaluFromBuf(NaluUnit *nalu, unsigned char *data, int dataLength)
{
	int i = 0;
	while(i<dataLength)  
	{  
		int j = i;
		if(data[j++] == 0x00 &&  data[j++] == 0x00) 
		{  
			if ((data[j] == 0x01) || (data[j++] == 0x00 &&  data[j] == 0x01))
			{
				j++;
				int m_CurPos = j;
				nalu->type = data[j]&0x1f;
				while (j<dataLength)
				{
					int k = j;
					if (data[k++] == 0x00 &&  data[k++] == 0x00)
					{
						if ((data[k] == 0x01)||(data[k++] == 0x00 &&  data[k] == 0x01))
						{
							nalu->size = j - m_CurPos;
							nalu->data = (unsigned char *)malloc(nalu->size);
							memcpy(nalu->data, data+m_CurPos, nalu->size);
							return j;
						}
					}
					j++;
				}
				nalu->size = j - m_CurPos;
				nalu->data = (unsigned char *)malloc(nalu->size);
				memcpy(nalu->data, data+m_CurPos, nalu->size);
				return j;//最后一个Nalu的处理
			} 
		} 
		i++;
	}  
	return 0;  

}

static unsigned int tick = 0;  
static unsigned int tick_gap =0;


int putH264BufferToRtmpStream(RTMP *rtmp, LPRTMPMetadata lpMetaData, unsigned char *h264Buffer, unsigned int h264size)
{
	int m_CurPos=0;
	NaluUnit nalu;
	//static unsigned int tick = 0;  
	//static unsigned int tick_gap = RTMP_GetTime();
	while (m_CurPos < h264size)
	{
		unsigned char *p = h264Buffer+m_CurPos;
		int readSize = ReadOneNaluFromBuf(&nalu, p, h264size-m_CurPos);
		m_CurPos += readSize;

		if (nalu.type == 0x07||nalu.type == 0x08)
		{
			if (nalu.type == 0x07 && lpMetaData->nSpsLen == 0)
			{
				lpMetaData->nSpsLen = nalu.size;
				lpMetaData->Sps = (unsigned char *)malloc(lpMetaData->nSpsLen);
				memcpy(lpMetaData->Sps, nalu.data, lpMetaData->nSpsLen);
				DebugPrint("Update sps");
			}
			if (nalu.type == 0x08 && lpMetaData->nPpsLen == 0)
			{
				lpMetaData->nPpsLen = nalu.size;
				lpMetaData->Pps = (unsigned char *)malloc(lpMetaData->nPpsLen);
				memcpy(lpMetaData->Pps, nalu.data, lpMetaData->nPpsLen);
				DebugPrint("Update pps");
			}
			if (lpMetaData->nWidth == 0 && lpMetaData->nHeight == 0 && lpMetaData->nSpsLen != 0)
			{
				h264_decode_sps(lpMetaData->Sps, lpMetaData->nSpsLen, &lpMetaData->nWidth, &lpMetaData->nHeight, &lpMetaData->nFrameRate);
				
				//lpMetaData->nWidth = 1920;
				//lpMetaData->nHeight = 1080;
				//lpMetaData->nFrameRate = 30;
				DebugPrint("Width : %d; Heigth %d; FrameRate %d", lpMetaData->nWidth, lpMetaData->nHeight, lpMetaData->nFrameRate);
				if (lpMetaData->nFrameRate==0)
				{
					lpMetaData->nFrameRate = 30;
				}
				tick_gap = RTMP_GetTime();
				//SendVideoSpsPpsPacket(rtmp, lpMetaData->Pps,lpMetaData->nPpsLen,lpMetaData->Sps,lpMetaData->nSpsLen);
				continue;
				//SendAMFPacket(rtmp, lpMetaData);
				
			}
			//if(nalu.type == 0x08)
			//	SendVideoSpsPpsPacket(rtmp, lpMetaData->Pps,lpMetaData->nPpsLen,lpMetaData->Sps,lpMetaData->nSpsLen);
			continue;
			
		}
		//printf("nalu.type : %d\n", nalu.type);
		int bKeyframe  = (nalu.type == 0x05) ? TRUE : FALSE;
		SendH264Packet(rtmp, lpMetaData, nalu.data, nalu.size, bKeyframe, tick);
		tick = RTMP_GetTime() - tick_gap;
		//usleep(tick_gap);
		if (nalu.data != NULL)
		{
			free(nalu.data);
		}

	}

	return 0;

}




