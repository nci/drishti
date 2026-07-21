#ifndef PYWIDGET_H
#define PYWIDGET_H

#include <QStringList>

struct PyWidget
{
    public :
        static QStringList toQStringList(const std::vector<std::string>&);
        
        static void showMessage(const std::string&, 
                        const std::string&);
        
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
};

#endif // PYWIDGET_H

