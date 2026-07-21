from pywidget import widget
import os
import sys
import numpy
import zarr
import json
import traceback
import inspect


print('---- zarr_folder_reader.py ----')

class Volume :
    def __init__(self) :
        self.flnms = ""
        self.description = "Zarr volume"
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
        print('\nZarr Reader Initialized\n')
    #--------------------


    #--------------------
    def loadJson(self, jsonflnm, sslevel) :
        fl = open(jsonflnm, 'r')
        JSON = json.load(fl)
        MANGO = JSON['attributes']['mango']
        self.dataMin, self.dataMax = MANGO['data_min_max']
        self.voxelSize = MANGO['voxel_size_xyz']
        vu = MANGO['voxel_unit']
        if (vu == "mu") :
            self.voxelUnit = "micron"
        if (vu == "mm") :
            self.voxelUnit = "millimeter"

        ms = JSON['attributes']['multiscales']
        datasets = ms[0]['datasets']
        for d in datasets :
            if d['path'] == sslevel :
                ct = d['coordinateTransformations']
                scale = ct[0]['scale'] 
                print(self.voxelSize)
                print(scale)
                vx, vy, vz = self.voxelSize
                sx, sy, sz = scale
                self.voxelSize = [vx*sx, vy*sy, vz*sz]
                break
                

            
    def getSubsamplingLevel(self, sslevels) :
        print(sslevels)
        try :
            sslevel = widget.get_item("Load Subsampling Level",
                                      "Subsampling Level",
                                      sslevels,
                                      0)
        except Exception as e :
            print('Error : ', str(e))
            print('Full Error : ', repr(e))
            traceback.print_exc()

        return sslevel
        
    def setFiles(self, flnms) :
        self.Filenames = flnms
            
        flnm = self.Filenames[0]

        self.headerBytes = 0       

        print('read zarr data from file: ', flnm)
        try :
            z = zarr.open(flnm)
            print(z.tree())
            print(z.info_complete())
        except Exception as e :
            print('Error : ', str(e))
            print('Full Error : ', repr(e))
            traceback.print_exc()

        sslevels = []
        for a in z.arrays() :
            sslevels.append(a[0])
        sslevels.sort()
        sslevel = self.getSubsamplingLevel(sslevels)

        try :
            jsonflnm = flnm + "/zarr.json"
            self.loadJson(jsonflnm, sslevel)
        except Exception as e :
            print('Error : ', str(e))
            print('Full Error : ', repr(e))
            traceback.print_exc()

        A = 0
        for a in z.arrays() :
            if a[0] == sslevel :
                self.data = a[1]
        print('data shape: ', self.data.shape)

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
        #self.dataMin = numpy.min(self.data)
        #self.dataMax = numpy.max(self.data)
        # min and max values taken from json file
        self.rawMin = self.dataMin
        self.rawMax = self.dataMax
    #--------------------


    #--------------------
    def gen_histogram(self):
        if self.voxelType < 2 :
            self.histogram, b = numpy.histogram(self.data, bins=256,
                                                range=(self.dataMin, self.dataMax))
        else :
            self.histogram, b = numpy.histogram(self.data, bins=65536,
                                                range=(self.dataMin, self.dataMax))
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
    print('\nInit from load_zarr.py\n')

def set_files(flnms) :
    print(flnms)
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
