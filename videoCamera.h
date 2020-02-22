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
  
public:
  videoCamera();
  ~videoCamera();
  int encode();
  void terminal(){m_bStop = true;};

private:
  int openInput();
  int openOutput();
  void init();
  void free();

private:
	const char *m_strInputVideoSize;
	bool m_bStop;

	const char *m_strInputFmt;
	const char *m_strInputName;
	const char *m_strOutputFmt;
	const char *m_strOutputName;

	AVCodecContext *m_pInCodecCtx;
	AVFormatContext *m_pInFmtCtx;

	AVCodecContext *m_pOutCodecCtx;
	AVFormatContext *m_pOutFmtCtx;
	AVStream *m_pOStream;

	SwsContext *m_pSwsCtx;
	enum AVPixelFormat m_eDstPixFmt;
};

#endif
