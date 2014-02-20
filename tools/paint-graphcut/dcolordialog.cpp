#include "dcolordialog.h"
#include <QColorDialog>
#include <QVBoxLayout>

DColorDialog::DColorDialog(QColor color,
			 QWidget *parent,
			 Qt::WindowFlags f) :
  QDialog(parent, f)
{
  setWindowTitle(QColorDialog::tr("Select color"));  

  m_color = color;
  

  moreButton = new QPushButton(tr("&More >>"));

  buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
				   QDialogButtonBox::Cancel);
  buttonBox->addButton(moreButton, QDialogButtonBox::ActionRole);
  buttonBox->setOrientation(Qt::Horizontal);

  int margin = 5;

  colorwheel = new DColorWheel(this, margin);
  colorwheel->setGeometry(0, 0, 200, 200);
  colorwheel->setColor(m_color);


  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(margin, margin, margin, margin);
  layout->addWidget(colorwheel);
  layout->addWidget(buttonBox);
  layout->setAlignment(buttonBox, Qt::AlignHCenter);
  setLayout(layout);


  connect(moreButton, SIGNAL(clicked(bool)),
	  this, SLOT(more(bool)));

  connect(buttonBox, SIGNAL(accepted()),
	  this, SLOT(accept()));

  connect(buttonBox, SIGNAL(rejected()),
	  this, SLOT(reject()));

  connect(colorwheel, SIGNAL(colorChanged()),
	  this, SLOT(colorChanged()));


  resize(300, 330);
}

void
DColorDialog::more(bool)
{
  QColor color = QColorDialog::getColor(m_color, 0);
  if (color.isValid())
    m_color = color;

  colorwheel->setColor(m_color);
}

void
DColorDialog::colorChanged()
{
  m_color = colorwheel->getColor();
}

void
DColorDialog::setColor(QColor col)
{
  m_color = col;
  colorwheel->setColor(m_color);
}

QColor
DColorDialog::color()
{
  return m_color;
}

QColor
DColorDialog::getColor(const QColor& initial)
{
  DColorDialog *dlg = new DColorDialog(initial);
  //dlg->setWindowTitle(QColorDialog::tr("Select color"));
  int ret = dlg->exec();

  QColor color;
  if (ret == QDialog::Accepted)
    color = dlg->color();

  delete dlg;

  return color;
}
