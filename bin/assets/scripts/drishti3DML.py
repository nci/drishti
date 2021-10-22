import os
import sys
import numpy as np
from myutils import get_patches,reconstruct_from_patches
import tensorflow as tf
import blosc2

sys.path.append('unet_collection')
from unet_collection import losses

#import unet3D
#from unet_collection import unet_plus_3d
from unet_collection import unet_3d
#-------



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


def mainModule(args) :
    print('------------')
    print('Arguments :')
    for kw in kwargs:
        print(kw, '-', kwargs[kw])
    print('------------')
        
    pvlfile = kwargs['volume']
    maskfile = kwargs['mask']
    segfile = kwargs['output']


    Epochs = 50
    # number of training epochs
    if kwargs.get('epochs') :
        Epochs = int(kwargs['epochs'])
    print('epochs = ',Epochs)
    
    BoxSize = (64,64,64)
    # imagesize for training
    if kwargs.get('boxsize') :
        b = kwargs['boxsize'].split(',')
        BoxSize = tuple((int(b[0]), int(b[1]), int(b[2])))
    print('Boxsize = ',BoxSize)

    BoxList = []
    # list of subvolume boxes for training
    if kwargs.get('boxlist') :
        blistfile = kwargs['boxlist']
        print(blistfile)
        with open(blistfile, 'rt') as file :
            for line in file:
                b = line.split(" ")
                if len(b) == 3 :
                    BoxList.append([int(b[0]),int(b[1]),int(b[2])])
    BoxList = np.array(BoxList)
    print('BoxList.shape ',BoxList.shape)
    
    
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
    
    OImgSizeD = dim[0]
    OImgSizeW = dim[1]
    OImgSizeH = dim[2]
    
    #X_train = []
    #Y_train = []
    #for i in range(0, OImgSizeD, BoxSize[0]) :
    #    ie = min(OImgSizeD,i+BoxSize[0])
    #    for j in range(0, OImgSizeW, BoxSize[1]) :
    #        je = min(OImgSizeW,j+BoxSize[1])
    #        for k in range(0, OImgSizeH, BoxSize[2]) :
    #            ke = min(OImgSizeH,k+BoxSize[2])
    #            ma = np.zeros(BoxSize)
    #            ma[0:ie-i, 0:je-j, 0:ke-k] = maskdata[i:ie, j:je, k:ke]
    #            if MaskValue < 1 :
    #                if np.max(ma) > 0  :
    #                    #print(i)
    #                    ma = ma > 0
    #                    Y_train.append(ma)
    #                    ma = np.zeros(BoxSize)
    #                    ma[0:ie-i, 0:je-j, 0:ke-k] = imgdata[i:ie, j:je, k:ke]
    #                    X_train.append(ma/255)
    #            else :
    #                if MaskValue in ma :
    #                    #print(i)
    #                    ma[ma != MaskValue] = 0
    #                    ma = ma > 0
    #                    Y_train.append(ma)
    #                    ma = np.zeros(BoxSize)
    #                    ma[0:ie-i, 0:je-j, 0:ke-k] = imgdata[i:ie, j:je, k:ke]
    #                    X_train.append(ma/255)

    X_train = []
    Y_train = []
    for i in range(len(BoxList)) :
        b = BoxList[i]
        xs = b[0]
        ys = b[1]                   
        zs = b[2]
        print(b)
                   
        xe = min(OImgSizeD,xs+BoxSize[0])
        ye = min(OImgSizeW,ys+BoxSize[1])
        ze = min(OImgSizeH,zs+BoxSize[2])
                   
        ma = np.zeros(BoxSize)
        ma[0:xe-xs, 0:ye-ys, 0:ze-zs] = maskdata[xs:xe, ys:ye, zs:ze]
        if MaskValue < 1 :
            if np.max(ma) > 0  :
                ma = ma > 0
                Y_train.append(ma)
                ma = np.zeros(BoxSize)
                ma[0:xe-xs, 0:ye-ys, 0:ze-zs] = imgdata[xs:xe, ys:ye, zs:ze]
                X_train.append(ma/255)
        else :
            if MaskValue in ma :
                ma[ma != MaskValue] = 0
                ma = ma > 0
                Y_train.append(ma)
                ma = np.zeros(BoxSize)
                ma[0:xe-xs, 0:ye-ys, 0:ze-zs] = imgdata[xs:xe, ys:ye, zs:ze]
                X_train.append(ma/255)
    
    print('Blocks for training and validation ',len(X_train))

    X_train = np.reshape(X_train, (-1, BoxSize[0], BoxSize[1], BoxSize[2]))
    Y_train = np.reshape(Y_train, (-1, BoxSize[0], BoxSize[1], BoxSize[2]))
    X_train = X_train.astype(np.float32)
    Y_train = Y_train.astype(np.float32)

    print(np.min(X_train), np.max(X_train))
    print(np.min(Y_train), np.max(Y_train))

    #exit()

    #-----------------------------------------------------------------
#    model = unet3D.custom_Unet3D(input_size = (BoxSize[0],BoxSize[1],BoxSize[2],1),
#                                 num_classes = 1,
#                                 filters = [32, 64],
#                                 dropout = 0.0)
 
#    model = unet_plus_3d.unet_plus_3d(input_size = (BoxSize[0],BoxSize[1],BoxSize[2],1),
#                                      filter_num = [32, 64, 128],
#                                      n_labels = 1,
#                                      stack_num_down = 2,  # number of iterations per downsampling level
#                                      stack_num_up = 2,  # number of iterations per upsampling level
#                                      activation = 'ReLU',
#                                      output_activation='Sigmoid',                          
#                                      batch_norm=False,
#                                      pool = True,
#                                      unpool = True,
#                                      deep_supervision=False)

    model = unet_3d.unet_3d(input_size = (BoxSize[0],BoxSize[1],BoxSize[2],1),
                            filter_num = [16, 32],
                            n_labels = 1,
                            stack_num_down = 2,  # number of iterations per downsampling level
                            stack_num_up = 2,  # number of iterations per upsampling level
                            activation = 'ReLU',
                            output_activation='Sigmoid',                          
                            batch_norm=False,
                            pool = True,
                            unpool = True)


    model.compile(optimizer = 'nadam',
                  loss = 'binary_crossentropy',
                  metrics = [losses.dice_coef])
    model.summary()

    #exit()
    results = model.fit(X_train,
                        Y_train,
                        validation_split = 0.25,
                        batch_size = BatchSize,
                        epochs = Epochs,
                        verbose=2)
    print('Training done')
    #-----------------------------------------------------------------



    #exit()
    

    #-----------------------------------------------------------------
    # test the model
    print('\n\nmodel.predict ...')
    sys.stdout.flush()
    
    segDim = np.array(dim)
    Seg = np.zeros(segDim, np.uint8)
    print(segDim)
    print(Seg.shape)
        
    n = 0
    for i in range(0, OImgSizeD, BoxSize[0]) :
        print(i)
        sys.stdout.flush()
        ie = min(OImgSizeD,i+BoxSize[0])
        for j in range(0, OImgSizeW, BoxSize[1]) :
            je = min(OImgSizeW,j+BoxSize[1])
            for k in range(0, OImgSizeH, BoxSize[2]) :
                ke = min(OImgSizeH,k+BoxSize[2])
                ma = np.zeros(BoxSize)
                ma[0:ie-i,0:je-j,0:ke-k] = imgdata[i:ie, j:je, k:ke]
                ma = ma/255.0
                ma = ma.reshape((BoxSize[0],BoxSize[1],BoxSize[2],1))
                ma = np.stack([ma])
                x = model.predict(ma)
                x = x*255
                x = x.astype(np.uint8)
                x = x.reshape(BoxSize)
                Seg[i:ie, j:je, k:ke] = x[0:ie-i,0:je-j,0:ke-k]

    print(Seg.dtype)
    print('prediction done')
    sys.stdout.flush()

    
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
