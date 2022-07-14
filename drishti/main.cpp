#include <GL/glew.h>

#include <QApplication>
#include "mainwindow.h"

#include <QTranslator>


int main(int argc, char** argv)
{  
#if defined(Q_OS_WIN32)
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  int MYargc = 3;
  char *MYargv[] = {(char*)"Appname", (char*)"--platform", (char*)"windows:dpiawareness=0"};
  QApplication application(MYargc, MYargv);
#else
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication application(argc, argv);   
#endif
  
  //------------------------------------------------------------
  //------------------------------------------------------------
  // choose the application
  {
    QStringList choices;
    choices << "Drishti";
    choices << "Drishti Import";
    choices << "Drishti Paint";

    QInputDialog appChooser;
    QFont font;
    font.setPointSize(16);
    appChooser.setFont(font);
    appChooser.setOption(QInputDialog::UseListViewForComboBoxItems);
    appChooser.setWindowTitle(QWidget::tr("Choose Application to Run"));
    appChooser.setLabelText(QWidget::tr("Applications"));
    appChooser.setComboBoxItems(choices);
    int ok = appChooser.exec();

    if (ok == QDialog::Accepted)
      {
	QString choice = appChooser.textValue();
	
	if (choice == "Drishti Import")
	  {
#if defined(Q_OS_WIN32)
	    QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtiimport.exe");
#else
	    QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtiimport");
#endif
	    exit(0);
	  }

	if (choice == "Drishti Paint")
	  {
#if defined(Q_OS_WIN32)
	    QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtipaint.exe");
#else
	    QProcess::startDetached(qApp->applicationDirPath() + QDir::separator() + "drishtipaint");
#endif
	    exit(0);
	  }

      }
  }
  //------------------------------------------------------------
  //------------------------------------------------------------

     
  QGLFormat glFormat;
  glFormat.setSampleBuffers(true);
  glFormat.setDoubleBuffer(true);
  glFormat.setRgba(true);
  glFormat.setAlpha(true);
  glFormat.setDepth(true);

//  //-----------------------------
//  // did not work - still getting 8bit buffers
//  glFormat.setAlphaBufferSize(16);
//  glFormat.setRedBufferSize(16);
//  glFormat.setGreenBufferSize(16);
//  glFormat.setBlueBufferSize(16);
//  //-----------------------------

  if (argc > 1)
    {
      if (QString::compare(argv[1], "-stereo", Qt::CaseInsensitive) == 0)
	glFormat.setStereo(true);
    }

  QGLFormat::setDefaultFormat(glFormat);

  //----------------------
  QTranslator translator;
  translator.load(qApp->applicationDirPath() +
		  QDir::separator() +
		  "drishtitr_ch");
  application.installTranslator(&translator);
  //----------------------
  
  MainWindow mainwindow;
  mainwindow.show();
  
  // Run main loop.
  return application.exec();
}
