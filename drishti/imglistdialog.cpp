#include "imglistdialog.h"
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QScreen>

ImgListDialog::ImgListDialog(QWidget *parent,
			     Qt::WindowFlags f) :
  QDialog(parent, f | Qt::WindowStaysOnTopHint)
{
  setWindowTitle(tr("Select Material"));
		 
  m_listWidget = new QListWidget(parent);

  buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
				   QDialogButtonBox::Cancel);
  buttonBox->setOrientation(Qt::Horizontal);

  QVBoxLayout *layout = new QVBoxLayout;
  //layout->setContentsMargins(margin, margin, margin, margin);
  layout->addWidget(m_listWidget);
  layout->addWidget(buttonBox);
  layout->setAlignment(buttonBox, Qt::AlignHCenter);
  setLayout(layout);

  connect(buttonBox, SIGNAL(accepted()),
	  this, SLOT(accept()));

  connect(buttonBox, SIGNAL(rejected()),
	  this, SLOT(reject()));


  connect(m_listWidget, SIGNAL(itemClicked(QListWidgetItem*)),
	  this, SLOT(itemClicked(QListWidgetItem*)));

  connect(m_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
	  this, SLOT(itemDoubleClicked(QListWidgetItem*)));

  m_id = 0;

  resize(600, 550);
}

int
ImgListDialog::getImageId(int initial)
{  
  //move(QCursor::pos());

  // make sure the pop up is fully visible
  QScreen *scr = qGuiApp->screenAt(QCursor::pos());
  QRect geo = scr->geometry();
  int x = QCursor::pos().x();
  int y = QCursor::pos().y();
  x = qMin(x, geo.right()-650);
  y = qMin(y, geo.bottom()-600);
  move(QPoint(x,y));

  
  show();
  m_listWidget->setCurrentRow(initial);
  
  int ret = exec();
  
  if (ret == QDialog::Accepted)
    return m_id;
  else
    return initial;
}

void
ImgListDialog::itemClicked(QListWidgetItem *item)
{
  m_id = item->text().toInt();  
}

void
ImgListDialog::itemDoubleClicked(QListWidgetItem *item)
{
  m_id = item->text().toInt();  
  accept();
}


void
ImgListDialog::setIcons(QStringList list)
{
  m_listWidget->clear();
  m_listWidget->setViewMode(QListWidget::IconMode);
  m_listWidget->setIconSize(QSize(100, 100));
  //m_listWidget->setResizeMode(QListWidget::Fixed);

  QPixmap defaultMat(100, 100);
  defaultMat.fill(Qt::white);
  m_listWidget->addItem(new QListWidgetItem(QIcon(defaultMat), "0"));
  for (int i=0; i<list.size(); i++)
    {
      m_listWidget->addItem(new QListWidgetItem(QIcon(list[i]), QString("%1").arg(i+1)));
    }
}  
