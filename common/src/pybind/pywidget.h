#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <QStringList>

class PyWidget
{
    public :
        static QStringList toQStringList(const std::vector<std::string>&);
        
        static void showMessage(const std::string&, 
                                const std::string&);
        
        static std::string getItem(const std::string&, 
                                   const std::string&,
                      const std::vector<std::string>&,
                                                 int);
};

#endif // PYWIDGET_H

