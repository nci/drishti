import os
import sys
import numpy as np
from myutils import get_patches,reconstruct_from_patches
import tensorflow as tf
import blosc2

sys.path.append('unet_collection')
from unet_collection import unet3_plus_2d, losses


UINT8 = 1
MAXVALUE = 255
DTYPE = np.uint8

#-------
def updateMaskFile(maskfile, Seg) :
    print('\n\nSaving - ',maskfile)
    dim = np.array(Seg.shape)
    f = open(maskfile, 'wb')
    vtype = b'\0'
    f.write(vtype)
    f.write(dim.tobytes())
    f.write(Seg.tobytes())
    f.close()
#-------

#-------
def loadMaskData(maskfilename) :
    maskfile = open(maskfilename, 'rb')
    maskfile.read(7)

    dim = np.fromfile(maskfile, dtype=np.int32, count=3)

    maskdata = np.zeros((dim[0]*dim[1]*dim[2]), dtype=np.uint8)

    nblocks = int.from_bytes(maskfile.read(4), byteorder='little', signed=True)
    mb100 = int.from_bytes(maskfile.read(4), byteorder='little', signed=True)    
    ns = 0
    for i in range(nblocks) :
        vbsize = int.from_bytes(maskfile.read(4), byteorder='little', signed=True)
        vbuf = maskfile.read(vbsize)
        vbuf = blosc2.decompress(vbuf)
        maskdata[ns:ns+mb100] = np.frombuffer(vbuf, np.uint8)
        ns = ns + mb100

    maskfile.close()
    
    maskdata = maskdata.reshape(dim)

    return maskdata
#-------

#-------
def loadPVLData(pvl) :    
    global UINT8, MAXVALUE, DTYPE
    imgfile = open(pvl, 'rb')
    vtype = imgfile.read(1)
    if vtype == b'\0' :
        DTYPE = np.uint8
        UINT8 = 1
        MAXVALUE = 255        
    if vtype == b'\2' :
        DTYPE = np.uint16
        UINT8 = 0
        MAXVALUE = 65555
    dim = np.fromfile(imgfile, dtype=np.int32, count=3)

    imgdata = np.fromfile(imgfile, dtype=DTYPE, count=dim[0]*dim[1]*dim[2])

    imgfile.close()
    imgdata = imgdata.reshape(dim)    
    return (dim, imgdata)
#-------



def loadTrainingData(MaskValue, BoxList, dim, maskdata, imgdata):    
    global UINT8, MAXVALUE, DTYPE
    OImgSizeD = dim[0]
    OImgSizeW = dim[1]
    OImgSizeH = dim[2]
    X_train = []
    Y_train = []
    for i in range(len(BoxList)) :
        b = BoxList[i]
        xs = b[0]
        ys = b[1]                   
        zs = b[2]
        xe = b[3]
        ye = b[4]                   
        ze = b[5]

        ma = maskdata[xs:xe, ys:ye, zs:ze]
        ma = np.squeeze(ma)
        if MaskValue < 1 :
            if np.max(ma) > 0  :
                ma = ma > 0
                Y_train.append(ma)
                ma = imgdata[xs:xe, ys:ye, zs:ze]
                ma = np.squeeze(ma)
                X_train.append(ma/MAXVALUE)
        else :
            if MaskValue in ma :
                ma[ma != MaskValue] = 0
                ma = ma > 0
                Y_train.append(ma)
                ma = imgdata[xs:xe, ys:ye, zs:ze]
                ma = np.squeeze(ma)
                X_train.append(ma/MAXVALUE)
    
    print('Blocks for training and validation ',len(X_train))
    ImgSize = X_train[0].shape[0]
    X_train = np.reshape(X_train, (-1, ImgSize, ImgSize))
    Y_train = np.reshape(Y_train, (-1, ImgSize, ImgSize))
    X_train = X_train.astype(np.float32)
    Y_train = Y_train.astype(np.float32)
    
    print(np.min(X_train), np.max(X_train))
    print(np.min(Y_train), np.max(Y_train))

    return (ImgSize, X_train, Y_train)



def mainModule(args) :
    global UINT8, MAXVALUE, DTYPE
    print('------------')
    print('Arguments :')
    for kw in kwargs:
        print(kw, '-', kwargs[kw])
    print('------------')
        
    pvlfile = kwargs['volume']
    maskfile = kwargs['mask']
    segfile = kwargs['output']


    MajorAxis = 0
    # swap axis if majoraxis is not 0
    if kwargs.get('majoraxis') :
        MajorAxis = int(kwargs['majoraxis'])
    print('majorAxis = ',MajorAxis)

    Epochs = 50
    # number of training epochs
    if kwargs.get('epochs') :
        Epochs = int(kwargs['epochs'])
    print('epochs = ',Epochs)


    down_filter = [16, 32, 64, 128]
    # down filters
    if kwargs.get('down_filters') :
        line = kwargs['down_filters']
        b = line.split(' ')
        while '' in b :   # remove white spaces
            b.remove('')
        down_filters = [int(x) for x in b]
    print('down_filters = ',down_filters)
        
    up_filters = 16
    # number of up filters
    if kwargs.get('up_filter') :
        up_filters = int(kwargs['up_filters'])
    print('up_filters = ',up_filters)

    
    
    
    print('---------------------------------------')
    print('---------------------------------------')
    BoxList = []
    # list of subvolume boxes for training
    if kwargs.get('boxlist') :
        blistfile = kwargs['boxlist']
        print(blistfile)
        with open(blistfile, 'rt') as file :
            for line in file:
                b = line.split(' ')
                while '' in b :   # remove white spaces
                    b.remove('')
                if len(b) == 6 :
                    BoxList.append([int(b[0]), int(b[1]), int(b[2]),
                                    int(b[3]), int(b[4]), int(b[5])])
    BoxList = np.array(BoxList)
    print('BoxList.shape ',BoxList.shape)
    print('---------------------------------------')
    print('---------------------------------------')
 
    BatchSize = 8
    # batchsize of images per training step
    if kwargs.get('batchsize') :
        BatchSize = int(kwargs['batchsize'])
    print('BatchSize = ',BatchSize)
    
    MaskValue = -1
    # mask value from maskdata to be used for training
    # -1 means take all values above 0 for training
    if kwargs.get('maskvalue') :
        MaskValue = int(kwargs['maskvalue'])
    print('MaskValue = ',MaskValue)

    
    
    print('------------')
    print(pvlfile)
    print(maskfile)
    print(segfile)
    print('------------')

    dim, imgdata = loadPVLData(pvlfile)
    maskdata = loadMaskData(maskfile)

    
    print(dim)
    print(imgdata.shape)
    print(maskdata.shape)
                    

    ImgSize, X_train, Y_train = loadTrainingData(MaskValue,
                                                 BoxList,
                                                 dim,
                                                 maskdata,
                                                 imgdata)

    print('ImgSize : ',ImgSize)
    
    OImgSizeW = dim[1]
    OImgSizeH = dim[2]
    stride = int(ImgSize/3)
    if ImgSize == OImgSizeW or ImgSize == OImgSizeH :
        stride = ImgSize
    
    #exit()

    #-----------------------------------------------------------------
    model = unet3_plus_2d.unet3_plus_2d(input_size = (ImgSize, ImgSize, 1),
                                        filter_num = down_filters,
                                        up_filters = up_filters,
                                        n_labels = 1,
                                        activation = 'ReLU',
                                        output_activation='sigmoid')

    model.summary()

    optimizer = tf.keras.optimizers.SGD(nesterov=True, momentum=0.99)

    model.compile(optimizer = optimizer,
                  loss = 'binary_crossentropy',
                  metrics = [losses.dice_coef])

    results = model.fit(X_train,
                        Y_train,
                        validation_split = 0.25,
                        batch_size = BatchSize,
                        epochs = Epochs,
                        verbose=2)
    #-----------------------------------------------------------------
    
    #exit()
    
    #-----------------------------------------------------------------
    # test the model

    if MajorAxis != 0 :
        print('------')
        print('Change axis - ')
        print(dim)
        print(imgdata.shape)
        print(maskdata.shape)
        print('------')

        if MajorAxis == 1 :
            dim = np.array([dim[1],dim[0],dim[2]])
        if MajorAxis == 2 :
            dim = np.array([dim[2],dim[1],dim[0]])
        imgdata = np.swapaxes(imgdata, 0, MajorAxis)

    print('\n\nmodel.predict ...')
    segDim = np.array(dim)
    Seg = np.zeros(segDim, np.uint8)
    print(segDim)
    print(Seg.shape)

    n = 0
    for i in range(dim[0]) :
        img = imgdata[i,:,:].astype(np.float32)
        img = img.reshape((dim[1], dim[2],1))
        x = img/MAXVALUE

        ##------------------------------------
        ##-just tile the patches
        d_crops = get_patches(img_arr = x,
                              size = ImgSize,
                              stride = ImgSize)
        preds_test = model.predict(d_crops)
        if type(preds_test) == type([]) :
            preds_test = preds_test[-1]
        x_reconstructed = reconstruct_from_patches(img_arr=preds_test,
                                                   org_img_size=(dim[1], dim[2]),
                                                   stride=ImgSize,
                                                   size=ImgSize)
        x = x_reconstructed.reshape((dim[1], dim[2]))
        x = x * 255
        x = x.astype(np.uint8)
        Seg[i,:,:] = x
        ##------------------------------------

        
    print('prediction done')


    if MajorAxis != 0 :
        if MajorAxis == 1 :
            segDim = np.array([segDim[1],segDim[0],segDim[2]])
        if MajorAxis == 2 :
            segDim = np.array([segDim[2],segDim[1],segDim[0]])
        Seg = np.swapaxes(Seg, 0, MajorAxis)

    #-------------
    # save probalilities to a raw file
    f = open(segfile, 'wb')
    vtype = b'\0'
    f.write(vtype)
    f.write(segDim.tobytes())
    f.write(Seg.tobytes())
    f.close()
    #-------------
        

    #-------------
    #save associated pvl.nc file is required
    sf = os.path.split(segfile)
    segraw = sf[1]
    sfp = os.path.splitext(segraw)
    segpvl = os.path.join(sf[0],sfp[0]+".pvl.nc")
    
    f = open(segpvl, 'w')
    f.write('<!DOCTYPE Drishti_Header>\n')
    f.write('<PvlDotNcFileHeader>\n')
    f.write('  <pvlnames>'+segraw+'</pvlnames>\n')
    f.write('  <voxeltype>unsigned short</voxeltype>\n')
    f.write('  <pvlvoxeltype>unsigned char</pvlvoxeltype>\n')
    f.write(f'  <gridsize>{segDim[0]} {segDim[1]} {segDim[2]}</gridsize>\n')
    f.write('  <voxelunit>millimeter</voxelunit>\n')
    f.write('  <voxelsize>1 1 1</voxelsize>\n')
    f.write('  <description>Segmented using ML</description>\n')
    f.write(f'  <slabsize>{segDim[0]}</slabsize>\n')
    f.write('  <rawmap>0 255 </rawmap>\n')
    f.write('  <pvlmap>0 255 </pvlmap>\n')
    f.write('</PvlDotNcFileHeader>\n')
    f.close()
    #-------------

    
    print('done')
    ##-----------------------------------------------------------------


if __name__ == "__main__" :
    kwargs = {}
    for a in sys.argv[1:] :
        (k,v) = a.split("=")
        kwargs[k] = v
    
    mainModule(kwargs)
