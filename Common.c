#include "Common.h"

#include <math.h>


char * put_byte( char *output, uint8_t nVal )
{
	output[0] = nVal;
	return output+1; 
}
char * put_be16(char *output, uint16_t nVal )
{
	output[1] = nVal & 0xff;
	output[0] = nVal >> 8;
	return output+2;
}
char * put_be24(char *output,uint32_t nVal )
{
	output[2] = nVal & 0xff;
	output[1] = nVal >> 8;
	output[0] = nVal >> 16;
	return output+3;
} 
char * put_be32(char *output, uint32_t nVal )    
{    
	output[3] = nVal & 0xff;    
	output[2] = nVal >> 8;    
	output[1] = nVal >> 16;    
	output[0] = nVal >> 24;    
	return output+4;    
}    
char *  put_be64( char *output, uint64_t nVal )    
{    
	output=put_be32( output, nVal >> 32 );    
	output=put_be32( output, nVal );    
	return output;    
}    
char * put_amf_string( char *c, const char *str )    
{    
	uint16_t len = strlen( str );    
	c=put_be16( c, len );    
	memcpy(c,str,len);    
	return c+len;    
}    
char * put_amf_double( char *c, double d )    
{    
	*c++ = AMF_NUMBER;  /* type: Number */    
	{    
		unsigned char *ci, *co;    
		ci = (unsigned char *)&d;    
		co = (unsigned char *)c;    
		co[0] = ci[7];    
		co[1] = ci[6];    
		co[2] = ci[5];    
		co[3] = ci[4];    
		co[4] = ci[3];    
		co[5] = ci[2];    
		co[6] = ci[1];    
		co[7] = ci[0];    
	}    
	return c+8;    
}

UINT Ue(BYTE *pBuff, UINT nLen, UINT *nStartBit)
{
	//计算0bit的个数
	UINT nZeroNum = 0;
	while (*nStartBit < nLen * 8)
	{
		if (pBuff[*nStartBit / 8] & (0x80 >> (*nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		(*nStartBit)++;
	}
	(*nStartBit)++;


	//计算结果
	DWORD dwRet = 0;
	UINT i;
	for (i=0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
		{
			dwRet += 1;
		}
		(*nStartBit)++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}


int Se(BYTE *pBuff, UINT nLen, UINT *nStartBit)
{
	int UeVal=Ue(pBuff,nLen,nStartBit);
	double k=UeVal;
	int nValue=ceil(k/2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2==0)
		nValue=-nValue;
	return nValue;
}


DWORD u(UINT BitCount,BYTE * buf,UINT *nStartBit)
{
	DWORD dwRet = 0;
	UINT i;
	for (i=0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[*nStartBit / 8] & (0x80 >> (*nStartBit % 8)))
		{
			dwRet += 1;
		}
		(*nStartBit)++;
	}
	return dwRet;
}

/**
 * H264的NAL起始码防竞争机制 
 *
 * @param buf SPS数据内容
 *
 * @无返回值
 */ 
void de_emulation_prevention(BYTE* buf,unsigned int* buf_size)
{
	int i=0,j=0;
	BYTE* tmp_ptr=NULL;
	unsigned int tmp_buf_size=0;
	int val=0;

	tmp_ptr=buf;
	tmp_buf_size=*buf_size;
	for(i=0;i<(tmp_buf_size-2);i++)
	{
		//check for 0x000003
		val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);
		if(val==0)
		{
			//kick out 0x03
			for(j=i+2;j<tmp_buf_size-1;j++)
				tmp_ptr[j]=tmp_ptr[j+1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}

/**
 * 解码SPS,获取视频图像宽、高信息 
 *
 * @param buf SPS数据内容
 * @param nLen SPS数据的长度
 * @param width 图像宽度
 * @param height 图像高度

 * @成功则返回1 , 失败则返回0
 */ 
int h264_decode_sps(BYTE * buf,unsigned int nLen,int *width,int *height,int *fps)
{
	UINT starBit=0;
	UINT *StartBit=&starBit; 
	fps=0;
	de_emulation_prevention(buf,&nLen);

	int forbidden_zero_bit=u(1,buf,StartBit);
	int nal_ref_idc=u(2, buf, StartBit);
	int nal_unit_type=u(5,buf,StartBit);
	if(nal_unit_type==7)
	{
		int profile_idc=u(8,buf,StartBit);
		int constraint_set0_flag=u(1,buf,StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag=u(1,buf,StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag=u(1,buf,StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag=u(1,buf,StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits=u(4,buf,StartBit);
		int level_idc=u(8,buf,StartBit);

		int seq_parameter_set_id=Ue(buf,nLen,StartBit);

		if( profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144 )
		{
			int chroma_format_idc=Ue(buf,nLen,StartBit);
			if( chroma_format_idc == 3 )
			{
				int residual_colour_transform_flag=u(1,buf,StartBit);
			}
			int bit_depth_luma_minus8=Ue(buf,nLen,StartBit);
			int bit_depth_chroma_minus8=Ue(buf,nLen,StartBit);
			int qpprime_y_zero_transform_bypass_flag=u(1,buf,StartBit);
			int seq_scaling_matrix_present_flag=u(1,buf,StartBit);

			int seq_scaling_list_present_flag[8];
			if( seq_scaling_matrix_present_flag )
			{
				int i;
				for( i = 0; i < 8; i++ ) 
				{
					seq_scaling_list_present_flag[i]=u(1,buf,StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4=Ue(buf,nLen,StartBit);
		int pic_order_cnt_type=Ue(buf,nLen,StartBit);
		if( pic_order_cnt_type == 0 )
		{
			int log2_max_pic_order_cnt_lsb_minus4=Ue(buf,nLen,StartBit);
		}
		else if( pic_order_cnt_type == 1 )
		{
			int delta_pic_order_always_zero_flag=u(1,buf,StartBit);
			int offset_for_non_ref_pic=Se(buf,nLen,StartBit);
			int offset_for_top_to_bottom_field=Se(buf,nLen,StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle=Ue(buf,nLen,StartBit);

			int *offset_for_ref_frame=(int *)malloc(num_ref_frames_in_pic_order_cnt_cycle);
			int i;
			for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
				offset_for_ref_frame[i]=Se(buf,nLen,StartBit);
			free(offset_for_ref_frame);
		}
		int num_ref_frames=Ue(buf,nLen,StartBit);
		int gaps_in_frame_num_value_allowed_flag=u(1,buf,StartBit);
		int pic_width_in_mbs_minus1=Ue(buf,nLen,StartBit);
		int pic_height_in_map_units_minus1=Ue(buf,nLen,StartBit);
		
		*width=(pic_width_in_mbs_minus1+1)*16;
		*height=(pic_height_in_map_units_minus1+1)*16;

		int frame_mbs_only_flag=u(1,buf,StartBit);
		if(!frame_mbs_only_flag)
		{
			int mb_adaptive_frame_field_flag=u(1,buf,StartBit);
		}
		int direct_8x8_inference_flag=u(1,buf,StartBit);
		int frame_cropping_flag=u(1,buf,StartBit);
		if(frame_cropping_flag)
		{
			int frame_crop_left_offset=Ue(buf,nLen,StartBit);
			int frame_crop_right_offset=Ue(buf,nLen,StartBit);
			int frame_crop_top_offset=Ue(buf,nLen,StartBit);
			int frame_crop_bottom_offset=Ue(buf,nLen,StartBit);
		}
		int vui_parameter_present_flag=u(1,buf,StartBit);
		if(vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag=u(1,buf,StartBit);              
			if(aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc=u(8,buf,StartBit);   
				if(aspect_ratio_idc==255)
				{
					int sar_width=u(16,buf,StartBit);                                  
					int sar_height=u(16,buf,StartBit);                                      
				}
			}
			int overscan_info_present_flag=u(1,buf,StartBit); 
			if(overscan_info_present_flag)
			{
				int overscan_appropriate_flagu=u(1,buf,StartBit);     
			}
			int video_signal_type_present_flag=u(1,buf,StartBit); 
			if(video_signal_type_present_flag)
			{
				int video_format=u(3,buf,StartBit);                         
				int video_full_range_flag=u(1,buf,StartBit);                       
				int colour_description_present_flag=u(1,buf,StartBit);
				if(colour_description_present_flag)
				{
					int colour_primaries=u(8,buf,StartBit);              
					int transfer_characteristics=u(8,buf,StartBit);                     
					int matrix_coefficients=u(8,buf,StartBit);                  		
				}
			}
			int chroma_loc_info_present_flag=u(1,buf,StartBit);  
			if(chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field=Ue(buf,nLen,StartBit);             
				int chroma_sample_loc_type_bottom_field=Ue(buf,nLen,StartBit);       
			}
			int timing_info_present_flag=u(1,buf,StartBit);        
			if(timing_info_present_flag)
			{
				int num_units_in_tick=u(32,buf,StartBit);                              
				int time_scale=u(32,buf,StartBit);    
				*fps=time_scale/(2*num_units_in_tick);
			}
		}
		return 1;
	}
	else
		return 0;
}





