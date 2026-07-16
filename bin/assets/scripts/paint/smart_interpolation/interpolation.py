import paintmod
import numpy as np
import cv2
import sys
from biomedisa.features.biomedisa_helper import load_data, save_data
from biomedisa.interpolation import smart_interpolation
import traceback

class paint_data :
    def __init__(self) :
        self.paint_obj = 0
        self.volume = 0
        self.mask = 0
        self.lut = 0
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = np.zeros(3,np.int32)
        self.arguments = 0

print('paint_data declared')
pd = paint_data()


def set_paint_data(py_obj) :
    pd.paint_obj = py_obj
    pd.volume = py_obj.get_volume_view()
    pd.mask = py_obj.get_mask_view()
    pd.lut = py_obj.get_lut_view()
    pd.depth = py_obj.depth
    pd.width = py_obj.width
    pd.height = py_obj.height
    pd.dim[0] = pd.depth
    pd.dim[1] = pd.width
    pd.dim[2] = pd.height
    print(pd.depth*pd.width*pd.height)
    print(pd.depth, pd.width, pd.height)
    print(pd.volume.shape)
    
def init() :
    print('init biomedisa smart interpolation')
    pd.volume = pd.volume.reshape(pd.dim)
    pd.mask = pd.mask.reshape(pd.dim)
    print(pd.volume.shape, pd.volume.dtype)
    print(pd.mask.shape, pd.mask.dtype)


#def process_slice(img, mask, width, height, tag) :
#    print('slice processing not implemented')
    
    
def process_volume() :
    print('volume processing using biomedisa smart interpolation')
    try :
        print(pd.paint_obj.script_args)
        allaxis = pd.paint_obj.script_args["allaxis"]
        smooth = pd.paint_obj.script_args["smooth"]
        nbrw = pd.paint_obj.script_args["nbrw"]
        sorw = pd.paint_obj.script_args["sorw"]
        print(nbrw, sorw)
        results = smart_interpolation(pd.volume, pd.mask,
                                      nbrw=nbrw,
                                      sorw=sorw,
                                      allaxis=allaxis,
                                      smooth=smooth)
        if 'smooth' in results : # opencl does not have smooth option yet
            pd.mask[:] = results['smooth'].astype(np.uint16)
        else :
            pd.mask[:] = results['regular'].astype(np.uint16)
        pd.paint_obj.update_3d_view()
        pd.paint_obj.update_slice_view()
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()

    
if __name__ == '__main__':
    print('biomedisa.interpolation')
