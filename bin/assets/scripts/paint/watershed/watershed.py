import paintmod
import numpy as np
from skimage.filters import threshold_otsu
from skimage.feature import peak_local_max
from skimage.morphology import label
from skimage.segmentation import watershed
from scipy.ndimage import distance_transform_edt
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
    pd.mask = pd.mask.reshape(pd.dim)  

    
def peaks_to_markers_3d(image, peaks):
    """Returns watershed markers from peaks data."""
    peaks_x, peaks_y, peaks_z = peaks.astype('int').T
    seeds = np.zeros(image.shape, dtype=bool)
    seeds[(peaks_x, peaks_y, peaks_z)] = 1
    # Label the marker points
    markers = label(seeds)    
    return markers

def process_volume() :
    print('perform 3d watershed')
    try : 
        # define foreground by visibility instead of otsu threshold
        #foreground = pd.volume >= threshold_otsu(pd.volume)
        foreground = np.take(pd.lut[::4]>0, pd.volume)
        
        distance_img = distance_transform_edt(foreground)
        
        peaks = peak_local_max(distance_img, labels=label(foreground), min_distance=5)
        
        # We do some minor tweaking to get the peaks data into the right format for watershed
        markers = peaks_to_markers_3d(pd.volume, peaks)
        
        # Watershed segmentation
        particle_labels = watershed(-distance_img, markers, mask=foreground)
        particle_labels = np.where(np.isnan(particle_labels), 0, particle_labels)
        pd.mask[:] = particle_labels.astype(np.uint16)
        pd.paint_obj.update_3d_view()
        pd.paint_obj.update_slice_view()
        print('done')
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()

    
def peaks_to_markers_2d(image, peaks):
    """Returns watershed markers from peaks data."""
    peaks_x, peaks_y = peaks.astype('int').T
    seeds = np.zeros(image.shape, dtype=bool)
    seeds[(peaks_x, peaks_y)] = 1
    # Label the marker points
    markers = label(seeds)    
    return markers

def process_slice(img, mask, width, height, tag) :
    print('process slice image mask .. ')
    try : 
        # define foreground by visibility instead of otsu threshold
        #foreground = img >= threshold_otsu(img)
        foreground = img > 0
        foregound = np.where(mask == 65535, 0, foreground)  # set masked background pixels to 0
        foreground = foreground.reshape(width, height)

        distance_img = distance_transform_edt(foreground)
        peaks = peak_local_max(distance_img, labels=label(foreground), min_distance=5)
        
        # We do some minor tweaking to get the peaks data into the right format for watershed
        markers = peaks_to_markers_2d(foreground, peaks)
        
        # Watershed segmentation
        particle_labels = watershed(-distance_img, markers, mask=foreground)
        
        markers = particle_labels.astype(np.uint16)
        markers = np.where(np.isnan(markers), 0, markers) # set nan pixels to 0
        mask[:] = markers.reshape(-1)
        print('done')
    except Exception as e :
        print('Error : ', str(e))
        print('Full Error : ', repr(e))
        traceback.print_exc()
        


if __name__ == '__main__':
    print('cv2.watershed')
