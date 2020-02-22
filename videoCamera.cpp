#include "videoCamera.h"


videoCamera::videoCamera()
					:m_strInputVideoSize("640x360"),
					 m_bStop(false),
					 m_strInputFmt("video4linux2"),
					 m_strInputName("/dev/video0"),
					 m_strOutputFmt("h264"),
					 m_strOutputName("out.h264"),
					 m_pInCodecCtx(NULL),
					 m_pInFmtCtx(NULL),
					 m_pOutCodecCtx(NULL),
					 m_pOutFmtCtx(NULL),
					 m_pOStream(NULL),
					 m_pSwsCtx(NULL),
					 m_eDstPixFmt(AV_PIX_FMT_YUV420P)
{
	init();
}

videoCamera::~videoCamera()
{
	free();
}

void videoCamera::free()
{
	if ( NULL != m_pOutCodecCtx )
	{
		avcodec_free_context(&m_pOutCodecCtx);
		m_pOutCodecCtx = NULL;
	}

	if ( NULL != m_pInCodecCtx )
	{
		avcodec_free_context(&m_pInCodecCtx);
		m_pInCodecCtx = NULL;
	}
}

void videoCamera::init()
{
	av_register_all();
	avformat_network_init();
	avcodec_register_all();
	avdevice_register_all();
	//   av_log_set_level(AV_LOG_DEBUG);

	if (NULL == m_pOutCodecCtx)
	{
		m_pOutCodecCtx = avcodec_alloc_context3(NULL);
	}

	if (NULL == m_pInCodecCtx)
	{
		m_pInCodecCtx = avcodec_alloc_context3(NULL);
	}
	openInput();
	openOutput();
}

//打开输入流
int videoCamera::openInput()
{
	//1. 获取视频流格式
	AVInputFormat *pInFmt = av_find_input_format(m_strInputFmt);
	if (pInFmt == NULL)
	{
		std::cout<<"can not find input format. "<<std::endl;
		return -1;
	}

	//2. 打开视频流
	AVDictionary *option = NULL;
	av_dict_set(&option, "video_size", m_strInputVideoSize, 0);
	if (avformat_open_input(&m_pInFmtCtx, m_strInputName, pInFmt, &option)<0)
	{
		std::cout<<"can not open input file."<<std::endl;
		return -1;
	}
	//***********************

	//3. 找到视频流索引
	int videoindex = -1;
	for (unsigned int i=0; i < m_pInFmtCtx->nb_streams; i++)
	{
		if ( m_pInFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
		{
			videoindex = i;
			break;
		}
	}
	if(videoindex == -1)
	{
		std::cout<<"find video stream failed, now return."<<std::endl;
		return -1;
	}
	//***********************

	//4. 获取输入编解码Context
	if (avcodec_parameters_to_context(m_pInCodecCtx, m_pInFmtCtx->streams[videoindex]->codecpar) < 0)
	{
		std::cout<<"avcodec_parameters_to_context failed."<<std::endl;
		return -1;
	}
	std::cout<<"picture width height format"<<m_pInCodecCtx->width<<m_pInCodecCtx->height<<m_pInCodecCtx->pix_fmt<<std::endl;
	//***********************

	//5. 获取视频格式转换Context
	m_pSwsCtx = sws_getContext(m_pInCodecCtx->width, m_pInCodecCtx->height, m_pInCodecCtx->pix_fmt
	,m_pInCodecCtx->width, m_pInCodecCtx->height, m_eDstPixFmt, SWS_BILINEAR, NULL, NULL, NULL);
	//***********************

	return 0;
}

//打开输出流
int videoCamera::openOutput()
{
	//1. 获取保存视频流格式
	AVOutputFormat *pOutFmt = NULL;
	pOutFmt = av_guess_format(m_strOutputFmt,NULL, NULL);
	if (!pOutFmt)
	{
		std::cout<<"av guess format failed, now return. "<<std::endl;
		return -1;
	}
	//***********************

	//2. 打开输出流
	m_pOutFmtCtx = avformat_alloc_context();
	if (avio_open(&m_pOutFmtCtx->pb, m_strOutputName, AVIO_FLAG_READ_WRITE) < 0)
	{
		std::cout<<"Failed to open output file! \n"<<std::endl;
		return -1;
	}
	m_pOutFmtCtx->oformat = pOutFmt;
	m_pOStream = avformat_new_stream(m_pOutFmtCtx, NULL);
	if (!m_pOStream)
	{
		std::cout<<"new stream failed, now return. "<<std::endl;
		return -1;
	}
	//***********************

	//3. 获取输出编解码Context
	m_pOutCodecCtx->codec_id = pOutFmt->video_codec;
	m_pOutCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	m_pOutCodecCtx->pix_fmt = m_eDstPixFmt;
	m_pOutCodecCtx->width = m_pInCodecCtx->width;
	m_pOutCodecCtx->height = m_pInCodecCtx->height;
	m_pOutCodecCtx->time_base.num =1;
	m_pOutCodecCtx->time_base.den = 25;
	m_pOutCodecCtx->bit_rate = 400000;
	m_pOutCodecCtx->gop_size=250;
	m_pOutCodecCtx->qmin = 10;
	m_pOutCodecCtx->qmax = 51;

	AVCodec *pOutcodec = avcodec_find_encoder(m_pOutCodecCtx->codec_id);
	if (!pOutcodec)
	{
		std::cout<<"find avcodec failed. "<<std::endl;
		return -1;
	}

	// Set Option
	AVDictionary *param = 0;
	//H.264
	if(m_pOutCodecCtx->codec_id == AV_CODEC_ID_H264)
	{
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		std::cout<<"out codec is h264"<<std::endl;
		//av_dict_set(¶m, "profile", "main", 0);
	}

	if(avcodec_open2(m_pOutCodecCtx, pOutcodec, NULL) < 0)
	{
		std::cout<<"avcodec open failed. "<<std::endl;
		return -1;
	}

	return 0;
}

int videoCamera::encode()
{
	int iRet = -1;
	uint8_t *parrSrcData[4];
	uint8_t *parrDstData[4];
	int iarrSrcLinesize[4];
	int iarrDstLinesize[4];

	av_image_alloc(parrSrcData, iarrSrcLinesize, m_pInCodecCtx->width, m_pInCodecCtx->height, m_pInCodecCtx->pix_fmt, 16);
	int dst_bufsize = av_image_alloc(parrDstData, iarrDstLinesize, m_pInCodecCtx->width, m_pInCodecCtx->height, m_eDstPixFmt, 16);

	int outBufSize = av_image_get_buffer_size(m_pOutCodecCtx->pix_fmt, m_pOutCodecCtx->width, m_pOutCodecCtx->height, 16);
	std::cout<<"out buf size "<<outBufSize<<std::endl;

	AVFrame *pOutFrame = av_frame_alloc();
	if (!pOutFrame)
	{
		std::cout<<"out frame alloc failed, now return. "<<std::endl;
		return -1;
	}
	uint8_t *pFrameBuff = (uint8_t *)av_malloc(outBufSize);
	if (!pFrameBuff)
	{
		std::cout<<"frame buf alloc failed. "<<std::endl;
		return -1;
	}

	//关联AVFrame *pOutFrame和 pFrameBuff
	av_image_fill_arrays(pOutFrame->data, pOutFrame->linesize, pFrameBuff, m_pOutCodecCtx->pix_fmt, m_pOutCodecCtx->width, m_pOutCodecCtx->height, 16);
	pOutFrame->width=m_pInCodecCtx->width;
	pOutFrame->height=m_pInCodecCtx->height;
	pOutFrame->format=m_pInCodecCtx->pix_fmt;
	std::cout<<"m_pInCodecCtx->pix_fmt "<<m_pInCodecCtx->pix_fmt<<std::endl;

	//文件头
	iRet = avformat_write_header(m_pOutFmtCtx, NULL);
	switch(iRet)
	{
	case AVSTREAM_INIT_IN_WRITE_HEADER:
	  std::cout<<"avformat_write_header AVSTREAM_INIT_IN_WRITE_HEADER"<<std::endl;
	  break;
	case AVSTREAM_INIT_IN_INIT_OUTPUT:
	  std::cout<<"avformat_write_header AVSTREAM_INIT_IN_INIT_OUTPUT"<<std::endl;
	  break;
	default:
	  break;
	}

	//开始
	AVPacket stuInPkt;
	AVPacket stuOutPkt;
	memset(&stuInPkt, 0, sizeof(AVPacket));
	av_init_packet(&stuInPkt);
	memset(&stuOutPkt, 0, sizeof(AVPacket));
	av_init_packet(&stuOutPkt);

	int i = 0;
	while (!m_bStop)
	{
		i++;
		//从视频流中获取
		av_read_frame(m_pInFmtCtx, &stuInPkt);

		//视频格式变换
		memcpy(parrSrcData[0], stuInPkt.data, stuInPkt.size);
		sws_scale(m_pSwsCtx, parrSrcData, iarrSrcLinesize, 0, m_pInCodecCtx->height, parrDstData, iarrDstLinesize);
		memcpy(pFrameBuff, parrDstData[0], dst_bufsize);

		//编码
		pOutFrame->pts=i*(m_pOutCodecCtx->time_base.den)/((m_pOutCodecCtx->time_base.num)*25);
		if ( 0 != avcodec_send_frame(m_pOutCodecCtx, pOutFrame) )
		{
			std::cerr<<"avcodec_send_frame failed. "<<std::endl;
			return -1;
		}

		//获取编码结果保存到输出流
		iRet = 0;
		while( 0 == iRet)
		{
			iRet = avcodec_receive_packet(m_pOutCodecCtx, &stuOutPkt);
			switch(iRet)
			{
			case 0:
				stuOutPkt.stream_index = m_pOStream->index;
				av_write_frame(m_pOutFmtCtx, &stuOutPkt);
				break;
			case AVERROR(EAGAIN):
				break;
			default:
				std::cout<<"avcodec_receive_packet error."<<std::endl;
				terminal();
				break;
			}
		}
	}

	//结束
	av_write_trailer(m_pOutFmtCtx);

	//清理
	if (m_pOStream)
	{
		avcodec_close(m_pOutCodecCtx);
		av_free(pOutFrame);
		av_free(pFrameBuff);
	}
	avcodec_close(m_pInCodecCtx);

//	avio_close(m_pOutFmtCtx->pb);

	avformat_close_input(&m_pInFmtCtx);
	avformat_close_input(&m_pOutFmtCtx);

	avformat_free_context(m_pInFmtCtx);
	avformat_free_context(m_pOutFmtCtx);

	av_packet_unref(&stuInPkt);
	av_packet_unref(&stuOutPkt);

	av_freep(parrDstData);
	av_freep(parrSrcData);
	sws_freeContext(m_pSwsCtx);

	free();

	return 0;
}
