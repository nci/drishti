TEMPLATE = subdirs
SUBDIRS = binarythinning \
	  connectedcomponent \
	  distancemap \
	  edgepreserving \
	  smoothing

isEmpty(DRISHTI_ENABLE_VED): DRISHTI_ENABLE_VED = $$(DRISHTI_ENABLE_VED)
equals(DRISHTI_ENABLE_VED, 1): SUBDIRS += ved
