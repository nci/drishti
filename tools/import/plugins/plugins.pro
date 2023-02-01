TEMPLATE = subdirs

SUBDIRS = analyze \
          dicom \
          grd \
	  imagestack \
	  metaimage \
	  raw \
	  rawslabs \
	  rawslices \
	  tiff \
	  tom \
	  txm \
          vgi
          
win32 {
SUBDIRS += nc4 \
           nifti \
           nrrd
}

# have not checked netCDF4 on linux systems so build netCDF3 plugin
unix {
SUBDIRS += nc
}
