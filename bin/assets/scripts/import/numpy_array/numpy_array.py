import os
import sys
import numpy

print('---- numpy_array_reader.py ----')

class Volume :
    def __init__(self) :
        self.flnms = ""
        self.description = "NumpyArray volume"
        self.voxelUnit = "micron"
        self.voxelSize = (1.0, 1.0, 1.0)
        self.voxelType = 0
        self.bytesPerVoxel = 1
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = (0, 0, 0)
        self.headerBytes = 0
        self.data = None
        self.dataMin = 0
        self.dataMax = 0
        self.rawMin = 0
        self.rawMax = 0
        self.histogram = None
        print('\nNumpyArray Reader Initialized\n')
    #--------------------


    #--------------------
    def setFiles(self, flnms) :
        self.Filenames = flnms
            
        flnm = self.Filenames[0]
    
        self.headerBytes = 0       

        print('read numpy array from file: ', flnm)
        sys.stdout.flush()
        self.data = numpy.load(flnm, mmap_mode='r')
        print('data shape: ', self.data.shape)
        sys.stdout.flush()

        self.voxelType = 0
        if self.data[0,0,0].dtype == numpy.dtype('B') :
            self.voxelType = 0 # UCHAR
        if self.data[0,0,0].dtype == numpy.dtype('b') :
            self.voxelType = 1 # CHAR
        if self.data[0,0,0].dtype == numpy.dtype('u2') :
            self.voxelType = 2 # USHORT
        if self.data[0,0,0].dtype == numpy.dtype('i2') :
            self.voxelType = 3 # SHORT
        if self.data[0,0,0].dtype == numpy.dtype('i4') :
            self.voxelType = 4 # INT
        if self.data[0,0,0].dtype == numpy.dtype('f4') :
            self.voxelType = 5 # FLOAT

        (self.depth, self.width, self.height) = self.data.shape
        self.dim = (self.height, self.width, self.depth)    
    #--------------------

        
    #--------------------
    def calculate_min_max(self) :
        self.dataMin = numpy.min(self.data)
        self.dataMax = numpy.max(self.data)
        self.rawMin = self.dataMin
        self.rawMax = self.dataMax
    #--------------------


    #--------------------
    def gen_histogram(self):
        if self.voxelType < 2 :
            self.histogram, b = numpy.histogram(self.data, bins=256)
        else :
            self.histogram, b = numpy.histogram(self.data, bins=65536)
        self.histogram.astype(numpy.uint32)
    #--------------------

    
    #--------------------
    def get_depth_slice(self, d):
        depth_slice = self.data[d, :]
        return depth_slice
    #--------------------

    
    #--------------------
    def get_rawvalue(self, d, w, h):
        val = self.data[d, w, h]
        return val
    #--------------------
#------------------------------------------------------------------
#------------------------------------------------------------------


vol = Volume()

def init() :
    print('\nInit from numpy_array.py\n')
    sys.stdout.flush()

def set_files(flnms) :
    print(flnms)
    sys.stdout.flush()
    vol.setFiles(flnms)    
    vol.calculate_min_max()
    vol.gen_histogram()

def get_description() :
    return vol.description

def get_voxel_unit() :
    return vol.voxelUnit

def get_voxel_size() :
    return vol.voxelSize

def get_voxel_type() :
    return vol.voxelType

def get_header_bytes() :
    return vol.headerBytes

def get_grid_size() :
    return vol.dim

def get_raw_min_max() :
    return (vol.rawMin, vol.rawMax)

def get_histogram(hist : numpy.ndarray) :
    ln = len(vol.histogram)
    hist[:ln] = vol.histogram[:]

def get_depth_slice(slc, slice : numpy.ndarray) :
    depth_slice = vol.get_depth_slice(slc)
    slice[:] = depth_slice.flatten()[:] # flatten to 1D array

def get_rawvalue(d, w, h) :
    val = vol.get_rawvalue(d, w, h).item() # convert numpy scalar to Python scalar
    return val
