from pydialog import dialog
import paintmod
import numpy as np
from skimage.segmentation import random_walker
import traceback

class paint_data :
    def __init__(self) :
        self.paint_obj = 0
        self.volume = 0
        self.mask = 0
        self.lut = 0
        self.label_color = 0
        self.boxmin = np.zeros(3, np.int32)
        self.boxmax = np.zeros(3, np.int32)
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = np.zeros(3, np.int32)
        self.script_args = 0

print('paint_data declared')
pd = paint_data()


def set_paint_data(py_obj) :
    pd.paint_obj = py_obj
    pd.volume = py_obj.get_volume_view()
    pd.labels = py_obj.get_mask_view()
    pd.lut = py_obj.get_lut_view()
    pd.label_color = py_obj.get_labelcolors_view()
    pd.boxmin = py_obj.get_boxmin();
    pd.boxmax = py_obj.get_boxmax();
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
    print('init random walker')
    pd.volume = pd.volume.reshape(pd.dim)  
    pd.labels = pd.labels.reshape(pd.dim)  
#---------------    



def process_slice(img, mask, width, height, tag) :
    print('process slice image mask ... ')
    print(pd.paint_obj.script_args)
    try : 
        foreground = np.where(img > 0, img, 0)
        foregound = np.where(mask == 65535, 0, foreground)  # set masked background pixels to 0
        
        # 0 = unlabeled, 1 = foreground, 2 = background
        labels = np.where(foreground == 0, 2, 0)
        labels = np.where((mask==tag), 1, labels)
        labels = np.where((mask>0) & (mask!=tag), 2, labels)

        foreground = foreground.reshape(width, height)
        labels = labels.reshape(width, height)

        beta = pd.paint_obj.script_args["beta"]
        mode = pd.paint_obj.script_args["mode"]
        #tol = pd.paint_obj.script_args["tol"]
        result = random_walker(img.reshape(width,height), labels, beta=beta, mode=mode)
        result = result.reshape(-1)
        mask[:] = np.where(result==1, tag, 0)
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()


        
def process_volume() :
    print('propagate labels using random walker through the selected region ...')
    try : 
        tag = dialog.get_int("Label to expand", "Label",
                             1, 1, 65535, 1)
        
        # take only visible labels and that too in visible region
        #segmentation = (
        #    (pd.label_color[3::4] > 0)[pd.labels] &
        #    (pd.lut[3::4] > 0)[pd.volume]
        #) * pd.labels

        foreground = (pd.lut[3::4] > 0)[pd.volume]
        # set labels that are set to 0 as 0
        foregound = np.where((pd.label_color[3::4] > 0)[pd.labels], foreground, 0)
        foreground = np.where(foreground, pd.volume, 0)

        
        # 0 = unlabeled, 1 = foreground, 2 = background
        labels = np.where(foreground == 0, 2, 0)
        labels = np.where((pd.labels==tag), 1, labels)
        labels = np.where((pd.labels>0) & (pd.labels!=tag), 2, labels)

        
        beta = pd.paint_obj.script_args["beta"]
        mode = pd.paint_obj.script_args["mode"]
        
        result = random_walker(foreground, labels, beta=beta, mode=mode)
        pd.labels[:] = np.where(result==1, tag, pd.labels).astype(np.uint16)
        
        pd.paint_obj.update_3d_view()
        pd.paint_obj.update_slice_view()
        print('done')
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()
#---------------    
#---------------    
        
if __name__ == '__main__':
    print('random walker using skimage')
