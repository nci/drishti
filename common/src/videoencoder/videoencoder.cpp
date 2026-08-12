#include "videoencoder.h"
#include "ffmpeg.h"

#include <QByteArray>
#include <QString>

#include <limits>
#include <string>

#ifdef av_err2str
#undef av_err2str
av_always_inline std::string av_err2string(int errnum)
{
  char text[AV_ERROR_MAX_STRING_SIZE];
  return av_make_error_string(text, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif

VideoEncoder::VideoEncoder()
{
  m_avFormatCtx = NULL;
  m_avOutputFormat = NULL;
  m_avCodec = NULL;
  m_avDict = NULL;
  m_videoStream = {0};

  m_frameRate = 0;
  m_width = 0;
  m_height = 0;
  m_bitrate = 0;
  m_gop = 0;
  m_ready = false;
  m_headerWritten = false;
}

VideoEncoder::~VideoEncoder()
{
  close();
}

void
VideoEncoder::init()
{
  release(m_headerWritten);
  m_lastError.clear();
}

bool
VideoEncoder::setError(const QString& error)
{
  m_lastError = error;
  return false;
}

bool
VideoEncoder::add_stream(OutputStream *ost,
                         AVFormatContext *formatContext,
                         const AVCodec **codec,
                         enum AVCodecID codecId)
{
  if (!ost || !formatContext || !codec)
    return setError("The video stream parameters are invalid.");

  *codec = avcodec_find_encoder(codecId);
  if (!*codec)
    return setError(QString("Could not find an encoder for %1.")
                    .arg(avcodec_get_name(codecId)));

  ost->tmp_pkt = av_packet_alloc();
  if (!ost->tmp_pkt)
    return setError("Could not allocate the video packet buffer.");

  ost->st = avformat_new_stream(formatContext, NULL);
  if (!ost->st)
    return setError("Could not allocate the video stream.");
  ost->st->id = formatContext->nb_streams-1;

  ost->enc = avcodec_alloc_context3(*codec);
  if (!ost->enc)
    return setError("Could not allocate the video encoding context.");

  AVCodecContext *encoder = ost->enc;
  encoder->codec_id = codecId;
  encoder->bit_rate = m_bitrate;
  encoder->width = static_cast<int>(m_width);
  encoder->height = static_cast<int>(m_height);
  ost->st->time_base = AVRational{1, m_frameRate};
  encoder->framerate = AVRational{m_frameRate, 1};
  encoder->time_base = ost->st->time_base;
  encoder->gop_size = static_cast<int>(m_gop);
  encoder->pix_fmt = AV_PIX_FMT_YUV420P;
  ost->st->avg_frame_rate = AVRational{m_frameRate, 1};

  if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
    encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  return true;
}

AVFrame*
VideoEncoder::alloc_frame(enum AVPixelFormat pixelFormat, int width, int height)
{
  AVFrame *frame = av_frame_alloc();
  if (!frame)
    return NULL;

  frame->format = pixelFormat;
  frame->width = width;
  frame->height = height;
  if (av_frame_get_buffer(frame, 0) < 0)
    {
      av_frame_free(&frame);
      return NULL;
    }
  return frame;
}

bool
VideoEncoder::open_video(AVFormatContext *formatContext,
                         const AVCodec *codec,
                         OutputStream *ost,
                         AVDictionary *options)
{
  if (!formatContext || !codec || !ost || !ost->enc || !ost->st)
    return setError("The video stream was not initialized.");

  AVDictionary *localOptions = NULL;
  int result = av_dict_copy(&localOptions, options, 0);
  if (result < 0)
    return setError(QString("Could not copy the video options: %1")
                    .arg(av_err2str(result)));

  result = avcodec_open2(ost->enc, codec, &localOptions);
  av_dict_free(&localOptions);
  if (result < 0)
    return setError(QString("Could not open the video codec: %1")
                    .arg(av_err2str(result)));

  ost->frame = alloc_frame(ost->enc->pix_fmt,
                           ost->enc->width,
                           ost->enc->height);
  if (!ost->frame)
    return setError("Could not allocate the video frame.");

  if (ost->enc->pix_fmt != AV_PIX_FMT_YUV420P)
    {
      ost->tmp_frame = alloc_frame(AV_PIX_FMT_YUV420P,
                                   ost->enc->width,
                                   ost->enc->height);
      if (!ost->tmp_frame)
        return setError("Could not allocate the temporary video frame.");
    }

  result = avcodec_parameters_from_context(ost->st->codecpar, ost->enc);
  if (result < 0)
    return setError(QString("Could not copy the video stream parameters: %1")
                    .arg(av_err2str(result)));
  return true;
}

bool
VideoEncoder::createFile(QString fileName,
                         unsigned width,
                         unsigned height,
                         unsigned bitrate,
                         unsigned gop,
                         unsigned fps)
{
  if (m_avFormatCtx && !close())
    return false;
  m_lastError.clear();

  if (fileName.isEmpty())
    return setError("No movie output filename was supplied.");
  if (width == 0 || height == 0 ||
      width > static_cast<unsigned>(std::numeric_limits<int>::max()) ||
      height > static_cast<unsigned>(std::numeric_limits<int>::max()))
    return setError("The movie dimensions are invalid.");
  if ((width & 1U) != 0 || (height & 1U) != 0)
    return setError("H.264 movie dimensions must be even.");
  if (bitrate == 0 || gop == 0 || fps == 0 ||
      gop > static_cast<unsigned>(std::numeric_limits<int>::max()) ||
      fps > static_cast<unsigned>(std::numeric_limits<int>::max()))
    return setError("The movie bitrate, GOP, or frame rate is invalid.");

  m_width = width;
  m_height = height;
  m_gop = gop;
  m_bitrate = bitrate;
  m_frameRate = static_cast<int>(fps);

  const QByteArray encodedFileName = fileName.toUtf8();
  avformat_alloc_output_context2(&m_avFormatCtx, NULL, NULL,
                                 encodedFileName.constData());
  if (!m_avFormatCtx)
    avformat_alloc_output_context2(&m_avFormatCtx, NULL, "mp4",
                                   encodedFileName.constData());
  if (!m_avFormatCtx)
    return setError("Could not create the movie output context.");

  m_avOutputFormat = m_avFormatCtx->oformat;
  if (!add_stream(&m_videoStream, m_avFormatCtx,
                  &m_avCodec, AV_CODEC_ID_H264) ||
      !open_video(m_avFormatCtx, m_avCodec, &m_videoStream, m_avDict))
    {
      release(false);
      return false;
    }

  if (!(m_avOutputFormat->flags & AVFMT_NOFILE))
    {
      const int result = avio_open(&m_avFormatCtx->pb,
                                   encodedFileName.constData(),
                                   AVIO_FLAG_WRITE);
      if (result < 0)
        {
          setError(QString("Could not open movie output '%1': %2")
                   .arg(fileName, av_err2str(result)));
          release(false);
          return false;
        }
    }

  const int result = avformat_write_header(m_avFormatCtx, &m_avDict);
  if (result < 0)
    {
      setError(QString("Could not write the movie header: %1")
               .arg(av_err2str(result)));
      release(false);
      return false;
    }

  m_headerWritten = true;
  m_ready = true;
  return true;
}

void
VideoEncoder::close_stream(OutputStream *ost)
{
  if (!ost)
    return;
  avcodec_free_context(&ost->enc);
  av_frame_free(&ost->frame);
  av_frame_free(&ost->tmp_frame);
  av_packet_free(&ost->tmp_pkt);
  sws_freeContext(ost->sws_ctx);
  swr_free(&ost->swr_ctx);
}

bool
VideoEncoder::release(bool finalize)
{
  bool ok = true;
  if (m_avFormatCtx)
    {
      if (finalize && m_headerWritten)
        {
          if (!write_frame(m_avFormatCtx, m_videoStream.enc,
                           m_videoStream.st, NULL, m_videoStream.tmp_pkt))
            ok = false;

          const int trailerResult = av_write_trailer(m_avFormatCtx);
          if (trailerResult < 0)
            {
              setError(QString("Could not finalize the movie: %1")
                       .arg(av_err2str(trailerResult)));
              ok = false;
            }
        }

      close_stream(&m_videoStream);
      if (m_avFormatCtx->pb)
        {
          const int closeResult = avio_closep(&m_avFormatCtx->pb);
          if (finalize && closeResult < 0)
            {
              setError(QString("Could not close the movie output: %1")
                       .arg(av_err2str(closeResult)));
              ok = false;
            }
        }
      avformat_free_context(m_avFormatCtx);
    }

  av_dict_free(&m_avDict);
  m_avFormatCtx = NULL;
  m_avOutputFormat = NULL;
  m_avCodec = NULL;
  m_videoStream = {0};
  m_frameRate = 0;
  m_width = 0;
  m_height = 0;
  m_bitrate = 0;
  m_gop = 0;
  m_ready = false;
  m_headerWritten = false;
  return ok;
}

bool
VideoEncoder::close()
{
  if (!m_avFormatCtx)
    return true;
  return release(m_headerWritten);
}

bool
VideoEncoder::write_frame(AVFormatContext *formatContext,
                          AVCodecContext *encoder,
                          AVStream *stream,
                          AVFrame *frame,
                          AVPacket *packet)
{
  if (!formatContext || !encoder || !stream || !packet)
    return setError("The movie encoder is not ready to write a frame.");

  int result = avcodec_send_frame(encoder, frame);
  if (result < 0)
    return setError(QString("Error sending a frame to the encoder: %1")
                    .arg(av_err2str(result)));

  while (true)
    {
      result = avcodec_receive_packet(encoder, packet);
      if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        return true;
      if (result < 0)
        return setError(QString("Error encoding a movie frame: %1")
                        .arg(av_err2str(result)));

      av_packet_rescale_ts(packet, encoder->time_base, stream->time_base);
      packet->stream_index = stream->index;
      result = av_interleaved_write_frame(formatContext, packet);
      if (result < 0)
        return setError(QString("Error writing a movie packet: %1")
                        .arg(av_err2str(result)));
    }
}

bool
VideoEncoder::encodeImage(const QImage& image)
{
  if (!m_ready || !m_videoStream.frame)
    return setError("The movie encoder is not ready.");

  const int writable = av_frame_make_writable(m_videoStream.frame);
  if (writable < 0)
    {
      m_ready = false;
      return setError(QString("Could not make the movie frame writable: %1")
                      .arg(av_err2str(writable)));
    }
  if (!convertImage(&m_videoStream, image))
    {
      m_ready = false;
      return false;
    }

  m_videoStream.frame->pts = m_videoStream.next_pts++;
  if (!write_frame(m_avFormatCtx, m_videoStream.enc, m_videoStream.st,
                   m_videoStream.frame, m_videoStream.tmp_pkt))
    {
      m_ready = false;
      return false;
    }
  return true;
}

bool
VideoEncoder::encodeImage(uchar *image,
                          int width,
                          int height,
                          int bytesPerLine,
                          int format)
{
  if (!m_ready || !m_videoStream.frame)
    return setError("The movie encoder is not ready.");

  const int writable = av_frame_make_writable(m_videoStream.frame);
  if (writable < 0)
    {
      m_ready = false;
      return setError(QString("Could not make the movie frame writable: %1")
                      .arg(av_err2str(writable)));
    }
  if (!convertImage(&m_videoStream, image, width, height,
                    bytesPerLine, format))
    {
      m_ready = false;
      return false;
    }

  m_videoStream.frame->pts = m_videoStream.next_pts++;
  m_videoStream.frame->time_base = AVRational{1, m_frameRate};
  if (!write_frame(m_avFormatCtx, m_videoStream.enc, m_videoStream.st,
                   m_videoStream.frame, m_videoStream.tmp_pkt))
    {
      m_ready = false;
      return false;
    }
  return true;
}

bool
VideoEncoder::convertImage(OutputStream *ost, const QImage& image)
{
  if (!ost || !ost->frame || image.isNull() ||
      image.width() != static_cast<int>(m_width) ||
      image.height() != static_cast<int>(m_height))
    return setError(QString("Wrong movie image size: expected %1 x %2, got %3 x %4.")
                    .arg(m_width).arg(m_height)
                    .arg(image.width()).arg(image.height()));

  AVPixelFormat sourceFormat;
  if (image.format() == QImage::Format_ARGB32 ||
      image.format() == QImage::Format_RGB32)
    sourceFormat = AV_PIX_FMT_BGRA;
  else if (image.format() == QImage::Format_RGBA8888)
    sourceFormat = AV_PIX_FMT_RGBA;
  else
    return setError("The movie image format is unsupported.");

  ost->sws_ctx = sws_getCachedContext(
    ost->sws_ctx,
    static_cast<int>(m_width), static_cast<int>(m_height), sourceFormat,
    static_cast<int>(m_width), static_cast<int>(m_height), AV_PIX_FMT_YUV420P,
    SWS_BICUBIC, NULL, NULL, NULL);
  if (!ost->sws_ctx)
    return setError("Cannot initialize the movie color conversion context.");

  const uint8_t *sourcePlanes[4] = {
    reinterpret_cast<const uint8_t*>(image.constBits()), NULL, NULL, NULL
  };
  const int sourceStrides[4] = {image.bytesPerLine(), 0, 0, 0};
  const int rows = sws_scale(ost->sws_ctx,
                             sourcePlanes,
                             sourceStrides,
                             0,
                             static_cast<int>(m_height),
                             ost->frame->data,
                             ost->frame->linesize);
  if (rows != static_cast<int>(m_height))
    return setError("The movie frame color conversion was incomplete.");
  return true;
}

bool
VideoEncoder::convertImage(OutputStream *ost,
                           uchar *image,
                           int width,
                           int height,
                           int bytesPerLine,
                           int format)
{
  if (!ost || !ost->frame || !image ||
      width != static_cast<int>(m_width) ||
      height != static_cast<int>(m_height) ||
      bytesPerLine < 0 ||
      static_cast<qint64>(bytesPerLine) < static_cast<qint64>(width)*4)
    return setError(QString("Wrong movie image buffer: expected %1 x %2.")
                    .arg(m_width).arg(m_height));

  AVPixelFormat sourceFormat;
  if (format == QImage::Format_ARGB32)
    sourceFormat = AV_PIX_FMT_BGRA;
  else if (format == QImage::Format_RGB32 ||
           format == QImage::Format_RGBA8888)
    sourceFormat = AV_PIX_FMT_RGBA;
  else
    return setError("The movie image buffer format is unsupported.");

  ost->sws_ctx = sws_getCachedContext(
    ost->sws_ctx,
    static_cast<int>(m_width), static_cast<int>(m_height), sourceFormat,
    static_cast<int>(m_width), static_cast<int>(m_height), AV_PIX_FMT_YUV420P,
    SWS_BICUBIC, NULL, NULL, NULL);
  if (!ost->sws_ctx)
    return setError("Cannot initialize the movie color conversion context.");

  const uint8_t *sourcePlanes[4] = {
    reinterpret_cast<const uint8_t*>(image), NULL, NULL, NULL
  };
  const int sourceStrides[4] = {bytesPerLine, 0, 0, 0};
  const int rows = sws_scale(ost->sws_ctx,
                             sourcePlanes,
                             sourceStrides,
                             0,
                             static_cast<int>(m_height),
                             ost->frame->data,
                             ost->frame->linesize);
  if (rows != static_cast<int>(m_height))
    return setError("The movie frame color conversion was incomplete.");
  return true;
}
