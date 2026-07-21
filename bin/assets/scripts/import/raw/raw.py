import os
import sys
import numpy

print('---- raw_reader.py ----')

class Volume :
    def __init__(self) :
        self.flnms = ""
        self.description = "RAW volume"
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
        print('\nvol Initialized\n')
    #--------------------

    #--------------------
    def setFiles(self, flnms) :
        self.flnms = flnms
        # open the first file and read the header
        fin = open(self.flnms[0], 'rb')    
        (self.voxelType) = numpy.fromfile(fin, dtype=numpy.int8, count=1)[0]
        (self.depth, self.width, self.height) = numpy.fromfile(fin, dtype=numpy.int32, count=3)
        self.dim = (self.height, self.width, self.depth)
        self.headerBytes = 13
        fin.close()
    #--------------------

    #--------------------
    def load_entire_data(self) :
        flnm = self.flnms[0]
        fin = open(flnm, 'rb')

        # skip the header bytes
        fin.seek(self.headerBytes)
        
        self.data=0
        
        if self.voxelType == 0 : # UCHAR
            self.bytesPerVoxel = 1
            self.data = numpy.fromfile(fin, dtype=numpy.uint8, count=self.depth*self.width*self.height)
            self.rawMin = 0
            self.rawMax = 255
            
        if self.voxelType == 2 : # USHORT
            self.bytesPerVoxel = 2
            self.data = numpy.fromfile(fin, dtype=numpy.uint16, count=self.depth*self.width*self.height)
            self.rawMin = 0
            self.rawMax = 65535            

        fin.close()
        print('data in memory')
    #--------------------

        
    #--------------------
    def calculate_min_max(self) :
        self.dataMin = numpy.min(self.data)
        self.dataMax = numpy.max(self.data)
    #--------------------


    #--------------------
    def gen_histogram(self):
        bins = list(range(self.rawMin, self.rawMax+2))
        self.histogram, b = numpy.histogram(self.data, bins,
                                            range=(self.dataMin, self.dataMax))
        self.histogram.astype(numpy.int64)
    #--------------------

    #--------------------
    def get_depth_slice(self, d):
        slice_size = self.width * self.height
        dstart = d * slice_size
        depth_slice = self.data[dstart:dstart+slice_size]                  
        return depth_slice
    #--------------------

    #--------------------
    def get_rawvalue(self, d, w, h):
        pos = d*self.width*self.height + w*self.height + h
        val = self.data[pos:pos+1]
        return val
    #--------------------
#------------------------------------------------------------------
#------------------------------------------------------------------


vol = Volume()

def init() :
    print('\nInit from raw.py\n')

def set_files(flnms) :
    print(flnms)
    vol.setFiles(flnms)    
    vol.load_entire_data()
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
    slice[:] = depth_slice[:]

def get_rawvalue(d, w, h) :
    val = vol.get_rawvalue(d, w, h)
    return val[0]
