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
    void tagClicked(int);

 public slots :
    void setColors();
    void newColorSet(int);
    void setOpacity(float);

 private slots :
    void itemChanged(QTableWidgetItem*);
    void tagClicked(int, int);
   
 private:
    void createGUI();

    QTableWidget *table;

    void copyGradientFile(QString);
    void askGradientChoice();
};

#endif
