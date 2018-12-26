#ifndef VP8ENCODER_H
#define	VP8ENCODER_H

#include "video.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"

class VP8Encoder : public VideoEncoder
{
public:
	VP8Encoder(const Properties& properties);
	virtual ~VP8Encoder();
	virtual VideoFrame* EncodeFrame(BYTE *in,DWORD len);
	virtual int FastPictureUpdate();
	virtual int SetSize(int width,int height);
	virtual int SetFrameRate(int fps,int kbits,int intraPeriod);
	
	bool UnsetRoiMap();
	bool SetRoiMap(size_t x, size_t y, size_t width, size_t heigth);
private:
	int OpenCodec();
private:
	vpx_codec_ctx_t		encoder;
	vpx_codec_enc_cfg_t	config;
	vpx_image_t*		pic;
	VideoFrame*		frame;
	bool forceKeyFrame;
	int width;
	int height;
	int numPixels;
	int bitrate;
	int fps;
	int format;
	int opened;
	int intraPeriod;
	int pts;
	int num;
	int threads;
};

#endif	/* VP8ENCODER_H */

