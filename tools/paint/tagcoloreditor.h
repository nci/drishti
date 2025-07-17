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

    QStringList tagNames();
    void setTagNames(QStringList);
    
 signals :
    void tagColorChanged();
    void tagSelected(int, bool);
    void tagNamesChanged();
    
 public slots :
    void setColors();
    void setColorGradient(QList<QColor>, int, int);
    void newColorSet(int);
    void cellClicked(int, int);
    void cellChanged(int, int);
    void cellDoubleClicked(int, int);
    void newTagsClicked();
    void showTagsClicked();
    void hideTagsClicked();

 private:
    void createGUI();

    QTableWidget *table;

    void copyGradientFile(QString);
    void askGradientChoice();
};

#endif
