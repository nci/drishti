import os
import sys
import numpy as np
from myutils import get_patches,reconstruct_from_patches
import tensorflow as tf
import blosc2

from n2v.models import N2VConfig, N2V
from csbdeep.utils import plot_history
from csbdeep.io import save_tiff_imagej_compatible
from n2v.utils.n2v_utils import manipulate_val_data
from n2v.internals.N2V_DataGenerator import N2V_DataGenerator


UINT8 = 1

#-------
def loadPVLData(pvl) :    
    imgfile = open(pvl, 'rb')
    vtype = imgfile.read(1)
    print(vtype)
    if vtype == b'\0' :
        UNIT8 = 1
    if vtype == b'\2' :
        UNIT8 = 0
        
    dim = np.fromfile(imgfile, dtype=np.int32, count=3)
    if UINT8 :
        imgdata = np.fromfile(imgfile, dtype=np.uint8, count=dim[0]*dim[1]*dim[2])
    else :
        imgdata = np.fromfile(imgfile, dtype=np.uint16, count=dim[0]*dim[1]*dim[2])
    imgfile.close()
    imgdata = imgdata.reshape(dim)    
    return (dim, imgdata)
#-------



def loadTrainingData(BoxList, dim, imgdata):    
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

        ma = imgdata[xs:xe, ys:ye, zs:ze]
        ma = np.squeeze(ma)
        #X_train.append(ma/255)
        X_train.append(ma)
    
    print('Blocks for training and validation ',len(X_train))
    ImgSize = X_train[0].shape[0]
    X_train = np.reshape(X_train, (-1, ImgSize, ImgSize))
    X_train = X_train.astype(np.float32)
    
    print(np.min(X_train), np.max(X_train))

    return (ImgSize, X_train)



def mainModule(args) :
    print('------------')
    print('Arguments :')
    for kw in kwargs:
        print(kw, '-', kwargs[kw])
    print('------------')
        
    pvlfile = kwargs['volume']
    denoisefile = kwargs['output']


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
    
    
    print('------------')
    print(pvlfile)
    print(denoisefile)
    print('------------')

    dim, imgdata = loadPVLData(pvlfile)    
    print(dim)
    print(imgdata.shape)
                    

    ImgSize, X_train = loadTrainingData(BoxList, dim, imgdata)

    print('ImgSize : ',ImgSize)
    
    OImgSizeW = dim[1]
    OImgSizeH = dim[2]
    stride = int(ImgSize/3)
    if ImgSize == OImgSizeW or ImgSize == OImgSizeH :
        stride = ImgSize
    
    #exit()

    #--------------------------
    datagen = N2V_DataGenerator()

    lx = len(X_train)
    trainSamples = int(lx*0.8);
    valSamples = lx - trainSamples;

    X = []
    for i in range(trainSamples) :
        x = X_train[i,:,:].astype(np.float32)
        x = x.reshape((1, ImgSize,ImgSize, 1))
        X.append(x)

    X = datagen.generate_patches_from_list(X, shape=(96,96))
    print(X.shape)
    print('Training Samples : ',X.shape)

    X_val = []
    for i in range(valSamples) :
        x = X_train[i+trainSamples,:,:].astype(np.float32)
        x = x.reshape((1, ImgSize,ImgSize, 1))
        X_val.append(x)

    X_val = datagen.generate_patches_from_list(X_val, shape=(96,96))
    print(X_val.shape)
    print('Validation Samples : ',X_val.shape)

    #exit()
    
    #--------------------------

    config = N2VConfig(X,
                       unet_n_depth=3,
                       unet_kern_size=3,
                       unet_n_first=32,
                       unet_last_activation='linear',
                       batch_norm=True,
                       train_loss='mse',
                       train_epochs=Epochs,
                       train_steps_per_epoch=10,
                       train_batch_size=128,
                       train_tensorboard=False,
                       train_checkpoint='None',
                       n2v_perc_pix=1.6,
                       n2v_patch_shape=(64, 64),
                       n2v_manipulator='uniform_withCP',
                       n2v_neighborhood_radius=5)
    # Let's look at the parameters stored in the config-object.
    print(vars(config))
    
    modelbase = os.path.split(denoisefile)
    print('Save model to ',modelbase[0])
    
    model = N2V(config, 'DeNoise', basedir=modelbase[0])
    history = model.train(X, X_val, verbose=2)
    #-----------------------------------------------------------------

    #exit()
    
    #-----------------------------------------------------------------
    # test the model

    if MajorAxis != 0 :
        print('------')
        print('Change axis - ')
        print(dim)
        print(imgdata.shape)
        print('------')

        if MajorAxis == 1 :
            dim = np.array([dim[1],dim[0],dim[2]])
        if MajorAxis == 2 :
            dim = np.array([dim[2],dim[1],dim[0]])
        imgdata = np.swapaxes(imgdata, 0, MajorAxis)

    print('\n\nmodel.predict ...')
    denoiseDim = np.array(dim)
    if UINT8 :
        DeNoise = np.zeros(denoiseDim, np.uint8)
    else :
        DeNoise = np.zeros(denoiseDim, np.uint16)
    print(denoiseDim)
    print(DeNoise.shape)

    n = 0
    for i in range(dim[0]) :
        img = imgdata[i,:,:].astype(np.float32)
        x = model.predict(img, 'YX')
        #x = x * 255
        if UINT8 :
            x = x.astype(np.uint8)
        else :
            x = x.astype(np.uint16)
        DeNoise[i,:,:] = x

        
    print('prediction done')


    if MajorAxis != 0 :
        if MajorAxis == 1 :
            denoiseDim = np.array([denoiseDim[1],denoiseDim[0],denoiseDim[2]])
        if MajorAxis == 2 :
            denoiseDim = np.array([denoiseDim[2],denoiseDim[1],denoiseDim[0]])
        DeNoise = np.swapaxes(DeNoise, 0, MajorAxis)

    #-------------
    # save denoised volume
    f = open(denoisefile, 'wb')
    if UINT8 :
        vtype = b'\0'
    else :
        vtype = b'\2'
    f.write(vtype)
    f.write(denoiseDim.tobytes())
    f.write(DeNoise.tobytes())
    f.close()
    #-------------
        

    #-------------
    #save associated pvl.nc file is required
    sf = os.path.split(denoisefile)
    denoiseraw = sf[1]
    sfp = os.path.splitext(denoiseraw)
    denoisepvl = os.path.join(sf[0],sfp[0]+".pvl.nc")
    
    f = open(denoisepvl, 'w')
    f.write('<!DOCTYPE Drishti_Header>\n')
    f.write('<PvlDotNcFileHeader>\n')
    f.write('  <pvlnames>'+denoiseraw+'</pvlnames>\n')
    f.write('  <voxeltype>unsigned short</voxeltype>\n')
    if UINT8 :
        f.write('  <pvlvoxeltype>unsigned char</pvlvoxeltype>\n')
    else:
        f.write('  <pvlvoxeltype>unsigned short</pvlvoxeltype>\n')
    f.write(f'  <gridsize>{denoiseDim[0]} {denoiseDim[1]} {denoiseDim[2]}</gridsize>\n')
    f.write('  <voxelunit>millimeter</voxelunit>\n')
    f.write('  <voxelsize>1 1 1</voxelsize>\n')
    f.write('  <description>Denoised using Noise2Void</description>\n')
    f.write(f'  <slabsize>{denoiseDim[0]}</slabsize>\n')
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
