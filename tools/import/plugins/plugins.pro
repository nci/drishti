TEMPLATE = subdirs

SUBDIRS = analyze \
          dicom \
          grd \
	  imagestack \
	  metaimage \
          nc4 \
          raw \
	  rawslabs \
	  rawslices \
	  tiff \
	  tom \
	  txm \
          vgi
          
win32 {
SUBDIRS += nifti \
           nrrd
}
