#ifndef IMGLISTDIALOG_H
#define IMGLISTDIALOG_H

#include <QListWidget>
#include <QIcon>
#include <QDialog>
#include <QDialogButtonBox>

class ImgListDialog : public QDialog
{
  Q_OBJECT

 public :
  ImgListDialog(QWidget *parent=0,
	       Qt::WindowFlags f=Qt::MSWindowsFixedSizeDialogHint);

  void setIcons(QStringList);

  int getImageId(int);
  
  public slots :
    void itemDoubleClicked(QListWidgetItem*);
    void itemClicked(QListWidgetItem*);
    
 private :    
  QListWidget *m_listWidget;  
  
  QDialogButtonBox *buttonBox;
  QPushButton *okButton;
  QPushButton *cancelButton;

  int m_id;
  
};

#endif
