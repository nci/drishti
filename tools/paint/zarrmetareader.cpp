#include <string>
#include <vector>

#include <QInputDialog>
#include <QMessageBox>

#include <libzarr/libzarr.hpp>
#include <libzarr/adapters/filesystem_store.hpp>

#include "zarrmetareader.h"

ZarrVolumeInfo ZarrMetaReader::zarrInfo;

ZarrVolumeInfo
ZarrMetaReader::getInfo(const QString& zarrDir)
{
  zarrInfo.m_levels.clear();
  
  std::shared_ptr<zarr::FilesystemStore> store;
  std::shared_ptr<zarr::Group> root;
  try
    {
      store = std::make_shared<zarr::FilesystemStore>(
                zarrDir.toStdString(), false);
      root = std::make_shared<zarr::Group>(
                zarr::Group::open(store, "", zarr::OpenOptions{}));
    }
  catch (const std::exception&)
    {
      return zarrInfo;
    }
  
  const zarr::json& attr = root->attributes();

  // drishti / mango attributes (voxel_unit / voxel_size_xyz / description)
  zarr::json meta = attr.contains("drishti")
                    ? attr.at("drishti") : zarr::json::object();
  if (meta.empty() && attr.contains("mango"))
    meta = attr.at("mango");

  if (meta.contains("description") && meta.at("description").is_string())
    zarrInfo.description = QString::fromStdString(
			   meta.at("description").get<std::string>());

  if (meta.contains("voxel_unit") && meta.at("voxel_unit").is_string())
    {
      QString vu = QString::fromStdString(
                     meta.at("voxel_unit").get<std::string>()).toLower();
      if (vu == "mm" || vu == "millimeter") zarrInfo.voxelUnit = 4;
      else if (vu == "micron" || vu == "um" || vu == "mu") zarrInfo.voxelUnit = 3;
      else if (vu == "angstrom") zarrInfo.voxelUnit = 1;
      else if (vu == "nanometer") zarrInfo.voxelUnit = 2;
      else if (vu == "centimeter") zarrInfo.voxelUnit = 5;
      else if (vu == "meter") zarrInfo.voxelUnit = 6;
    }

  if (meta.contains("voxel_size_xyz") && meta.at("voxel_size_xyz").is_array())
    {
      const zarr::json& vsz = meta.at("voxel_size_xyz");
      if (vsz.size() >= 3)
        {
          zarrInfo.voxelSizeX = (float)vsz[0].get<double>();
          zarrInfo.voxelSizeY = (float)vsz[1].get<double>();
          zarrInfo.voxelSizeZ = (float)vsz[2].get<double>();
        }
    }
  
  // harvest all pyramid level paths from multiscales[0].datasets[].path
  // (unconditional), and use the level-0 scale transform only as a fallback
  // for the voxel size if it was not written in the drishti attributes.
  if (attr.contains("multiscales") && attr.at("multiscales").is_array())
    {
      const zarr::json& ms = attr.at("multiscales");
      if (ms.size() > 0 && ms[0].contains("datasets") &&
          ms[0]["datasets"].is_array())
        {
          const zarr::json& datasets = ms[0]["datasets"];
          for (const zarr::json& d : datasets)
            {
              if (!d.is_object() || !d.contains("path"))
                continue;
              std::string p = d.at("path").get<std::string>();
              if (p.empty())
                continue;
              QString pq = QString::fromStdString(p);
              zarrInfo.m_levels.append(pq);

              // record this level's scale transform
              QVector<float> sc(3, 1.0f);
              if (d.contains("coordinateTransformations") &&
                  d["coordinateTransformations"].is_array())
                {
                  const zarr::json& ct = d["coordinateTransformations"];
                  if (ct.size() > 0 && ct[0].is_object() &&
                      ct[0].contains("scale") && ct[0]["scale"].is_array() &&
                      ct[0]["scale"].size() >= 3)
                    {
                      sc[0] = (float)ct[0]["scale"][0].get<double>();
                      sc[1] = (float)ct[0]["scale"][1].get<double>();
                      sc[2] = (float)ct[0]["scale"][2].get<double>();
                    }
                }
              zarrInfo.m_levelScales[pq] = sc;
            }

          // fallback voxel size from the level-0 dataset scale transform
          const bool needFallback =
            zarrInfo.voxelSizeX == 1.0f && zarrInfo.voxelSizeY == 1.0f &&
            zarrInfo.voxelSizeZ == 1.0f;
          if (needFallback && datasets.size() > 0)
            {
              const zarr::json& d = datasets[0];
              if (d.contains("coordinateTransformations") &&
                  d["coordinateTransformations"].is_array())
                {
                  const zarr::json& ct = d["coordinateTransformations"];
                  if (ct.size() > 0 && ct[0].is_object() &&
                      ct[0].contains("scale") && ct[0]["scale"].is_array() &&
                      ct[0]["scale"].size() >= 3)
                    {
                      zarrInfo.voxelSizeX = (float)ct[0]["scale"][0].get<double>();
                      zarrInfo.voxelSizeY = (float)ct[0]["scale"][1].get<double>();
                      zarrInfo.voxelSizeZ = (float)ct[0]["scale"][2].get<double>();
                    }
                }
            }
        }
    }

  if (zarrInfo.m_levels.isEmpty())
    {
      zarrInfo.valid = false;
      return zarrInfo;
    }
  if (zarrInfo.m_levels.size() == 1)
    {
      zarrInfo.level = zarrInfo.m_levels[0];
    }
  else
    {
      bool ok;
      QString lv = QInputDialog::getItem(0,
					 "Choose a pyramid level",
					 "Levels",
					 zarrInfo.m_levels,
					 0,
					 false,
					 &ok);
      zarrInfo.level = ok ? lv : zarrInfo.m_levels[0];
    }

  // apply the selected level's scale transform to the base (level-0) voxel
  // size: higher pyramid levels are downsampled, so their voxels are larger.
  {
    QVector<float> sc = zarrInfo.m_levelScales.value(zarrInfo.level);
    if (sc.size() >= 3)
      {
        if (sc[0] > 0) zarrInfo.voxelSizeX *= sc[0];
        if (sc[1] > 0) zarrInfo.voxelSizeY *= sc[1];
        if (sc[2] > 0) zarrInfo.voxelSizeZ *= sc[2];
      }
  }

  // selected level array: shape → dimensions, dtype → voxel type
  try
    {
      zarr::Array arr = root->open_array(zarrInfo.level.toStdString());
      const zarr::ArrayMeta& meta = arr.meta();
      const std::vector<std::uint64_t>& shape = meta.shape;
      if (shape.size() >= 3)
        {
          zarrInfo.depth    = (int)shape[0];
          zarrInfo.width    = (int)shape[1];
          zarrInfo.height   = (int)shape[2];
          zarrInfo.voxelType =
            (meta.dtype.kind == zarr::DType::uint16) ? 2 : 0;
          zarrInfo.valid = true;
        }
    }
  catch (const std::exception&)
    {
    }

  return zarrInfo;
}
