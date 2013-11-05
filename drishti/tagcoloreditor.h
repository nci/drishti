#ifndef TAGCOLOREDITOR_H
#define TAGCOLOREDITOR_H

#include <QtGui>

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
    void setOpacity(float);

 private slots :
    void itemChanged(QTableWidgetItem*);
   
 private:
    void createGUI();

    QTableWidget *table;

    void copyGradientFile(QString);
    void askGradientChoice();
};

#endif
