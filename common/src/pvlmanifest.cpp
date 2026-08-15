#include "pvlmanifest.h"

#include <QDataStream>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegExp>
#include <QSet>

#include <cstring>
#include <limits>
#include <cmath>

PvlManifest::PvlManifest()
  : valid(false), depth(0), width(0), height(0), slabSize(0),
    headerSize(13), rawHeaderSize(13),
    voxelType(PvlManifestParser::UnsignedChar),
    rawVoxelType(PvlManifestParser::UnsignedChar),
    voxelSizeX(1), voxelSizeY(1), voxelSizeZ(1),
    isColor(false)
{
}

bool
PvlManifestParser::fail(PvlManifest& manifest, const QString& error)
{
  manifest.valid = false;
  manifest.error = error;
  return false;
}

bool
PvlManifestParser::readDocument(const QString& filename,
                                QDomDocument& document,
                                QString& error)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
    {
      error = QString("cannot open PVL header '%1': %2")
        .arg(filename).arg(file.errorString());
      return false;
    }

  QString parseError;
  int line = 0;
  int column = 0;
  if (!document.setContent(&file, &parseError, &line, &column))
    {
      error = QString("invalid PVL header '%1' at %2:%3: %4")
        .arg(filename).arg(line).arg(column).arg(parseError);
      return false;
    }
  return true;
}

bool
PvlManifestParser::readRequiredInt(const QDomElement& root,
                                   const QString& name,
                                   int& value,
                                   QString& error)
{
  QDomElement node = root.firstChildElement(name);
  if (node.isNull() || !node.nextSiblingElement(name).isNull())
    {
      error = QString("PVL header must contain exactly one <%1>").arg(name);
      return false;
    }
  if (!node.firstChildElement().isNull())
    {
      error = QString("PVL <%1> must contain text only").arg(name);
      return false;
    }

  bool ok = false;
  const qlonglong parsed = node.text().trimmed().toLongLong(&ok);
  if (!ok || parsed <= 0 || parsed > std::numeric_limits<int>::max())
    {
      error = QString("PVL <%1> must be a positive 32-bit integer").arg(name);
      return false;
    }
  value = static_cast<int>(parsed);
  return true;
}

bool
PvlManifestParser::readTriple(const QDomElement& root,
                              const QString& name,
                              int& first,
                              int& second,
                              int& third,
                              QString& error)
{
  QDomElement node = root.firstChildElement(name);
  if (node.isNull() || !node.nextSiblingElement(name).isNull())
    {
      error = QString("PVL header must contain exactly one <%1>").arg(name);
      return false;
    }
  if (!node.firstChildElement().isNull())
    {
      error = QString("PVL <%1> must contain text only").arg(name);
      return false;
    }

  const QStringList values = node.text().split(
    QRegExp("\\s+"), QString::SkipEmptyParts);
  if (values.count() != 3)
    {
      error = QString("PVL <%1> must contain exactly three values").arg(name);
      return false;
    }

  bool ok0 = false;
  bool ok1 = false;
  bool ok2 = false;
  const qlonglong v0 = values.at(0).toLongLong(&ok0);
  const qlonglong v1 = values.at(1).toLongLong(&ok1);
  const qlonglong v2 = values.at(2).toLongLong(&ok2);
  if (!ok0 || !ok1 || !ok2 || v0 <= 0 || v1 <= 0 || v2 <= 0 ||
      v0 > std::numeric_limits<int>::max() ||
      v1 > std::numeric_limits<int>::max() ||
      v2 > std::numeric_limits<int>::max())
    {
      error = QString("PVL <%1> contains invalid dimensions").arg(name);
      return false;
    }

  first = static_cast<int>(v0);
  second = static_cast<int>(v1);
  third = static_cast<int>(v2);
  return true;
}

bool
PvlManifestParser::readVoxelType(const QDomElement& root,
                                 const QString& tag,
                                 int& voxelType,
                                 QString& error)
{
  QDomElement node = root.firstChildElement(tag);
  if (node.isNull() || !node.nextSiblingElement(tag).isNull())
    {
      error = QString("PVL header must contain exactly one <%1>").arg(tag);
      return false;
    }
  if (!node.firstChildElement().isNull())
    {
      error = QString("PVL <%1> must contain text only").arg(tag);
      return false;
    }

  const QString value = node.text().trimmed();
  if (value == "unsigned char") voxelType = UnsignedChar;
  else if (value == "char") voxelType = Char;
  else if (value == "unsigned short") voxelType = UnsignedShort;
  else if (value == "short") voxelType = Short;
  else if (value == "int") voxelType = Int;
  else if (value == "float") voxelType = Float;
  else if (value == "RGB") voxelType = RGB;
  else if (value == "RGBA") voxelType = RGBA;
  else
    {
      error = QString("unsupported PVL voxel type '%1'").arg(value);
      return false;
    }
  return true;
}

QStringList
PvlManifestParser::readNames(const QDomElement& root,
                             const QString& tag,
                             QString& error,
                             int expectedCount)
{
  QStringList names;
  QDomElement container = root.firstChildElement(tag);
  if (!container.isNull() && !container.nextSiblingElement(tag).isNull())
    {
      error = QString("PVL header must contain at most one <%1>").arg(tag);
      return names;
    }
  if (container.isNull())
    return names;

  const QDomNodeList children = container.childNodes();
  bool hasStructuredChildren = false;
  bool hasNonWhitespaceText = false;
  for (int i = 0; i < children.count(); ++i)
    {
      const QDomNode child = children.at(i);
      if (child.isElement())
        {
          if (child.nodeName() != "name" && child.nodeName() != "file")
            {
              error = QString("unknown <%1> child <%2>")
                .arg(tag).arg(child.nodeName());
              return QStringList();
            }
          if (!child.toElement().firstChildElement().isNull())
            {
              error = QString("PVL <%1> entries must contain text only")
                .arg(tag);
              return QStringList();
            }
          hasStructuredChildren = true;
          const QString value = child.toElement().text().trimmed();
          if (value.trimmed().isEmpty())
            {
              error = QString("empty <%1> entry in <%2>")
                .arg(child.nodeName()).arg(tag);
              return QStringList();
            }
          names << value;
        }
      else if (child.isText() && !child.nodeValue().trimmed().isEmpty())
        hasNonWhitespaceText = true;
    }

  if (hasStructuredChildren && hasNonWhitespaceText)
    {
      error = QString("PVL <%1> cannot mix structured entries with text")
        .arg(tag);
      return QStringList();
    }

  if (!hasStructuredChildren)
    {
      // Legacy headers stored a whitespace-separated list. A one-slab
      // manifest is unambiguous even when the filename itself contains
      // spaces, so keep the complete value in that case. Multi-slab legacy
      // headers retain the historical tokenised representation; callers
      // should use structured <name>/<file> entries for multiple names with
      // spaces.
      const QString legacy = container.text().trimmed();
      if (!legacy.isEmpty())
        {
          if (expectedCount == 1)
            names << legacy;
          else
            names = legacy.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        }
    }
  return names;
}

int
PvlManifestParser::bytesPerVoxel(int voxelType)
{
  if (voxelType == UnsignedChar || voxelType == Char)
    return 1;
  if (voxelType == UnsignedShort || voxelType == Short)
    return 2;
  if (voxelType == Int || voxelType == Float)
    return 4;
  return 0;
}

QStringList
PvlManifestParser::deriveNames(const QString& headerFile,
                               int depth,
                               int slabSize)
{
  QStringList names;
  if (depth <= 0 || slabSize <= 0)
    return names;

  const int count = 1 + (depth-1)/slabSize;
  for (int i = 0; i < count; ++i)
    names << QString("%1.%2").arg(headerFile).arg(i+1, 3, 10, QChar('0'));
  return names;
}

bool
PvlManifestParser::validateSlab(const PvlManifest& manifest,
                                int index,
                                const QString& filename,
                                bool raw)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
    return false;

  const int firstSlice = index*manifest.slabSize;
  const int slices = qMin(manifest.slabSize, manifest.depth-firstSlice);
  if (slices <= 0)
    return false;

  const int headerSize = raw ? manifest.rawHeaderSize : manifest.headerSize;
  const int voxelType = raw ? manifest.rawVoxelType : manifest.voxelType;
  const qint64 payload = static_cast<qint64>(slices) * manifest.width *
    manifest.height * bytesPerVoxel(voxelType);
  const qint64 expected = headerSize + payload;
  if (file.size() != expected)
    return false;

  if (headerSize >= 13)
    {
      QByteArray header = file.read(13);
      if (header.size() != 13)
        return false;
      const unsigned char type = static_cast<unsigned char>(header.at(0));
      qint32 fileSlices = 0;
      qint32 fileWidth = 0;
      qint32 fileHeight = 0;
      std::memcpy(&fileSlices, header.constData()+1, sizeof(fileSlices));
      std::memcpy(&fileWidth, header.constData()+5, sizeof(fileWidth));
      std::memcpy(&fileHeight, header.constData()+9, sizeof(fileHeight));
      if (type != static_cast<unsigned char>(voxelType == Float ? 8 : voxelType) ||
          fileSlices != slices || fileWidth != manifest.width ||
          fileHeight != manifest.height)
        return false;
    }
  return true;
}

bool
PvlManifestParser::validateColorChannel(const PvlManifest& manifest,
                                        const QString& baseFilename)
{
  PvlManifest channel = manifest;
  channel.voxelType = UnsignedChar;
  channel.headerSize = 13;
  const QStringList slabs = deriveNames(baseFilename, manifest.depth,
                                        manifest.slabSize);
  if (slabs.count() != 1 + (manifest.depth-1)/manifest.slabSize)
    return false;
  for (int i = 0; i < slabs.count(); ++i)
    if (!validateSlab(channel, i, slabs.at(i), false))
      return false;
  return true;
}

bool
PvlManifestParser::parse(const QString& filename,
                         PvlManifest& manifest,
                         bool validateFiles)
{
  manifest = PvlManifest();
  manifest.headerFile = QFileInfo(filename).absoluteFilePath();

  QDomDocument document;
  QString error;
  if (!readDocument(manifest.headerFile, document, error))
    return fail(manifest, error);

  const QDomElement root = document.documentElement();
  if (root.isNull() || root.tagName() != "PvlDotNcFileHeader")
    return fail(manifest, "PVL header has an invalid root element");

  const QSet<QString> allowedFields = QSet<QString>()
    << "rawfile" << "voxeltype" << "pvlvoxeltype" << "gridsize"
    << "voxelunit" << "voxelsize" << "description" << "slabsize"
    << "rawmap" << "pvlmap" << "rawnames" << "pvlnames"
    << "pvlheadersize" << "rawheadersize" << "channelnames"
    << "sourceorder";
  for (QDomElement child = root.firstChildElement(); !child.isNull();
       child = child.nextSiblingElement())
    if (!allowedFields.contains(child.tagName()))
      return fail(manifest, QString("PVL header contains unknown <%1>")
                  .arg(child.tagName()));

  if (!readTriple(root, "gridsize", manifest.depth, manifest.width,
                  manifest.height, error) ||
      !readRequiredInt(root, "slabsize", manifest.slabSize, error))
    return fail(manifest, error);

  const auto readOptionalText = [&root](const QString& tag,
                                        QString& value,
                                        QString& textError) {
    const QDomElement element = root.firstChildElement(tag);
    if (element.isNull())
      return true;
    if (!element.nextSiblingElement(tag).isNull() ||
        !element.firstChildElement().isNull())
      {
        textError = QString("PVL <%1> must contain one text-only element")
          .arg(tag);
        return false;
      }
    value = element.text();
    return true;
  };
  if (!readOptionalText("rawfile", manifest.rawFile, error) ||
      !readOptionalText("voxelunit", manifest.voxelUnit, error) ||
      !readOptionalText("description", manifest.description, error))
    return fail(manifest, error);
  manifest.rawFile = manifest.rawFile.trimmed();
  manifest.voxelUnit = manifest.voxelUnit.trimmed();

  QString voxelSizeText;
  if (!readOptionalText("voxelsize", voxelSizeText, error))
    return fail(manifest, error);
  if (!voxelSizeText.trimmed().isEmpty())
    {
      const QStringList values = voxelSizeText.split(
        QRegExp("\\s+"), QString::SkipEmptyParts);
      if (values.count() != 3)
        return fail(manifest, "PVL <voxelsize> must contain exactly three values");
      bool ok0 = false, ok1 = false, ok2 = false;
      const float x = values.at(0).toFloat(&ok0);
      const float y = values.at(1).toFloat(&ok1);
      const float z = values.at(2).toFloat(&ok2);
      if (!ok0 || !ok1 || !ok2 || x <= 0 || y <= 0 || z <= 0)
        return fail(manifest, "PVL <voxelsize> contains invalid values");
      manifest.voxelSizeX = x;
      manifest.voxelSizeY = y;
      manifest.voxelSizeZ = z;
    }

  const auto readFloatList = [&root](const QString& tag,
                                     QList<float>& values,
                                     QString& listError) {
    values.clear();
    const QDomElement element = root.firstChildElement(tag);
    if (element.isNull()) return true;
    if (!element.nextSiblingElement(tag).isNull() ||
        !element.firstChildElement().isNull())
      {
        listError = QString("PVL <%1> must contain one text-only element")
          .arg(tag);
        return false;
      }
    const QStringList tokens = element.text().split(
      QRegExp("\\s+"), QString::SkipEmptyParts);
    for (int i = 0; i < tokens.count(); ++i)
      {
        bool ok = false;
        const float value = tokens.at(i).toFloat(&ok);
        if (!ok || !std::isfinite(value))
          {
            listError = QString("PVL <%1> contains an invalid number")
              .arg(tag);
            return false;
          }
        values << value;
      }
    return true;
  };
  const auto readIntList = [&root](const QString& tag,
                                   QList<int>& values,
                                   QString& listError) {
    const QDomElement element = root.firstChildElement(tag);
    if (element.isNull()) return true;
    if (!element.nextSiblingElement(tag).isNull() ||
        !element.firstChildElement().isNull())
      {
        listError = QString("PVL <%1> must contain one text-only element")
          .arg(tag);
        return false;
      }
    const QStringList tokens = element.text().split(
      QRegExp("\\s+"), QString::SkipEmptyParts);
    for (int i = 0; i < tokens.count(); ++i)
      {
        bool ok = false;
        const int value = tokens.at(i).toInt(&ok);
        if (!ok)
          {
            listError = QString("PVL <%1> contains an invalid integer")
              .arg(tag);
            return false;
          }
        values << value;
      }
    return true;
  };
  if (!readFloatList("rawmap", manifest.rawMap, error) ||
      !readIntList("pvlmap", manifest.pvlMap, error))
    return fail(manifest, error);
  if (!manifest.rawMap.isEmpty() && !manifest.pvlMap.isEmpty() &&
      manifest.rawMap.count() != manifest.pvlMap.count())
    return fail(manifest, "PVL <rawmap> and <pvlmap> must have equal lengths");

  QDomElement pvlVoxelTypeElement = root.firstChildElement("pvlvoxeltype");
  QDomElement voxelTypeElement = root.firstChildElement("voxeltype");
  if (!pvlVoxelTypeElement.isNull())
    {
      if (!readVoxelType(root, "pvlvoxeltype", manifest.voxelType, error))
        return fail(manifest, error);
    }
  else if (!voxelTypeElement.isNull())
    {
      // Legacy headers documented by Drishti used <voxeltype> for the PVL
      // payload and omitted <pvlvoxeltype>.
      if (!readVoxelType(root, "voxeltype", manifest.voxelType, error))
        return fail(manifest, error);
    }
  else
    return fail(manifest,
                "PVL header must contain <pvlvoxeltype> or legacy <voxeltype>");

  if (!voxelTypeElement.isNull())
    {
      if (!readVoxelType(root, "voxeltype", manifest.rawVoxelType, error))
        return fail(manifest, error);
    }
  else
    manifest.rawVoxelType = manifest.voxelType;

  manifest.isColor = manifest.voxelType == RGB || manifest.voxelType == RGBA;
  // A colour manifest describes one logical channel set.  The legacy
  // <voxeltype> field is the RAW type, while <pvlvoxeltype> is the PVL type;
  // accepting RGB with RGBA (or colour with scalar) here would make readers
  // select a channel count different from the writer's output.
  if ((manifest.isColor || manifest.rawVoxelType == RGB ||
       manifest.rawVoxelType == RGBA) &&
      manifest.rawVoxelType != manifest.voxelType)
    return fail(manifest, "RGB/RGBA manifest has inconsistent voxel types");

  QDomElement pvlHeaderElement = root.firstChildElement("pvlheadersize");
  if (!pvlHeaderElement.isNull() &&
      !pvlHeaderElement.nextSiblingElement("pvlheadersize").isNull())
    return fail(manifest, "PVL header must contain at most one <pvlheadersize>");
  if (!pvlHeaderElement.isNull())
    {
      bool ok = false;
      manifest.headerSize = pvlHeaderElement.text().trimmed().toInt(&ok);
      if (!ok || manifest.headerSize < 0)
        return fail(manifest, "PVL <pvlheadersize> is invalid");
    }

  QDomElement rawHeaderElement = root.firstChildElement("rawheadersize");
  if (!rawHeaderElement.isNull() &&
      !rawHeaderElement.nextSiblingElement("rawheadersize").isNull())
    return fail(manifest, "PVL header must contain at most one <rawheadersize>");
  if (!rawHeaderElement.isNull())
    {
      bool ok = false;
      manifest.rawHeaderSize = rawHeaderElement.text().trimmed().toInt(&ok);
      if (!ok || manifest.rawHeaderSize < 0)
        return fail(manifest, "PVL <rawheadersize> is invalid");
    }

  const int expectedCount = 1 + (manifest.depth-1)/manifest.slabSize;
  manifest.pvlNames = readNames(root, "pvlnames", error, expectedCount);
  if (!error.isEmpty())
    return fail(manifest, error);
  manifest.rawNames = readNames(root, "rawnames", error, expectedCount);
  if (!error.isEmpty())
    return fail(manifest, error);
  manifest.sourceOrder = readNames(root, "sourceorder", error);
  if (!error.isEmpty())
    return fail(manifest, error);

  const QFileInfo headerInfo(manifest.headerFile);
  const QDir directory = headerInfo.absoluteDir();
  auto resolveNames = [&directory](const QStringList& relative) {
    QStringList result;
    for (int i = 0; i < relative.count(); ++i)
      result << QFileInfo(directory, relative.at(i)).absoluteFilePath();
    return result;
  };

  if (manifest.isColor)
    {
      if (!manifest.pvlNames.isEmpty() || !manifest.rawNames.isEmpty() ||
          (!manifest.rawFile.isEmpty() && manifest.rawFile != "dontask"))
        return fail(manifest,
                    "RGB/RGBA manifest must use <channelnames> without scalar PVL/RAW names");

      QDomElement channels = root.firstChildElement("channelnames");
      if (!channels.isNull() && !channels.nextSiblingElement("channelnames").isNull())
        return fail(manifest, "PVL header must contain at most one <channelnames>");
      if (!channels.isNull())
        {
          manifest.channelNames = readNames(root, "channelnames", error);
          if (!error.isEmpty())
            return fail(manifest, error);
        }
      const int expectedChannels = manifest.voxelType == RGBA ? 4 : 3;
      if (manifest.channelNames.isEmpty())
        {
          QString base = manifest.headerFile;
          if (base.endsWith(".pvl.nc"))
            base.chop(6);
          else
            return fail(manifest, "RGB/RGBA header must use a .pvl.nc filename");
          const QStringList defaults = QStringList()
            << base + "red" << base + "green" << base + "blue";
          manifest.channelNames = defaults;
          if (expectedChannels == 4)
            manifest.channelNames << base + "alpha";
        }
      else
        manifest.channelNames = resolveNames(manifest.channelNames);
      if (manifest.channelNames.count() != expectedChannels)
        return fail(manifest, QString("RGB manifest lists %1 channels, expected %2")
                    .arg(manifest.channelNames.count()).arg(expectedChannels));
      QSet<QString> uniqueChannels;
      for (int i = 0; i < manifest.channelNames.count(); ++i)
        {
          const QString normalized = QFileInfo(manifest.channelNames.at(i))
            .canonicalFilePath();
          const QString key = normalized.isEmpty() ?
            QFileInfo(manifest.channelNames.at(i)).absoluteFilePath() : normalized;
          if (uniqueChannels.contains(key))
            return fail(manifest, QString("duplicate RGB channel '%1'")
                        .arg(manifest.channelNames.at(i)));
          uniqueChannels.insert(key);
        }
      if (validateFiles)
        for (int i = 0; i < manifest.channelNames.count(); ++i)
          if (!validateColorChannel(manifest, manifest.channelNames.at(i)))
            return fail(manifest, QString("RGB channel %1 is missing, malformed, or has an unexpected size")
                        .arg(manifest.channelNames.at(i)));
      manifest.valid = true;
      return true;
    }

  if (!root.firstChildElement("channelnames").isNull())
    return fail(manifest,
                "scalar PVL manifest must not contain <channelnames>");

  if (manifest.pvlNames.isEmpty())
    manifest.pvlNames = deriveNames(manifest.headerFile, manifest.depth,
                                    manifest.slabSize);
  else
    manifest.pvlNames = resolveNames(manifest.pvlNames);
  if (!manifest.rawNames.isEmpty())
    manifest.rawNames = resolveNames(manifest.rawNames);

  if (manifest.rawNames.isEmpty() && !manifest.rawFile.isEmpty() &&
      manifest.rawFile != "dontask")
    {
      const QString firstRaw = QFileInfo(directory, manifest.rawFile)
        .absoluteFilePath();
      QRegExp numbered("^(.*)\\.([0-9]{3})$");
      if (numbered.exactMatch(firstRaw))
        {
          const QString base = numbered.cap(1);
          for (int i = 0; i < expectedCount; ++i)
            manifest.rawNames << QString("%1.%2")
              .arg(base).arg(i+1, 3, 10, QChar('0'));
        }
      else if (expectedCount == 1)
        manifest.rawNames << firstRaw;
      else
        return fail(manifest,
                    "rawfile does not identify all RAW slab files; add <rawnames>");
    }
  if (!manifest.rawNames.isEmpty() && manifest.rawNames.count() != expectedCount)
    return fail(manifest, QString("RAW manifest lists %1 slabs, expected %2")
                .arg(manifest.rawNames.count()).arg(expectedCount));

  const auto validateNames = [&manifest](const QStringList& names,
                                         const QString& label) {
    QSet<QString> unique;
    for (int i = 0; i < names.count(); ++i)
    {
      if (names.at(i).trimmed().isEmpty())
        return QString("empty %1 slab name").arg(label);
      const QString normalized = QFileInfo(names.at(i)).canonicalFilePath();
      const QString key = normalized.isEmpty() ?
        QFileInfo(names.at(i)).absoluteFilePath() : normalized;
      if (unique.contains(key))
        return QString("duplicate %1 slab '%2'").arg(label).arg(names.at(i));
      unique.insert(key);
    }
    return QString();
  };

  const QString pvlNameError = validateNames(manifest.pvlNames, "PVL");
  if (!pvlNameError.isEmpty())
    return fail(manifest, pvlNameError);
  const QString rawNameError = validateNames(manifest.rawNames, "RAW");
  if (!rawNameError.isEmpty())
    return fail(manifest, rawNameError);

  if (manifest.pvlNames.count() != expectedCount)
    return fail(manifest, QString("PVL manifest lists %1 slabs, expected %2")
                .arg(manifest.pvlNames.count()).arg(expectedCount));

  if (validateFiles)
    for (int i = 0; i < manifest.pvlNames.count(); ++i)
      if (!validateSlab(manifest, i, manifest.pvlNames.at(i), false))
        return fail(manifest, QString("PVL slab %1 is missing, malformed, or has an unexpected size")
                    .arg(manifest.pvlNames.at(i)));

  if (validateFiles)
    for (int i = 0; i < manifest.rawNames.count(); ++i)
      if (!validateSlab(manifest, i, manifest.rawNames.at(i), true))
        return fail(manifest, QString("RAW slab %1 is missing, malformed, or has an unexpected size")
                    .arg(manifest.rawNames.at(i)));

  manifest.valid = true;
  return true;
}
