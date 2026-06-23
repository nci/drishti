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
        self.dim = np.zeros(3, np.int32)

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
    print('init watershed')
    pd.volume = pd.volume.reshape(pd.dim)  
    
def process_volume() :
    print('3D watershed not implemented')
    #pd.paint_obj.update_3d_view()

def process_slice(img, mask, width, height, tag) :
    print('process slice image mask .. ')
    gray = img
    gray = np.where(mask == 65535, 0, gray)
    gray = gray.reshape(width,height)

    timg = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    cv2.imshow('img', timg)

    
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    
    # threshold procession
    ret, bin_img = cv2.threshold(gray,
                                 0, 255, 
                                 cv2.THRESH_OTSU)
    
    # noise removal - morphological gradient processing
    bin_img = cv2.morphologyEx(bin_img, 
                               cv2.MORPH_OPEN,
                               kernel,
                               iterations=2)

    # distance transform
    dist = cv2.distanceTransform(bin_img, cv2.DIST_L2, 5)

    #foreground area
    ret, sure_fg = cv2.threshold(dist, 0.25*dist.max(), 255, cv2.THRESH_BINARY)
    sure_fg = sure_fg.astype(np.uint8)

    #create markers
    ret, markers = cv2.connectedComponents(sure_fg)

    markers[dist == 0] = 65535
    
    ##watershed
    rgb = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    markers = cv2.watershed(rgb, markers)

    markers = markers.astype(np.uint16)
    markers[markers==65535] = 0
    mask[:] = markers.reshape(-1)
    print('done')


if __name__ == '__main__':
    print('cv2.watershed')
