import paintmod
import numpy as np
import cv2
import sys

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
    print('init grabcut')
    pd.volume = pd.volume.reshape(pd.dim)  

    
def process_volume() :
    print('volume processing not implemented')
    #pd.paint_obj.update_3d_view()
    

def process_slice(img, mask, width, height, tag) :
    mask = mask.reshape((width, height))
    t_img = img.reshape(width,height)
    image = cv2.cvtColor(t_img, cv2.COLOR_GRAY2BGR)
    bgdModel = np.zeros((1, 65), np.float64) # Background model
    fgdModel = np.zeros((1, 65), np.float64) # Foreground model
    rect = (0,0,height-1,width-1)
    mask2 = mask.astype(np.uint8)
    cv2.grabCut(image, mask2, rect, bgdModel, fgdModel, 1, cv2.GC_INIT_WITH_RECT)
    mask2 = np.where((mask2==1) + (mask2==3),tag,0).astype('uint16')
    mask[:] = mask2
    mask3 = np.where((mask2==1) + (mask2==3),255,0).astype('uint8')
    image = cv2.cvtColor(mask3, cv2.COLOR_GRAY2BGR)

    
if __name__ == '__main__':
    print('cv2.grabCut')
    
