#ifndef __VIDEO_CAMERA__
#define __VIDEO_CAMERA__
#include <iostream>
extern "C"{
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libavdevice/avdevice.h>
  #include <libswscale/swscale.h>
  #include <libavutil/imgutils.h>
}

class videoCamera{
  
  const char *inputVideoSize = "640x360";
  bool isStop = false;
  const char *input_name = "video4linux2";
  const char *file_name = "/dev/video0";
  enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUVJ420P;
  AVStream *pOStream;
  AVCodecContext *pInCodecCtx = NULL;
  AVCodecContext *pOutCodecCtx = NULL;
  AVFormatContext *pInFmtCtx = NULL;
  AVFormatContext *pOutFmtCtx = NULL;
  SwsContext *sws_ctx = NULL;
  
  int openInput();
  int openOutput();
public:
  
  videoCamera();
  int encode();
  void terminal(){isStop = true;};
};

int videoCamera::openInput(){
  
  AVInputFormat *pInFmt = av_find_input_format(input_name);
  if (pInFmt == NULL){
    std::cout<<"can not find input format. "<<std::endl;
    return -1;
  }
  
  const char *videosize1 = "640x360";
  AVDictionary *option;
  av_dict_set(&option, "video_size", videosize1, 0);
  
  if (avformat_open_input(&pInFmtCtx, file_name, pInFmt, &option)<0){
    std::cout<<"can not open input file."<<std::endl;
    return -1;
  }
  
  int videoindex = -1;
  for (int i=0;i<pInFmtCtx->nb_streams;i++){
    if(pInFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
      videoindex = i;
      break;
    }
  }
  
  if(videoindex == -1){
    std::cout<<"find video stream failed, now return."<<std::endl;
    return -1;
  }
  
  pInCodecCtx = pInFmtCtx->streams[videoindex]->codec;
  
  std::cout<<"picture width height format"<<pInCodecCtx->width<<pInCodecCtx->height<<pInCodecCtx->pix_fmt<<std::endl;
  
  sws_ctx = sws_getContext(pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt
    ,pInCodecCtx->width, pInCodecCtx->height, dst_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);
}

int videoCamera::openOutput(){
  
  const char* out_file = "out.h264";
  AVOutputFormat *pOutFmt;
  pOutFmt = av_guess_format("h264",NULL, NULL);
  if (!pOutFmt){
    std::cout<<"av guess format failed, now return. "<<std::endl;
    return -1;
  }
  
  pOutFmtCtx = avformat_alloc_context();
  if (avio_open(&pOutFmtCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0){
    printf("Failed to open output file! \n");
    return -1;
  }
  
//   avformat_alloc_output_context2(&pOutFmtCtx, NULL, NULL, out_file);  
  pOutFmtCtx->oformat = pOutFmt;  
  
  pOStream = avformat_new_stream(pOutFmtCtx, NULL);
  if (!pOStream){
    std::cout<<"new stream failed, now return. "<<std::endl;
    return -1;
  }
  
  pOutCodecCtx = pOStream->codec;
  pOutCodecCtx->codec_id = pOutFmt->video_codec;
  pOutCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  pOutCodecCtx->pix_fmt = dst_pix_fmt;
  pOutCodecCtx->width = pInCodecCtx->width;
  pOutCodecCtx->height = pInCodecCtx->height;
  pOutCodecCtx->time_base.num =1;
  pOutCodecCtx->time_base.den = 25;
  pOutCodecCtx->bit_rate = 400000;
  pOutCodecCtx->gop_size=250;
  pOutCodecCtx->qmin = 10;
  pOutCodecCtx->qmax = 51;
  
  AVCodec *pOutcodec = avcodec_find_encoder(pOutCodecCtx->codec_id);
  if (!pOutcodec){
    std::cout<<"find avcodec failed. "<<std::endl;
    return -1;
  }
  
  // Set Option
  AVDictionary *param = 0;
  //H.264
  if(pOutCodecCtx->codec_id == AV_CODEC_ID_H264) {
    av_dict_set(&param, "preset", "slow", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);
    std::cout<<"out codec is h264"<<std::endl;
    //av_dict_set(¶m, "profile", "main", 0);
  }
  
  if(avcodec_open2(pOutCodecCtx, pOutcodec, NULL) < 0){
    std::cout<<"avcodec open failed. "<<std::endl;
    return -1;
  }
  
}

videoCamera::videoCamera(){
  std::cout<<"video Camera constructor."<<std::endl;
  av_register_all();
  avformat_network_init();
  avcodec_register_all();
  avdevice_register_all();
//   av_log_set_level(AV_LOG_DEBUG);
  
  openInput();
  openOutput();
}

int videoCamera::encode(){
  
  uint8_t *src_data[4];
  uint8_t *dst_data[4];
  int src_linesize[4];
  int dst_linesize[4];

  AVPacket outPkt;
  AVPacket *pInPkt;
  int got_picture;
  
  AVFrame *pOutFrame = av_frame_alloc();
  if (!pOutFrame){
    std::cout<<"out frame alloc failed, now return. "<<std::endl;
    return -1;
  }
  
  int outBufSize = avpicture_get_size(pOutCodecCtx->pix_fmt, pOutCodecCtx->width, pOutCodecCtx->height);
  int dst_bufsize = av_image_alloc(dst_data, dst_linesize, pInCodecCtx->width, pInCodecCtx->height, dst_pix_fmt, 16);
  int src_bufsize = av_image_alloc(src_data, src_linesize, pInCodecCtx->width, pInCodecCtx->height, pInCodecCtx->pix_fmt, 16);
  
  std::cout<<"dst buf size "<<dst_bufsize<<std::endl;
  std::cout<<"out buf size "<<outBufSize<<std::endl;
  uint8_t *pFrame_buf = (uint8_t *)av_malloc(outBufSize);
  if (!pFrame_buf){
    std::cout<<"frame buf alloc failed. "<<std::endl;
    return -1;
  }
  avpicture_fill((AVPicture *)pOutFrame, pFrame_buf, pOutCodecCtx->pix_fmt, pOutCodecCtx->width, pOutCodecCtx->height);
  
  
  //Write header
  avformat_write_header(pOutFmtCtx, NULL);
  
  int i=0;
  while (!isStop){
    i++;
    //read data
    pInPkt = (AVPacket *)av_malloc(sizeof(AVPacket));
    av_read_frame(pInFmtCtx, pInPkt);
    
    //fill out data
    memcpy(src_data[0], pInPkt->data, pInPkt->size);
    sws_scale(sws_ctx, src_data, src_linesize, 0, pInCodecCtx->height, dst_data, dst_linesize);
    memcpy(pFrame_buf, dst_data[0], dst_bufsize);
    std::cout<<"dst bufsize "<<dst_bufsize<<std::endl;
    
    memset(&outPkt, 0, sizeof(AVPacket));
    av_init_packet(&outPkt);
    //encode data 
    pOutFrame->pts=i*(pOutCodecCtx->time_base.den)/((pOutCodecCtx->time_base.num)*25);
    int ret = avcodec_encode_video2(pOutCodecCtx, &outPkt,pOutFrame, &got_picture);  
    if(ret < 0){  
	printf("Encode Error.\n");  
	return -1;  
    }
    
    //flush data
    if (got_picture==1){  
      std::cout<<"ecode success, write to file."<<std::endl;
      outPkt.stream_index = pOStream->index;  
      ret = av_write_frame(pOutFmtCtx, &outPkt);  
    }  
  }
  
  av_free_packet(&outPkt); 
  
  //Write Trailer  
  av_write_trailer(pOutFmtCtx);  

  //end encode
  printf("Encode Successful.\n");  

  if (pOStream){  
      avcodec_close(pOStream->codec);  
      av_free(pOutFrame);  
      av_free(pFrame_buf);  
  }  
  avio_close(pInFmtCtx->pb);  
  avformat_close_input(&pInFmtCtx);
  
  avformat_free_context(pInFmtCtx);  
  av_free_packet(pInPkt);
  av_freep(&dst_data[0]);
  sws_freeContext(sws_ctx);
}

#endif