#include "../pluginoperationstatus.h"

#include <iostream>

namespace
{
class NoErrorMethod : public QObject
{
  Q_OBJECT
};

class DecoderStatus : public QObject
{
  Q_OBJECT

public:
  explicit DecoderStatus(const QString& error, bool canceled = false)
    : m_error(error), m_canceled(canceled) {}

  Q_INVOKABLE QString lastError() const { return m_error; }
  Q_INVOKABLE bool wasCanceled() const { return m_canceled; }

private:
  QString m_error;
  bool m_canceled;
};

int fail(const char *message)
{
  std::cerr << message << std::endl;
  return 1;
}
}

int main()
{
  if (!importPluginLastError(0).isEmpty())
    return fail("a null plugin reported an error");

  NoErrorMethod legacyPlugin;
  if (importPluginReportsOperationStatus(&legacyPlugin))
    return fail("a legacy plugin was treated as status-capable");
  if (!importPluginLastError(&legacyPlugin).isEmpty())
    return fail("a legacy plugin without lastError() was rejected");
  if (importPluginWasCanceled(&legacyPlugin))
    return fail("a legacy plugin without wasCanceled() reported cancellation");

  DecoderStatus successfulPlugin(QString{});
  if (!importPluginReportsOperationStatus(&successfulPlugin))
    return fail("a decoder with error/cancellation methods was not detected");
  if (!importPluginLastError(&successfulPlugin).isEmpty())
    return fail("an empty decoder status was treated as an error");
  if (importPluginWasCanceled(&successfulPlugin))
    return fail("a successful decoder reported cancellation");

  DecoderStatus failedPlugin(QStringLiteral("synthetic decode failure"));
  if (importPluginLastError(&failedPlugin) !=
      QStringLiteral("synthetic decode failure"))
    return fail("the decoder error was not propagated");

  DecoderStatus canceledPlugin(QString{}, true);
  if (!importPluginLastError(&canceledPlugin).isEmpty() ||
      !importPluginWasCanceled(&canceledPlugin))
    return fail("the decoder cancellation flag was not propagated");

  std::cout << "Plugin error bridge smoke passed" << std::endl;
  return 0;
}

#include "plugin_error_bridge_smoke.moc"
