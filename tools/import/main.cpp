#include "drishtiimport.h"

int main(int argv, char **args)
{
    QApplication app(argv, args);

    DrishtiImport mainWindow;
    mainWindow.show();

    return app.exec();
}
