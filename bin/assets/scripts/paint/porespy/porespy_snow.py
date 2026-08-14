import paintmod
import numpy as np
import porespy as ps
import traceback
import matplotlib.pyplot as plt

class paint_data :
    def __init__(self) :
        self.paint_obj = 0
        self.volume = 0
        self.labels = 0
        self.lut = 0
        self.label_color = 0
        self.boxmin = np.zeros(3, np.int32)
        self.boxmax = np.zeros(3, np.int32)
        self.depth = 0
        self.width = 0
        self.height = 0
        self.dim = np.zeros(3, np.int32)

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
    print('init porespy SNOW partitioning')
    pd.volume = pd.volume.reshape(pd.dim)  
    pd.labels = pd.labels.reshape(pd.dim)
    

    
def process_slice(img, mask, width, height, tag) :
    print('process slice image mask .. ')
    try : 
        # define foreground by visibility
        foreground = img > 0
        foregound = np.where(mask == 65535, 0, foreground)  # set masked background pixels to 0
        foreground = foreground.reshape(width, height)

        #2D images is divided into 2 chunks in each direction for a total of 4:
        parallel_kw = {"divs": 2}
        snow_out = ps.filters.snow_partitioning_parallel(
            im=foreground, parallel_kw=parallel_kw, r_max=5, sigma=0.4
        )

        markers = snow_out.regions.astype(np.uint16)
        mask[:] = markers.reshape(-1)
        print('Number of regions : ', snow_out.regions.max())
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()


def process_volume() :
    print('perform 3d watershed')
    try : 
        # define foreground by visibility
        #foreground = np.take(pd.lut[3::4]>0, pd.volume)        
        #mask = np.take(pd.label_color[3::4]>0, pd.labels) # consider label visibility
        #foreground = np.where(mask>0, foreground, 0)
        foreground = np.where(
            np.take(pd.label_color[3::4] > 0, pd.labels, mode='clip'),
            np.take(pd.lut[3::4] > 0, pd.volume, mode='clip'),
            0
        )
        
        print(pd.boxmin, pd.boxmax)

        foreground = foreground[pd.boxmin[0]:pd.boxmax[0],
                                pd.boxmin[1]:pd.boxmax[1],
                                pd.boxmin[2]:pd.boxmax[2]]
        #3D images is divided into 2 chunks in each direction for a total of 8:
        parallel_kw = {"divs": 2}
        snow_out = ps.filters.snow_partitioning_parallel(
            im=foreground, parallel_kw=parallel_kw, r_max=5, sigma=0.4
        )

        #pd.labels[:] = snow_out.regions.astype(np.uint16)
        #### looks like porespy snow partitioning is spitting out array
        #### which is shorter by 1 voxel in each dimension
        pd.labels[pd.boxmin[0]:pd.boxmax[0]-1,
                  pd.boxmin[1]:pd.boxmax[1]-1,
                  pd.boxmin[2]:pd.boxmax[2]-1] = snow_out.regions.astype(np.uint16)
        
        pd.paint_obj.update_3d_view()
        pd.paint_obj.update_slice_view()
        print('Number of regions : ', snow_out.regions.max())
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()

    
        


if __name__ == '__main__':
    print('porespy SNOW partitioning')
