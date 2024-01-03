#ifndef MACHINEID
#define MACHINEID

#include <QtGlobal>
#include <QString>

#include <sstream>
#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>

#define WIN32_LEAN_AND_MEAN

class MachineID
{
 public :
  static QStringList machineHash();

 private :
  static quint16 hashMacAddress(PIP_ADAPTER_INFO);
  static void getMacHash(quint16&, quint16&);
  static quint16 getVolumeHash();
  static quint16 getCpuHash();
  static QString getMachineName();
  static QString generateHash(QString);
};

#endif
