#ifndef MESHINFOWIDGET_H
#define MESHINFOWIDGET_H

#include "ui_meshinfowidget.h"

#include <QTableWidget>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QVector3D>
#include <QVector4D>
#include <QIcon>

#include "imglistdialog.h"

class MeshInfoWidget : public QWidget
{
  Q_OBJECT

 public :
  MeshInfoWidget(QWidget *parent=NULL);
  ~MeshInfoWidget();
  
  void addMesh(QString, bool, bool, bool, QString, int, float);
  void setMeshes(QStringList);
  QList<int> getSelectedMeshes();
			     
  public slots :
    void setParameters(QMap<QString, QVariantList>);
    void selectMesh(int);
    void selectMeshes(QList<int>);
    void changeSelectionMode(bool);
    void clearSelection();
    void matcapFiles(QStringList);
						   
  private slots :
    void on_Command_pressed();
    void sectionClicked(int);
    void sectionDoubleClicked(int);
    void cellClicked(int, int);
    void meshListContextMenu(const QPoint&);
    void removeMesh();
    void saveMesh();
    void duplicateMesh();
    void positionChanged();
    void rotationChanged();
    void scaleChanged();
  
  signals :
    void updateGL();
    void setActive(int, bool);
    void setVisible(int, bool);
    void setVisible(QList<bool>);
    void setClip(int, bool);
    void setClip(QList<bool>);
    void setClearView(int, bool);
    void setClearView(QList<bool>);
    void removeMesh(int);
    void removeMesh(QList<int>);
    void saveMesh(int);
    void duplicateMesh(int);
    void lineModeChanged(bool);
    void lineWidthChanged(int);
    void transparencyChanged(int);
    void revealChanged(int);
    void outlineChanged(int);
    void glowChanged(int);
    void darkenChanged(int);
    void positionChanged(QVector3D);
    void rotationChanged(QVector4D);
    void scaleChanged(QVector3D);
    void colorChanged(QColor);
    void colorChanged(QList<int>, QColor);
    void materialChanged(int);  
    void materialChanged(QList<int>, int);
    void materialMixChanged(float);  
    void materialMixChanged(QList<int>, float);
    void processCommand(QList<int>, QString);
    void processCommand(int, QString);
    void processCommand(QString);
    void rotationMode(bool);
    void grabMesh(bool);
    void multiSelection(QList<int>);
  
  private :
    Ui::MeshInfoWidget ui;
  
    QTableWidget *m_meshList;
    ImgListDialog *m_matcapDialog;
    QList<QIcon> m_propIcons;
  
    QStringList m_meshNames;
    int m_prevRow;

    int m_totVertices;
    int m_totTriangles;
    QList<int> m_vertCount;
    QList<int> m_triCount;


    bool m_updatingTable;

    float getMatMix(float);
};

#endif
