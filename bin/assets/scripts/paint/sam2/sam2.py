import paintmod
import sys
import os
import numpy as np

from PIL import Image
import cv2
import matplotlib.pyplot as plt

import torch
import torchvision
from segment_anything import sam_model_registry, SamAutomaticMaskGenerator, SamPredictor

num_cpus = os.cpu_count()
print('Number of CPUs : ',num_cpus)
print("PyTorch version:", torch.__version__)
print("Torchvision version:", torchvision.__version__)
print("CUDA is available:", torch.cuda.is_available())

torch.set_num_threads(num_cpus)
num_threads = torch.get_num_threads()
print(f"Number of threads being used by PyTorch: {num_threads}")


sam_checkpoint = sys.path[0]+os.sep+"sam_vit_h_4b8939.pth"
model_type = "vit_h"
sam_model = sam_model_registry[model_type](checkpoint=sam_checkpoint)
sam_model.to(device="cuda" if torch.cuda.is_available() else "cpu")

print('sam2 started')

class sam_data :
    def __init__(self) :
        self.points_per_side = 32
        self.pred_iou_thresh = 0.9
        self.stability_score_thresh = 0.96
        self.crop_n_layers = 1
        self.crop_n_points_downscale_factor = 2
        self.min_mask_region_area = 100
        self.mask_generator = 0
        self.predictor = 0

class paint_data :
    def __init__(self) :
        self.paint_obj = 0
        self.volume = 0
        self.mask = 0
        self.lut = 0
        self.depth = 0
        self.width = 0
        self.height = 0

print('paint_data declared')
pd = paint_data()
sam = sam_data()

def set_paint_data(py_obj) :
    print('unpacking the data')
    pd.paint_obj = py_obj
    pd.volume = py_obj.get_volume_view()
    pd.mask = py_obj.get_mask_view()
    pd.lut = py_obj.get_lut_view()
    pd.depth = py_obj.depth
    pd.width = py_obj.width
    pd.height = py_obj.height
    print(pd.depth*pd.width*pd.height)
    print(pd.depth, pd.width, pd.height)
    print(pd.volume.shape)
    print('transfer complete')    

def init_sam() :
    print('init sam')
    sam.mask_generator = SamAutomaticMaskGenerator(model=sam_model,
                                                   points_per_side = sam.points_per_side,
                                                   pred_iou_thresh = sam.pred_iou_thresh,
                                                   stability_score_thresh =sam.stability_score_thresh,
                                                   crop_n_layers = sam.crop_n_layers,
                                                   crop_n_points_downscale_factor = sam.crop_n_points_downscale_factor,
                                                   min_mask_region_area = sam.min_mask_region_area)
    #sam.predictor = SamPredictor(model=sam_model,
    #                             sam.points_per_side,
    #                             sam.pred_iou_thresh,
    #                             sam.stability_score_thresh,
    #                             sam.crop_n_layers,
    #                             sam.crop_n_points_downscale_factor,
    #                             sam.min_mask_region_area)
    print('mask_generator created')
    
def process_slice(slc_no) :
    print('process slice ',slc_no)
    lut = pd.lut
    lut = lut.reshape(256,4)
    print(lut.shape)
    lut = np.delete(lut, 3, axis=1)
    lut[:,[0,2]] = lut[:,[2,0]]
    lut = np.transpose(lut, axes=(1,0))
    lut = lut.reshape(-1).tolist()

    slc_size = pd.width*pd.height
    slc_start = slc_no*slc_size
    slice = pd.volume[slc_start:slc_start+slc_size]
    gray_array = slice.reshape(pd.width, pd.height)
    gray_img = Image.fromarray(gray_array).convert('RGB')
    rgb_img = gray_img.point(lut)

    #fig, axes = plt.subplots(1, 2, figsize=(10,5))
    #axes[0].imshow(rgb_img)
    #axes[1].imshow(gray_img)
    #plt.axis('off')
    #plt.tight_layout()
    #plt.show()

    print('prediction ....')
    rgb_array = np.asarray(rgb_img)
    masks = sam.mask_generator.generate(rgb_array)
    
    if len(masks) == 0 :
        print('no segmentation found')
        return
    else :
        print('Number of segmentations : ',len(masks))
    
    sorted_masks = sorted(masks, key=(lambda x: x['area']), reverse=True)
    maskId = 1
    for mask in masks :
        maskId = maskId + 1
        m = mask['segmentation']
        setm = m!=0 # boolean mask where mask != 0
        a = pd.mask[slc_start:slc_start+slc_size].reshape(pd.width, pd.height)
        a[setm] = maskId
        pd.mask[slc_start:slc_start+slc_size] = a.reshape(pd.width*pd.height);

    pd.paint_obj.update_slice_view()
    pd.paint_obj.update_3d_view()
    print('processing done')
    
    
