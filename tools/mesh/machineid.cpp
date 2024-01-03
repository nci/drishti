#include "machineid.h"

#include <QMessageBox>

// we just need this for purposes of unique machine id. So any one or two mac's is
// fine.
quint16
MachineID::hashMacAddress(PIP_ADAPTER_INFO info)
{
  quint16 hash = 0;
  for (quint32 i = 0; i < info->AddressLength; i++) {
    hash += (info->Address[i] << ((i & 1) * 8));
  }
  return hash;
}

void
MachineID::getMacHash(quint16 &mac1, quint16 &mac2)
{
  IP_ADAPTER_INFO AdapterInfo[32];
  DWORD dwBufLen = sizeof(AdapterInfo);
  
  DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
  if (dwStatus != ERROR_SUCCESS)
    return; // no adapters.
  
  PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
  mac1 = hashMacAddress(pAdapterInfo);
  if (pAdapterInfo->Next)
    mac2 = hashMacAddress(pAdapterInfo->Next);
  
  // sort the mac addresses. We don't want to invalidate
  // both macs if they just change order.
  if (mac1 > mac2) {
    quint16 tmp = mac2;
    mac2 = mac1;
    mac1 = tmp;
  }
}

quint16
MachineID::getVolumeHash()
{
  DWORD serialNum = 0;
  
  // Determine if this volume uses an NTFS file system.
  GetVolumeInformation(L"c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
  quint16 hash = (quint16)((serialNum + (serialNum >> 16)) & 0xFFFF);
  
  return hash;
}

quint16
MachineID::getCpuHash()
{
  int cpuinfo[4] = {0, 0, 0, 0};
  __cpuid(cpuinfo, 0);
  quint16 hash = 0;
  quint16 *ptr = (quint16 * )(&cpuinfo[0]);
  for (quint32 i = 0; i < 8; i++)
    hash += ptr[i];
  
  return hash;
}

QString
MachineID::getMachineName()
{
  wchar_t computerName[1024];
  DWORD size = 1024;
  GetComputerName(computerName, &size);

  //return(QString::fromStdWString(computerName).toLatin1().data());
  return(QString::fromStdWString(computerName));
}

QString
MachineID::generateHash(QString strForHash)
{
  static char chars[] = "0123456789ABCDEF";
  
  std::stringstream stream;
  
  int size = strForHash.size();
  char* bytes = strForHash.toLatin1().data();  
  for (int i = 0; i < size; ++i)
    {
      unsigned char ch = ~((unsigned char)((unsigned short)bytes[i] +
					   (unsigned short)bytes[(i + 1) % size] +
					   (unsigned short)bytes[(i + 2) % size] +
					   (unsigned short)bytes[(i + 3) % size])) * (i + 1);
 
      stream << chars[(ch >> 4) & 0x0F] << chars[ch & 0x0F];
  }
  
  return QString::fromStdString(stream.str());
}


//Generate 5 different ids, atleast 2 should match
//in order to identify the machine

QStringList
MachineID::machineHash()
{
  int TargetLength = 32;
  

  QStringList hashStrings;
  
  quint16 mac1, mac2;
  getMacHash(mac1, mac2); 

  QString machineName = getMachineName();
  quint64 cpuHash = getCpuHash();
  quint64 volHash = getVolumeHash(); 

  {
    hashStrings << machineName;
  }
  
  {
    QString strForHash;
    strForHash = QString("%1 %2").arg(machineName).arg(cpuHash);    
    QString hashString = generateHash(strForHash);    
    hashStrings << hashString;
  }
  
  {
    QString strForHash;
    strForHash = QString("%1 %2").arg(machineName).arg(volHash);    
    QString hashString = generateHash(strForHash);    
    hashStrings << hashString;
  }

  {
    QString strForHash;
    strForHash = QString("%1 %2").arg(cpuHash).arg(volHash);    
    QString hashString = generateHash(strForHash);    
    hashStrings << hashString;
  }

  {
    QString strForHash;
    strForHash = QString("%1 %2").arg(cpuHash).arg(mac1);    
    QString hashString = generateHash(strForHash);    
    hashStrings << hashString;
  }

  {
    QString strForHash;
    strForHash = QString("%1 %2").arg(cpuHash).arg(mac2);    
    QString hashString = generateHash(strForHash);    
    hashStrings << hashString;
  }

//  while (string.size() < TargetLength) {
//    string = string + string;
//  }
//  
//  if (string.size() > TargetLength) {
//    string = string.substr(0, TargetLength);
//  }

  return hashStrings;
}

