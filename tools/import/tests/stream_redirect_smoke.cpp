#include "../../../common/src/widgets/streamredirect.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QTextDocument>
#include <QThread>

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace
{
int fail(const char *message)
{
  std::fprintf(stderr, "FAIL: %s\n", message);
  return 1;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication application(argc, argv);

  QTextEdit *widget = QtStreamRedirect::logWidget();
  if (!widget)
    return fail("The log widget was not created on the GUI thread");

  QtStreamRedirect redirect(widget);
  qInstallMessageHandler(QtStreamRedirect::qtMessageHandler);

  const int threadCount = 4;
  const int linesPerThread = 200;
  std::vector<std::thread> workers;
  for (int thread = 0; thread < threadCount; ++thread)
    {
      workers.emplace_back([thread, linesPerThread, &redirect]() {
        for (int line = 0; line < linesPerThread; ++line)
          {
            const std::string text =
              "stream-" + std::to_string(thread) + "-" +
              std::to_string(line) + "\n";
            if (redirect.sputn(text.data(),
                               static_cast<std::streamsize>(text.size())) !=
                static_cast<std::streamsize>(text.size()))
              std::abort();
            qWarning("qt-%d-%d", thread, line);
          }
      });
    }
  for (std::thread& worker : workers)
    worker.join();

  const int expectedLines = threadCount*linesPerThread*2;
  QElapsedTimer timer;
  timer.start();
  while (widget->document()->blockCount() < expectedLines &&
         timer.elapsed() < 10000)
    {
      application.processEvents();
      QThread::msleep(1);
    }

  const QString output = widget->toPlainText();
  if (!output.contains("stream-0-0") ||
      !output.contains("stream-3-199") ||
      !output.contains("[WARNING] qt-0-0") ||
      !output.contains("[WARNING] qt-3-199"))
    return fail("Queued background-thread output was lost");

  delete widget;
  qWarning("message after log widget destruction");
  application.processEvents();
  qInstallMessageHandler(nullptr);

  std::printf("PASS: %d background log messages were delivered safely\n",
              expectedLines);
  return 0;
}
