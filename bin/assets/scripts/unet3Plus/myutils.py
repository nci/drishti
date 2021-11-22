import numpy as np

#-----------------------
def get_patches(img_arr, size=256, stride=256, within_bounds=0):
    """
    Takes single image or array of images and returns
    crops using sliding window method.
    If stride < size it will do overlapping.
    
    Args:
        img_arr (numpy.ndarray): [description]
        size (int, optional): [description]. Defaults to 256.
        stride (int, optional): [description]. Defaults to 256.
    
    Raises:
        ValueError: [description]
        ValueError: [description]
    
    Returns:
        numpy.ndarray: [description]
    """    
#    # check size and stride
#    if size % stride != 0:
#        raise ValueError("size % stride must be equal 0")

    patches_list = []
    overlapping = 0
    if stride != size:
        overlapping = (size // stride) - 1

    if img_arr.ndim == 3:
        i_max = img_arr.shape[0] // stride - overlapping
        if i_max*stride < img_arr.shape[0] :
            i_max = i_max + 1
        j_max = img_arr.shape[1] // stride - overlapping
        if j_max*stride < img_arr.shape[1] :
            j_max = j_max + 1

        if within_bounds == 1 : # limit the patches within bounds of the original image
            for i in range(i_max):
                istart = i * stride
                iend = i * stride + size
                if iend > img_arr.shape[0]-1 : # modify to fit within image boundary
                    iend = img_arr.shape[0]-1
                    istart = iend - size + 1
                for j in range(j_max):
                    jstart = j * stride
                    jend = j * stride + size
                    if jend > img_arr.shape[1]-1 : # modify to fit within image boundary
                        jend = img_arr.shape[1]-1
                        jstart = jend - size + 1
                    a = img_arr[istart : iend,
                                jstart : jend]
                    b = np.zeros((size,size,1))
                    b[0:a.shape[0], 0:a.shape[1], 0:] = a
                    patches_list.append(b)
        else :
            for i in range(i_max):
                for j in range(j_max):                
                    a = img_arr[i * stride : i * stride + size,
                                j * stride : j * stride + size]
                    b = np.zeros((size,size,1))
                    b[0:a.shape[0], 0:a.shape[1], 0:] = a
                    patches_list.append(b)

    elif img_arr.ndim == 4:
        i_max = img_arr.shape[1] // stride - overlapping
        for im in img_arr:
            for i in range(i_max):
                for j in range(j_max):
                    patches_list.append(
                        im[
                            i * stride : i * stride + size,
                            j * stride : j * stride + size,
                        ]
                    )

    else:
        raise ValueError("img_arr.ndim must be equal 3 or 4")

    #print('total patches : ', i_max, j_max, len(patches_list))
    return np.stack(patches_list)
#-----------------------


#-----------------------
def reconstruct_from_patches(img_arr, org_img_size, stride=None, size=None):
    """[summary]
    
    Args:
        img_arr (numpy.ndarray): [description]
        org_img_size (tuple): [description]
        stride ([type], optional): [description]. Defaults to None.
        size ([type], optional): [description]. Defaults to None.
    
    Raises:
        ValueError: [description]
    
    Returns:
        numpy.ndarray: [description]
    """

    #print('Img_Arr : ',img_arr.shape)
    #print('Orig_Img_Size : ',org_img_size)
    
    # check parameters
    if type(org_img_size) is not tuple:
        raise ValueError("org_image_size must be a tuple")

    if img_arr.ndim == 3:
        img_arr = np.expand_dims(img_arr, axis=0)

    if size is None:
        size = img_arr.shape[1]

    if stride is None:
        stride = size

    nm_layers = img_arr.shape[3]
    
    i_max = org_img_size[0] // stride
    if i_max*stride < org_img_size[0] :
        i_max = i_max + 1
        
    j_max = org_img_size[1] // stride
    if j_max*stride < org_img_size[1] :
        j_max = j_max + 1

    #total_nm_images = img_arr.shape[0] // (i_max ** 2)
    total_nm_images = img_arr.shape[0] // (i_max * j_max)
    nm_images = img_arr.shape[0]

    images_list = []
    kk = 0
    for img_count in range(total_nm_images):
        img_r = np.zeros(
            (i_max*stride, j_max*stride, nm_layers), dtype=img_arr[0].dtype
        )
        for i in range(i_max):
            for j in range(j_max):
                for layer in range(nm_layers):
                    img_r[
                        i * stride : i * stride + size,
                        j * stride : j * stride + size,
                        layer,
                    ] = img_arr[kk, :, :, layer]

                kk += 1

        img_bg = np.zeros(
            (org_img_size[0], org_img_size[1], nm_layers), dtype=img_arr[0].dtype
        )
        img_bg = img_r[0:org_img_size[0], 0:org_img_size[1], 0:]
        images_list.append(img_bg)

    return np.stack(images_list)
#-----------------------
