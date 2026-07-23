import paintmod
import os
import numpy as np
from biomedisa.deeplearning import deep_learning
import traceback

class paint_data :
    def __init__(self) :
        self.paint_obj = 0
        self.volume = 0
        self.mask = 0
        self.lut = 0
        self.label_color = 0
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = np.zeros(3,np.int32)
        self.script_args = 0

print('paint_data declared')
pd = paint_data()

script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
pos = script_path.find('scripts'+os.sep+'paint')
path_to_models = script_path[0:pos] + os.sep + 'models' + os.sep
print(path_to_models)

def set_paint_data(py_obj) :
    try :
        pd.paint_obj = py_obj
        pd.volume = py_obj.get_volume_view()
        pd.mask = py_obj.get_mask_view()
        pd.lut = py_obj.get_lut_view()
        pd.label_color = py_obj.get_labelcolors_view()
        pd.depth = py_obj.depth
        pd.width = py_obj.width
        pd.height = py_obj.height
        pd.dim[0] = pd.depth
        pd.dim[1] = pd.width
        pd.dim[2] = pd.height
        print(pd.depth*pd.width*pd.height)
        print(pd.depth, pd.width, pd.height)
        print(pd.volume.shape)
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()

    
def init() :
    print('init biomedisa particle segmentation')
    pd.volume = pd.volume.reshape(pd.dim)
    pd.mask = pd.mask.reshape(pd.dim)
    print(pd.volume.shape, pd.volume.dtype)
    print(pd.mask.shape, pd.mask.dtype)


def process_slice(img, mask, width, height, tag) :
    t_img = np.where(mask==65535, 0, img) # set masked background pixels to 0
    t_img = t_img.reshape(width,height,1)

    # define mask by visibility
    lut = pd.lut[::4]
    lut[lut > 0] = 1
    mask2 = np.take(lut, t_img)
    
    batch_size = pd.paint_obj.script_args["batch_size"]
    epochs = pd.paint_obj.script_args["epochs"]
    min_particle_size = pd.paint_obj.script_args["min_particle_size"]
    downsample = pd.paint_obj.script_args["downsample"]
    model = path_to_models + pd.paint_obj.script_args["model"]
    try :
        results = deep_learning(t_img,
                                mask_data=mask2,
                                path_to_model=model,
                                predict=True,
                                epochs=epochs,
                                min_particle_size=min_particle_size,
                                downsample=downsample,
                                batch_size=512)
        mask[:] = results['regular'].astype(np.uint16).reshape(width*height)
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()

    
    
def process_volume() :
    print('volume processing using biomedisa particle segmentation')
    try :
        script_path = os.path.abspath(__file__)
        script_dir = os.path.dirname(script_path)
        os.chdir(script_dir)
        
        batch_size = pd.paint_obj.script_args["batch_size"]
        epochs = pd.paint_obj.script_args["epochs"]
        min_particle_size = pd.paint_obj.script_args["min_particle_size"]
        downsample = pd.paint_obj.script_args["downsample"]
        model = path_to_models + pd.paint_obj.script_args["model"]

        # define mask by visibility
        lut = pd.lut[::4]
        lut[lut > 0] = 1
        mask = np.take(lut, pd.volume)
        #mask[:] = pd.mask
        
        #model='model_svl_step=2.h5'
        #model='sam_vit_l_0b3195.pth'
        results = deep_learning(pd.volume,
                                #mask_data=pd.mask,
                                mask_data=mask,
                                path_to_model=model,
                                predict=True,
                                epochs=epochs,
                                min_particle_size=min_particle_size,
                                downsample=downsample,
                                batch_size=512)
        
        pd.mask[:] = results['regular'].astype(np.uint16)
        pd.paint_obj.update_3d_view()
        pd.paint_obj.update_slice_view()
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()

    
if __name__ == '__main__':
    print('biomedisa.interpolation')
