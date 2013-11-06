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
          vgi

win32 {
    SUBDIRS += nifti \
               nrrd
}
