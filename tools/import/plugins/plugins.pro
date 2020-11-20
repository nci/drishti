TEMPLATE = subdirs

SUBDIRS = analyze \
	  grd \
	  imagestack \
	  dicom \
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
