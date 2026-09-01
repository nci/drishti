#ifndef PYDIALOG_H
#define PYDIALOG_H

#include <QStringList>
#include <QWidget>

struct PyDialog
{
public :
  static void setParent(QWidget*);
  static void setDataDirectory(QString);
  
  static QStringList toQStringList(const std::vector<std::string>&);
  
  static void showMessage(const std::string&, 
			  const std::string&);
  
  static void printMessage(const std::string&);
  
  static int getInt(const std::string&,
		    const std::string&,
		    int, int, int, int);
  
  static double getDouble(const std::string&,
			  const std::string&,
			  double, double, double, int, double);
  
  static std::string getText(const std::string&,
			     const std::string&,
			     const std::string&);
  
  static std::string getItem(const std::string&, 
			     const std::string&,
			     const std::vector<std::string>&,
			     int);

  static std::string saveFile(const std::string&,
			      const std::string&);
private :
  static QWidget *m_parent;
  static QString m_dataDir;
};

#endif // PYDIALOG_H

