/********************************************************************************
** Form generated from reading UI file 'loadrawdialog.ui'
**
** Created: Fri 25. Jan 13:35:56 2013
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOADRAWDIALOG_H
#define UI_LOADRAWDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_LoadRawDialog
{
public:
    QHBoxLayout *hboxLayout;
    QGridLayout *gridLayout;
    QLabel *label;
    QComboBox *voxelType;
    QLabel *label_2;
    QLineEdit *gridSize;
    QLabel *label_3;
    QSpinBox *headerBytes;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *LoadRawDialog)
    {
        if (LoadRawDialog->objectName().isEmpty())
            LoadRawDialog->setObjectName(QString::fromUtf8("LoadRawDialog"));
        LoadRawDialog->resize(500, 150);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(LoadRawDialog->sizePolicy().hasHeightForWidth());
        LoadRawDialog->setSizePolicy(sizePolicy);
        LoadRawDialog->setMinimumSize(QSize(300, 100));
        LoadRawDialog->setMaximumSize(QSize(500, 150));
        hboxLayout = new QHBoxLayout(LoadRawDialog);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(LoadRawDialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMinimumSize(QSize(60, 0));
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label, 0, 0, 1, 1);

        voxelType = new QComboBox(LoadRawDialog);
        voxelType->setObjectName(QString::fromUtf8("voxelType"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(voxelType->sizePolicy().hasHeightForWidth());
        voxelType->setSizePolicy(sizePolicy1);
        voxelType->setMinimumSize(QSize(100, 0));

        gridLayout->addWidget(voxelType, 0, 1, 1, 1);

        label_2 = new QLabel(LoadRawDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMinimumSize(QSize(60, 0));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        gridSize = new QLineEdit(LoadRawDialog);
        gridSize->setObjectName(QString::fromUtf8("gridSize"));
        gridSize->setMinimumSize(QSize(100, 0));

        gridLayout->addWidget(gridSize, 1, 1, 1, 1);

        label_3 = new QLabel(LoadRawDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setMinimumSize(QSize(60, 0));
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        headerBytes = new QSpinBox(LoadRawDialog);
        headerBytes->setObjectName(QString::fromUtf8("headerBytes"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(headerBytes->sizePolicy().hasHeightForWidth());
        headerBytes->setSizePolicy(sizePolicy2);
        headerBytes->setMaximumSize(QSize(50, 16777215));
        headerBytes->setMaximum(99999);

        gridLayout->addWidget(headerBytes, 2, 1, 1, 1);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem, 1, 2, 1, 1);


        hboxLayout->addLayout(gridLayout);

        buttonBox = new QDialogButtonBox(LoadRawDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Vertical);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        hboxLayout->addWidget(buttonBox);


        retranslateUi(LoadRawDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), LoadRawDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), LoadRawDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(LoadRawDialog);
    } // setupUi

    void retranslateUi(QDialog *LoadRawDialog)
    {
        LoadRawDialog->setWindowTitle(QApplication::translate("LoadRawDialog", "Load Raw Plugin Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("LoadRawDialog", "Voxel Type", 0, QApplication::UnicodeUTF8));
        voxelType->clear();
        voxelType->insertItems(0, QStringList()
         << QApplication::translate("LoadRawDialog", "unsigned byte", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LoadRawDialog", "byte", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LoadRawDialog", "unsigned short", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LoadRawDialog", "short", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LoadRawDialog", "int", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LoadRawDialog", "float", 0, QApplication::UnicodeUTF8)
        );
        label_2->setText(QApplication::translate("LoadRawDialog", "Grid Size", 0, QApplication::UnicodeUTF8));
        gridSize->setText(QString());
        label_3->setText(QApplication::translate("LoadRawDialog", "Skip Header Bytes", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class LoadRawDialog: public Ui_LoadRawDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOADRAWDIALOG_H
