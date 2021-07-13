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
SUBDIRS += nifti \
           nc \
           nrrd
}
