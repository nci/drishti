TEMPLATE = subdirs

SUBDIRS = analyze \
	  grd \
	  imagestack \
	  dicom \
	  metaimage \
	  nc \
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
