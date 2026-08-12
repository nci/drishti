#include "../../../common/src/videoencoder/videoencoder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace
{
bool
fail(const QString& message)
{
  qCritical().noquote() << message;
  return false;
}

quint64
privateBytes()
{
#ifdef Q_OS_WIN
  PROCESS_MEMORY_COUNTERS_EX counters = {};
  counters.cb = sizeof(counters);
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                           sizeof(counters)))
    return static_cast<quint64>(counters.PrivateUsage);
#endif
  return 0;
}

bool
verifyMovie(const QString& filename)
{
  AVFormatContext *input = NULL;
  const QByteArray path = filename.toUtf8();
  int result = avformat_open_input(&input, path.constData(), NULL, NULL);
  if (result < 0)
    return fail("FFmpeg cannot reopen the generated movie.");
  result = avformat_find_stream_info(input, NULL);
  if (result < 0)
    {
      avformat_close_input(&input);
      return fail("FFmpeg cannot inspect the generated movie.");
    }

  bool found = false;
  for(unsigned i=0; i<input->nb_streams; ++i)
    {
      const AVCodecParameters *parameters = input->streams[i]->codecpar;
      if (parameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
          found = parameters->codec_id == AV_CODEC_ID_H264 &&
                  parameters->width == 64 &&
                  parameters->height == 64;
          break;
        }
    }
  avformat_close_input(&input);
  return found || fail("The generated movie stream has the wrong format.");
}
}

int
main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  Q_UNUSED(app);

  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("Cannot create the video smoke-test directory.") ? 0 : 1;
  const QString outputDirectory = temporary.path()+"/视频 output";
  if (!QDir().mkpath(outputDirectory))
    return fail("Cannot create the Unicode video path.") ? 0 : 1;

  VideoEncoder encoder;
  const QString missingDirectory = outputDirectory+"/missing/movie.mp4";
  if (encoder.createFile(missingDirectory, 64, 64, 1000000, 30, 30) ||
      encoder.isOk() || encoder.lastError().isEmpty())
    return fail("An unavailable movie output path was reported as successful.") ? 0 : 1;

  const QString filename = outputDirectory+"/movie 测试.mp4";
  if (encoder.createFile(filename, 63, 64, 1000000, 30, 30) ||
      encoder.isOk())
    return fail("Odd H.264 dimensions were accepted.") ? 0 : 1;

  if (!encoder.createFile(filename, 64, 64, 1000000, 30, 30))
    return fail("Cannot create the valid movie: "+encoder.lastError()) ? 0 : 1;

  QImage image(64, 64, QImage::Format_ARGB32);
  for(int frame=0; frame<30; ++frame)
    {
      image.fill(qRgb(frame, 255-frame, frame*3));
      if (!encoder.encodeImage(image))
        return fail("Cannot encode a warm-up frame: "+encoder.lastError()) ? 0 : 1;
    }

  const quint64 memoryBefore = privateBytes();
  for(int frame=0; frame<600; ++frame)
    {
      image.fill(qRgb(frame%256, (frame*3)%256, (frame*7)%256));
      if (!encoder.encodeImage(image))
        return fail("Cannot encode a regression frame: "+encoder.lastError()) ? 0 : 1;
    }
  const quint64 memoryAfter = privateBytes();
  if (memoryBefore > 0 && memoryAfter > memoryBefore+48ULL*1024ULL*1024ULL)
    return fail("Per-frame video conversion memory grew unexpectedly.") ? 0 : 1;

  QByteArray rgba(64*64*4, '\0');
  if (!encoder.encodeImage(reinterpret_cast<uchar*>(rgba.data()),
                           64, 64, 64*4, QImage::Format_RGB32))
    return fail("Cannot encode the raw RGBA movie frame: "+
                encoder.lastError()) ? 0 : 1;
  if (!encoder.close())
    return fail("Cannot finalize the movie: "+encoder.lastError()) ? 0 : 1;
  if (encoder.isOk())
    return fail("A closed movie encoder still reports ready.") ? 0 : 1;

  QFile output(filename);
  if (!output.exists() || output.size() < 512)
    return fail("The generated movie is missing or too small.") ? 0 : 1;
  if (!verifyMovie(filename))
    return 1;

  qInfo() << "Video encoder smoke passed";
  return 0;
}
