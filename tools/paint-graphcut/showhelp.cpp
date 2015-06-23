#include "showhelp.h"
#include "propertyeditor.h"

void
ShowHelp::showCurvesHelp()
{
  PropertyEditor propertyEditor;

  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/curves.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }  
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Curves Help", plist, keys);
  propertyEditor.exec();
}

void
ShowHelp::showGraphCutHelp()
{
  PropertyEditor propertyEditor;

  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/graphcut.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }  
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Graph Cut Segmentation Help", plist, keys);
  propertyEditor.exec();
}

void
ShowHelp::showFibersHelp()
{
  PropertyEditor propertyEditor;

  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/fibers.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }  
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Fiber/Channel Tracking Help", plist, keys);
  propertyEditor.exec();
}

void
ShowHelp::showMainHelp()
{  
  PropertyEditor propertyEditor;

  QMap<QString, QVariantList> plist;
  QVariantList vlist;

  vlist.clear();
  QFile helpFile(":/paint.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }  
  plist["commandhelp"] = vlist;
  
  QStringList keys;
  keys << "commandhelp";
  
  propertyEditor.set("Drishti Paint Help", plist, keys);
  propertyEditor.exec();
}

