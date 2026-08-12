#include "scriptsplugin.h"

#include <QApplication>
#include <QFileInfo>

#include <iostream>

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);

  if (argc != 3)
    {
      std::cerr << "Usage: python_script_plugin_smoke <raw.json> <numpy_array.json>\n";
      return 2;
    }

  ScriptsPlugin plugin;
  for (int argument = 1; argument < argc; ++argument)
    {
      const QString jsonFile =
        QFileInfo(QString::fromLocal8Bit(argv[argument])).absoluteFilePath();
      if (!plugin.start(jsonFile))
        {
          std::cerr << "Cannot start script plugin: "
                    << jsonFile.toStdString() << "\n";
          return 3;
        }
    }

  std::cout << "Portable Python import-script smoke passed\n";
  return 0;
}
