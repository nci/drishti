#ifndef VideoEncoder_H
#define VideoEncoder_H


#include <QIODevice>
#include <QFile>
#include <QImage>

#include "ffmpeg.h"

// a wrapper around a single output AVStream
typedef struct OutputStream {
  AVStream *st;
  AVCodecContext *enc;
  
  /* pts of the next frame that will be generated */
  int64_t next_pts;
  int samples_count;
  
  AVFrame *frame;
  AVFrame *tmp_frame;
  
  AVPacket *tmp_pkt;
  
  float t, tincr, tincr2;
  
  struct SwsContext *sws_ctx;
  struct SwrContext *swr_ctx;
} OutputStream;

class VideoEncoder
{
public:
  VideoEncoder();
  ~VideoEncoder();
  
  void init();
  
  bool createFile(QString filename,
		  unsigned width, unsigned height,
		  unsigned bitrate, unsigned gop, unsigned fps=25);
  bool close();
  
  void encodeImage(const QImage &);

private:
  QString fileName;
  int FrameRate;
  unsigned Width,Height;
  unsigned Bitrate;
  unsigned Gop;
  
  AVFormatContext *avFormatCtx;
  const AVOutputFormat *avOutputFormat;
  const AVCodec *avCodec;
  AVDictionary *avDict;
  OutputStream video_st = {0};  
  
  
  // Frame conversion
  bool convertImage(OutputStream*, const QImage&);
  
  void close_stream(AVFormatContext*, OutputStream*);
  void add_stream(OutputStream*, AVFormatContext*,
		  const AVCodec**, enum AVCodecID);

  void open_video(AVFormatContext *, const AVCodec *,
		  OutputStream *, AVDictionary *);
  AVFrame* alloc_frame(enum AVPixelFormat, int, int);
  int write_frame(AVFormatContext*, AVCodecContext*,
		  AVStream*, AVFrame*, AVPacket*);
};




#endif // QVideoEncoder_H
