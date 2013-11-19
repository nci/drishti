/********************************************************************************
** Form generated from reading UI file 'propertyeditor.ui'
**
** Created: Mon Nov 18 14:45:05 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROPERTYEDITOR_H
#define UI_PROPERTYEDITOR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QSplitter>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PropertyEditor
{
public:
    QVBoxLayout *verticalLayout_2;
    QSplitter *splitter_2;
    QSplitter *splitter;
    QPlainTextEdit *messageLabel;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *propertyBox;
    QGroupBox *commandString;
    QHBoxLayout *horizontalLayout;
    QLineEdit *lineEdit;
    QGroupBox *commandHelp;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *PropertyEditor)
    {
        if (PropertyEditor->objectName().isEmpty())
            PropertyEditor->setObjectName(QString::fromUtf8("PropertyEditor"));
        PropertyEditor->resize(538, 405);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PropertyEditor->sizePolicy().hasHeightForWidth());
        PropertyEditor->setSizePolicy(sizePolicy);
        PropertyEditor->setMaximumSize(QSize(1000, 16777215));
        PropertyEditor->setSizeGripEnabled(true);
        verticalLayout_2 = new QVBoxLayout(PropertyEditor);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        splitter_2 = new QSplitter(PropertyEditor);
        splitter_2->setObjectName(QString::fromUtf8("splitter_2"));
        splitter_2->setOrientation(Qt::Horizontal);
        splitter = new QSplitter(splitter_2);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Vertical);
        messageLabel = new QPlainTextEdit(splitter);
        messageLabel->setObjectName(QString::fromUtf8("messageLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(messageLabel->sizePolicy().hasHeightForWidth());
        messageLabel->setSizePolicy(sizePolicy1);
        messageLabel->setMinimumSize(QSize(0, 20));
        messageLabel->setMaximumSize(QSize(600, 200));
        messageLabel->setFocusPolicy(Qt::NoFocus);
        messageLabel->setFrameShape(QFrame::NoFrame);
        messageLabel->setFrameShadow(QFrame::Plain);
        messageLabel->setReadOnly(true);
        splitter->addWidget(messageLabel);
        layoutWidget = new QWidget(splitter);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        propertyBox = new QGroupBox(layoutWidget);
        propertyBox->setObjectName(QString::fromUtf8("propertyBox"));
        QSizePolicy sizePolicy2(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(propertyBox->sizePolicy().hasHeightForWidth());
        propertyBox->setSizePolicy(sizePolicy2);

        verticalLayout->addWidget(propertyBox);

        commandString = new QGroupBox(layoutWidget);
        commandString->setObjectName(QString::fromUtf8("commandString"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(commandString->sizePolicy().hasHeightForWidth());
        commandString->setSizePolicy(sizePolicy3);
        commandString->setMinimumSize(QSize(0, 70));
        commandString->setMaximumSize(QSize(16777215, 90));
        horizontalLayout = new QHBoxLayout(commandString);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        lineEdit = new QLineEdit(commandString);
        lineEdit->setObjectName(QString::fromUtf8("lineEdit"));

        horizontalLayout->addWidget(lineEdit);


        verticalLayout->addWidget(commandString);

        splitter->addWidget(layoutWidget);
        splitter_2->addWidget(splitter);
        commandHelp = new QGroupBox(splitter_2);
        commandHelp->setObjectName(QString::fromUtf8("commandHelp"));
        QSizePolicy sizePolicy4(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(commandHelp->sizePolicy().hasHeightForWidth());
        commandHelp->setSizePolicy(sizePolicy4);
        splitter_2->addWidget(commandHelp);

        verticalLayout_2->addWidget(splitter_2);

        buttonBox = new QDialogButtonBox(PropertyEditor);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy5);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_2->addWidget(buttonBox);


        retranslateUi(PropertyEditor);
        QObject::connect(buttonBox, SIGNAL(accepted()), PropertyEditor, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), PropertyEditor, SLOT(reject()));

        QMetaObject::connectSlotsByName(PropertyEditor);
    } // setupUi

    void retranslateUi(QDialog *PropertyEditor)
    {
        PropertyEditor->setWindowTitle(QApplication::translate("PropertyEditor", "Dialog", 0, QApplication::UnicodeUTF8));
        propertyBox->setTitle(QApplication::translate("PropertyEditor", "Property Editor", 0, QApplication::UnicodeUTF8));
        commandString->setTitle(QApplication::translate("PropertyEditor", "Command String", 0, QApplication::UnicodeUTF8));
        commandHelp->setTitle(QApplication::translate("PropertyEditor", "Command Help", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PropertyEditor: public Ui_PropertyEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROPERTYEDITOR_H
