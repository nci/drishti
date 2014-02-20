#ifndef TAGCOLOREDITOR_H
#define TAGCOLOREDITOR_H

#include "commonqtclasses.h"
#include <QTableWidget>
#include <QTableWidgetItem>

class TagColorEditor : public QWidget
{
    Q_OBJECT

 public:
    TagColorEditor();

 signals :
    void tagColorChanged();

 public slots :
    void setColors();
    void newColorSet(int);
    void cellClicked(int, int);
    void newTagsClicked();

 private:
    void createGUI();

    QTableWidget *table;

    void copyGradientFile(QString);
    void askGradientChoice();
};

#endif
