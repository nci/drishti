#ifndef PLUGINOPERATIONSTATUS_H
#define PLUGINOPERATIONSTATUS_H

#include <QMetaObject>
#include <QObject>
#include <QString>

inline bool importPluginReportsOperationStatus(QObject *pluginObject)
{
  return pluginObject &&
    pluginObject->metaObject()->indexOfMethod("lastError()") >= 0 &&
    pluginObject->metaObject()->indexOfMethod("wasCanceled()") >= 0;
}

inline QString importPluginLastError(QObject *pluginObject)
{
  if (!pluginObject ||
      pluginObject->metaObject()->indexOfMethod("lastError()") < 0)
    return QString();

  QString error;
  if (!QMetaObject::invokeMethod(pluginObject, "lastError",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QString, error)))
    return QStringLiteral(
      "The volume decoder exposes lastError(), but it could not be invoked.");

  return error;
}

inline bool importPluginWasCanceled(QObject *pluginObject)
{
  if (!pluginObject ||
      pluginObject->metaObject()->indexOfMethod("wasCanceled()") < 0)
    return false;

  bool canceled = false;
  if (!QMetaObject::invokeMethod(pluginObject, "wasCanceled",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(bool, canceled)))
    return false;

  return canceled;
}

#endif
