#include "drishtipaint.h"

int main(int argv, char **args)
{
    QApplication app(argv, args);

    DrishtiPaint mainWindow;
    mainWindow.show();

    return app.exec();
}
