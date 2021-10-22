import os
import sys
import numpy as np
from myutils import get_patches,reconstruct_from_patches
import tensorflow as tf
from keras_unet_collection import models, losses
import unet2D
import blosc2
from PIL import Image

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
    imgfile = open(pvl, 'rb')
    vtype = imgfile.read(1)
    dim = np.fromfile(imgfile, dtype=np.int32, count=3)
    imgdata = np.fromfile(imgfile, dtype=np.uint8, count=dim[0]*dim[1]*dim[2])
    imgfile.close()
    imgdata = imgdata.reshape(dim)    
    return (dim, imgdata)
#-------


#def mainModule(argv) :
#    if len(argv) < 4 :
#        print("\npython "+argv[0]+" <pvl> <mask> <segraw>")
#        exit()
#    pvlfile = argv[1]
#    maskfile = argv[2]
#    segfile = argv[3]

def mainModule(args) :
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
    
    BatchSize = 8
    # batchsize of images per training step
    if kwargs.get('batchsize') :
        BatchSize = int(kwargs['batchsize'])
    print('BatchSize = ',BatchSize)
    
    ImgSize = 256
    # imagesize for training
    if kwargs.get('imgsize') :
        ImgSize = int(kwargs['imgsize'])
    print('imgsize = ',ImgSize)
    
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
        maskdata = np.swapaxes(maskdata, 0, MajorAxis)
        imgdata = np.swapaxes(imgdata, 0, MajorAxis)

    print(dim)
    print(imgdata.shape)
    print(maskdata.shape)
    

    maskSlices = []
    if MaskValue < 1 :
        for i in range(dim[0]) :
            if np.max(maskdata[i,:,:]) > 0  :
                maskSlices.append(i)
    else :
        for i in range(dim[0]) :
            if MaskValue in maskdata[i,:,:] :
                maskSlices.append(i)

    ntrain = len(maskSlices)
    print('Number slices for training and validation ',ntrain)
    #print(maskSlices)
                

    OImgSizeW = dim[1]
    OImgSizeH = dim[2]
    #ImgSize = 500
    ncropW = OImgSizeW//ImgSize + (OImgSizeW%ImgSize>0)
    ncropH = OImgSizeH//ImgSize + (OImgSizeH%ImgSize>0)
    ncrops = ncropW*ncropH
    nempty = 1

    
    X_train = np.zeros((nempty*ncrops*ntrain, ImgSize, ImgSize, 1))
    Y_train = np.zeros((nempty*ncrops*ntrain, ImgSize, ImgSize, 1))
    print(X_train.shape)

    n = 0
    for i in range(ntrain) :
        img = imgdata[maskSlices[i],:,:].astype(np.float32)
        #img = img[0:ImgSize, 0:ImgSize].reshape((ImgSize,ImgSize,1))
        img = img.reshape((OImgSizeW,OImgSizeH,1))
        x = img/255.0
        d_crops = get_patches(img_arr = x,
                              size = ImgSize,
                              stride = ImgSize)
        for cdata in d_crops :
            X_train[n] = cdata
            n = n + nempty
    print(np.min(X_train), np.max(X_train))

    n = 0
    for i in range(ntrain) :
        img = maskdata[maskSlices[i],:,:].astype(np.float32)        
        if MaskValue > 0 :
            img[img != MaskValue] = 0
        #img = img[0:ImgSize, 0:ImgSize].reshape((ImgSize,ImgSize,1))
        #x = img/255.0
        img = img.reshape((OImgSizeW,OImgSizeH,1))
        x = (img > 0)
        d_crops = get_patches(img_arr = x,
                              size = ImgSize,
                              stride = ImgSize)
        for cdata in d_crops :
            Y_train[n] = cdata
            n = n + nempty
    print(np.min(Y_train), np.max(Y_train))
    
    #-----------------------------------------------------------------
    model = unet2D.unet2D(input_size = (ImgSize, ImgSize, 1),
                          filter_num = [2, 4],
                          n_labels = 1,
                          stack_num_down = 2,  # number of iterations per downsampling level
                          stack_num_up = 2,  # number of iterations per upsampling level
                          activation = 'ReLU',
                          batch_norm=True,
                          pool = 'max',
                          unpool = True,
                          dropout = 0.0,
                          use_dropout_on_upsampling = False,
                          dropout_type = 'spatial',
                          output_activation='sigmoid')


    model.compile(optimizer = 'nadam',
                  loss = 'binary_crossentropy',
                  metrics = [losses.dice_coef])
    #model.summary()

    results = model.fit(X_train,
                        Y_train,
                        validation_split = 0.25,
                        batch_size = BatchSize,
                        epochs = Epochs,
                        verbose=2)
    #-----------------------------------------------------------------


    #-----------------------------------------------------------------
    # test the model
    print('\n\nmodel.predict ...')
    segDim = np.array([dim[0], OImgSizeW, OImgSizeH])
    Seg = np.zeros(segDim, np.uint8)
    print(segDim)
    print(Seg.shape)
        
    n = 0
    for i in range(dim[0]) :
        img = imgdata[i,:,:].astype(np.float32)
        #img = img[0:ImgSize, 0:ImgSize].reshape((1,ImgSize,ImgSize,1))
        img = img.reshape((OImgSizeW,OImgSizeH,1))
        x = img/255.0
        d_crops = get_patches(img_arr = x,
                              size = ImgSize,
                              stride = ImgSize)
        preds_test = model.predict(d_crops)
        x_reconstructed = reconstruct_from_patches(img_arr=preds_test,
                                                   org_img_size=(OImgSizeW, OImgSizeH),
                                                   stride=ImgSize)
        x = x_reconstructed.reshape((OImgSizeW, OImgSizeH))
        #x = model.predict(x)
        x = x * 255
        x = x.astype(np.uint8)
        #x = x.reshape((ImgSize,ImgSize))
        #Seg[i,0:ImgSize,0:ImgSize] = x
        Seg[i,:,:] = x

    print(Seg.dtype)
    print('prediction done')

    
    if MajorAxis != 0 :
        if MajorAxis == 1 :
            segDim = np.array([segDim[1],segDim[0],segDim[2]])
        if MajorAxis == 2 :
            segDim = np.array([segDim[2],segDim[1],segDim[0]])
        Seg = np.swapaxes(Seg, 0, MajorAxis)

    
    segfile = open(segfile, 'wb')
    vtype = b'\0'
    segfile.write(vtype)
    segfile.write(segDim.tobytes())
    segfile.write(Seg.tobytes())
    segfile.close()

#    segfile = open('thread_seg.pvl.nc', 'w')
#    segfile.write('<!DOCTYPE Drishti_Header>\n')
#    segfile.write('<PvlDotNcFileHeader>\n')
#    segfile.write('  <pvlnames>seg.raw</pvlnames>\n')
#    segfile.write('  <voxeltype>unsigned short</voxeltype>\n')
#    segfile.write('  <pvlvoxeltype>unsigned char</pvlvoxeltype>\n')
#    segfile.write(f'  <gridsize>{segDim[0]} {segDim[1]} {segDim[2]}</gridsize>\n')
#    segfile.write('  <voxelunit>millimeter</voxelunit>\n')
#    segfile.write('  <voxelsize>1 1 1</voxelsize>\n')
#    segfile.write('  <description>Segmented using ML</description>\n')
#    segfile.write(f'  <slabsize>{segDim[0]}</slabsize>\n')
#    segfile.write('  <rawmap>0 255 </rawmap>\n')
#    segfile.write('  <pvlmap>0 255 </pvlmap>\n')
#    segfile.write('</PvlDotNcFileHeader>\n')
#    segfile.close()
    
    print('done')
    ##-----------------------------------------------------------------


if __name__ == "__main__" :
    kwargs = {}
    for a in sys.argv[1:] :
        (k,v) = a.split("=")
        kwargs[k] = v
    
    mainModule(kwargs)
