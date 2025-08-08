#include <QPainter>
#include "videoencoder.h"
#include "ffmpeg.h"

#include <QMessageBox>
#include <QString>


#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum) {
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif  // av_err2str


/**
  gop: maximal interval in frames between keyframes
**/
VideoEncoder::VideoEncoder()
{
  avFormatCtx=0;
  avOutputFormat=0;
  avCodec=0;
  avDict = NULL;
}

VideoEncoder::~VideoEncoder()
{
  close();
}

void
VideoEncoder::init()
{
  avFormatCtx=0;
  avOutputFormat=0;
  avCodec=0;
  avDict = NULL;
}


/* Add an output stream. */
void
VideoEncoder::add_stream(OutputStream *ost, AVFormatContext *oc,
			 const AVCodec **codec,
			 enum AVCodecID codec_id)
{
  AVCodecContext *c;
  int i;
  
  /* find the encoder */
  *codec = avcodec_find_encoder(codec_id);
  if (!(*codec))
    {
      QMessageBox::information(0, "Error", QString("Could not find encoder for : %1"). \
			       arg(avcodec_get_name(codec_id)));
      return;
    }
 
  ost->tmp_pkt = av_packet_alloc();
  if (!ost->tmp_pkt)
    {
      QMessageBox::information(0, "Error", "Could not allocate AVPacket");
      return;
    }
  
  ost->st = avformat_new_stream(oc, NULL);
  if (!ost->st)
    {
      QMessageBox::information(0, "Error", "Could not allocate stream");
      return;
    }
  ost->st->id = oc->nb_streams-1;
  c = avcodec_alloc_context3(*codec);
  if (!c)
    {
      QMessageBox::information(0, "Error", "Could not alloc an encoding context");
      return;
    }
  ost->enc = c;
  
  c->codec_id = codec_id;
    
  //------------
  //------------
  c->bit_rate = Bitrate;
  /* Resolution must be a multiple of two. */
  c->width    = Width;
  c->height   = Height;
  /* timebase: This is the fundamental unit of time (in seconds) in terms
   * of which frame timestamps are represented. For fixed-fps content,
   * timebase should be 1/framerate and timestamp increments should be
   * identical to 1. */
  ost->st->time_base = AVRational{1, FrameRate};
  c->time_base       = ost->st->time_base;
  
  c->gop_size      = Gop; /* emit one intra frame every Gop at most */
  c->pix_fmt       = AV_PIX_FMT_YUV420P;
  //------------
  //------------

  ost->st->avg_frame_rate = AVRational{FrameRate, 1};
  
  /* Some formats want stream headers to be separate. */
  if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}
 
AVFrame*
VideoEncoder::alloc_frame(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *frame;
    int ret;
 
    frame = av_frame_alloc();
    if (!frame)
        return NULL;
 
    frame->format = pix_fmt;
    frame->width  = width;
    frame->height = height;
 
    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }
 
    return frame;
}
 
void
VideoEncoder::open_video(AVFormatContext *oc, const AVCodec *codec,
			  OutputStream *ost, AVDictionary *opt_arg)
{
  int ret;
  AVCodecContext *c = ost->enc;
  AVDictionary *opt = NULL;
  
  av_dict_copy(&opt, opt_arg, 0);
  
  /* open the codec */
  ret = avcodec_open2(c, codec, &opt);
  av_dict_free(&opt);
  if (ret < 0)
    {
      QMessageBox::information(0, "Error", QString("Could not open video codec: %1").arg(av_err2str(ret)));
      return;
    }
 
  /* allocate and init a re-usable frame */
  ost->frame = alloc_frame(c->pix_fmt, c->width, c->height);
  if (!ost->frame)
    {
      QMessageBox::information(0, "Error", "Could not allocate video frame");
      return;
    }
  
  /* If the output format is not YUV420P, then a temporary YUV420P
   * picture is needed too. It is then converted to the required
   * output format. */
  ost->tmp_frame = NULL;
  if (c->pix_fmt != AV_PIX_FMT_YUV420P)
    {
      ost->tmp_frame = alloc_frame(AV_PIX_FMT_YUV420P, c->width, c->height);
      if (!ost->tmp_frame)
	{
	  QMessageBox::information(0, "Error", "Could not allocate temporary video frame");
	  return;
	}
    }
      
    /* copy the stream parameters to the muxer */
  ret = avcodec_parameters_from_context(ost->st->codecpar, c);
  if (ret < 0)
    {
      QMessageBox::information(0, "Error", "Could not copy the stream parameters");
      return;
    }
}

bool VideoEncoder::createFile(QString fileName,
			       unsigned width,unsigned height,
			       unsigned bitrate,unsigned gop,unsigned fps)
{
  int ret;
  
  Width=width;
  Height=height;
  Gop=gop;
  Bitrate=bitrate;
  FrameRate=fps;

  
  avformat_alloc_output_context2(&avFormatCtx, NULL, NULL, fileName.toStdString().c_str());
  if (!avFormatCtx) {
    printf("Could not deduce output format from file extension: using MP4.\n");
    avformat_alloc_output_context2(&avFormatCtx, NULL, "mp4", fileName.toStdString().c_str());
  }
  if (!avFormatCtx)
    return false;
  
  avOutputFormat = avFormatCtx->oformat;
  
  /* Add video stream using the default format codecs
   * and initialize the codecs. */
//  if (avOutputFormat->video_codec != AV_CODEC_ID_NONE) {
//    add_stream(&video_st, avFormatCtx, &avCodec, avOutputFormat->video_codec);
//  }
  add_stream(&video_st, avFormatCtx, &avCodec, AV_CODEC_ID_H264);
  
  /* Now that all the parameters are set, we can open the audio and
   * video codecs and allocate the necessary encode buffers. */
  avDict = NULL;
  open_video(avFormatCtx, avCodec, &video_st, avDict);
  
  //----
  // print output format to screen
  //av_dump_format(oc, 0, fileName.toStdString().c_str(), 1);
  //----
  
  /* open the output file, if needed */
  if (!(avOutputFormat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&avFormatCtx->pb, fileName.toStdString().c_str(), AVIO_FLAG_WRITE);
    if (ret < 0)
      {
	QMessageBox::information(0, "Error", QString("Could not open '%1': %2"). \
				 arg(fileName).arg(av_err2str(ret)));
	return 1;
      }
  }
  
  /* Write the stream header, if any. */
  ret = avformat_write_header(avFormatCtx, &avDict);
  if (ret < 0)
    {
      QMessageBox::information(0, "Error",
			       QString("Error occurred when opening output file: %1"). \
			       arg(av_err2str(ret)));
      return 1;
    }
  
  //QMessageBox::information(0, "", QString("File created : %1 : %2 %3").arg(fileName).arg(Width).arg(Height));

  return true;
}

void
VideoEncoder::close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}
 

/**
   \brief Completes writing the stream, closes it, release resources.
**/
bool VideoEncoder::close()
{
  if (avFormatCtx == 0)
    return false;
  
  write_frame(avFormatCtx, video_st.enc, video_st.st, NULL, video_st.tmp_pkt);
  
  av_write_trailer(avFormatCtx);
  
  close_stream(avFormatCtx, &video_st);
  
  // Close file
  avio_closep(&avFormatCtx->pb);
  
  // Free the stream
  avformat_free_context(avFormatCtx);
  
  return true;
}

int
VideoEncoder::write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
			   AVStream *st, AVFrame *frame, AVPacket *pkt)
{
    int ret;
 
    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
      QMessageBox::information(0, "Error", QString("Error sending a frame to the encoder: %1").\
			       arg(av_err2str(ret)));
      return 1;
    }
    
    while (ret >= 0) {
      ret = avcodec_receive_packet(c, pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
	break;
      else if (ret < 0) {
	QMessageBox::information(0, "Error", QString("Error encoding a frame: %1").arg(av_err2str(ret)));
	return 1;
      }
 
      /* rescale output packet timestamp values from codec to stream timebase */
      av_packet_rescale_ts(pkt, c->time_base, st->time_base);
      pkt->stream_index = st->index;
 
      /* Write the compressed frame to the media file. */
      //log_packet(fmt_ctx, pkt);
      ret = av_interleaved_write_frame(fmt_ctx, pkt);
      /* pkt is now blank (av_interleaved_write_frame() takes ownership of
       * its contents and resets pkt), so that no unreferencing is necessary.
       * This would be different if one used av_write_frame(). */
      if (ret < 0) {
	QMessageBox::information(0, "Error", QString("Error while writing output packet: %1").arg(av_err2str(ret)));
	return 1;
      }
    }
    
    return ret == AVERROR_EOF ? 1 : 0;
}
 
/**
   \brief Encode one frame

   The frame must be of the same size as specified in the createFile call.

   This is the standard method to encode videos with fixed frame rates.
   Each call to encodeImage adds a frame, which will be played back at the frame rate
   specified in the createFile call.
**/
void
VideoEncoder::encodeImage(const QImage &img)
{
  convertImage(&video_st, img);
  video_st.frame->pts = video_st.next_pts++;

  write_frame(avFormatCtx, video_st.enc, video_st.st, video_st.frame, video_st.tmp_pkt);
}


/**
  \brief Convert the QImage to the internal YUV format

  SWS conversion

   Caution: the QImage is allocated by QT without guarantee about the alignment and bytes per lines.
   It *should* be okay as we make sure the image is a multiple of many bytes (8 or 16)...
   ... however it is not guaranteed that sws_scale won't at some point require more bytes per line.
   We keep the custom conversion for that case.

**/

bool VideoEncoder::convertImage(OutputStream *ost, const QImage &img)
{
   // Check if the image matches the size
   if(img.width()!= Width || img.height()!= Height)
   {
      printf("Wrong image size!\n");
      return false;
   } 
   if(img.format()!=QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
   {
      printf("Wrong image format\n");
      return false;
   }

   ost->sws_ctx = sws_getContext(Width, Height,
				 AV_PIX_FMT_BGRA,
				 Width, Height,
				 AV_PIX_FMT_YUV420P,
				 SWS_BICUBIC,
				 NULL, NULL, NULL);

   if (ost->sws_ctx == NULL)
   {
      printf("Cannot initialize the conversion context\n");
      return false;
   }

   uint8_t *srcplanes[3];
   srcplanes[0]=(uint8_t*)img.bits();
   srcplanes[1]=0;
   srcplanes[2]=0;

   int srcstride[3];
   srcstride[0]=img.bytesPerLine();
   srcstride[1]=0;
   srcstride[2]=0;


   sws_scale(ost->sws_ctx,
	     srcplanes,
	     srcstride, 0, Height,
	     ost->frame->data, ost->frame->linesize);

   return true;
}

