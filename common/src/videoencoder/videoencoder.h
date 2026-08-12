#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H


#include <QIODevice>
#include <QFile>
#include <QImage>
#include <QString>

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

  bool isOk() const { return m_ready; }
  QString lastError() const { return m_lastError; }
  
  bool createFile(QString filename,
		  unsigned width, unsigned height,
		  unsigned bitrate, unsigned gop, unsigned fps=25);
  bool close();
  
  bool encodeImage(const QImage &);
  bool encodeImage(uchar*, int, int, int, int);

private:
  int m_frameRate;
  unsigned m_width,m_height;
  unsigned m_bitrate;
  unsigned m_gop;
  bool m_ready;
  bool m_headerWritten;
  QString m_lastError;
  
  AVFormatContext *m_avFormatCtx;
  const AVOutputFormat *m_avOutputFormat;
  const AVCodec *m_avCodec;
  AVDictionary *m_avDict;
  OutputStream m_videoStream = {0};  
  
  
  // Frame conversion
  bool convertImage(OutputStream*, const QImage&);
  bool convertImage(OutputStream*,
		    uchar*, int, int, int, int);

  void close_stream(OutputStream*);
  bool add_stream(OutputStream*, AVFormatContext*,
		  const AVCodec**, enum AVCodecID);

  bool open_video(AVFormatContext *, const AVCodec *,
		  OutputStream *, AVDictionary *);
  AVFrame* alloc_frame(enum AVPixelFormat, int, int);
  bool write_frame(AVFormatContext*, AVCodecContext*,
		   AVStream*, AVFrame*, AVPacket*);
  bool release(bool finalize);
  bool setError(const QString&);
};




#endif // QVideoEncoder_H
