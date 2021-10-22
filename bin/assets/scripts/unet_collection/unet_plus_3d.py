from __future__ import absolute_import

from unet_collection.layer_utils import *

from tensorflow.keras.layers import Input, Conv3D
from tensorflow.keras.layers import BatchNormalization, Activation, concatenate, multiply
from tensorflow.keras.layers import ReLU, LeakyReLU, PReLU, ELU
from tensorflow.keras.models import Model

def UNET_left3D(X, channel, kernel_size=3, stack_num=2, activation='ReLU', 
                pool=True, batch_norm=False, name='left0'):
    '''
    The encoder block of U-net.
    
    UNET_left(X, channel, kernel_size=3, stack_num=2, activation='ReLU', 
              pool=True, batch_norm=False, name='left0')
    
    Input
    ----------
        X: input tensor.
        channel: number of convolution filters.
        kernel_size: size of 2-d convolution kernels.
        stack_num: number of convolutional layers.
        activation: one of the `tensorflow.keras.layers` interface, e.g., 'ReLU'.
        pool: True or 'max' for MaxPooling2D.
              'ave' for AveragePooling2D.
              False for strided conv + batch norm + activation.
        batch_norm: True for batch normalization, False otherwise.
        name: prefix of the created keras layers.
        
    Output
    ----------
        X: output tensor.
        
    '''
    pool_size = 2
    
    X = encode_layer3D(X, channel, pool_size, pool, activation=activation, 
                       batch_norm=batch_norm, name='{}_encode'.format(name))

    X = CONV_stack3D(X, channel, kernel_size, stack_num=stack_num, activation=activation, 
                     batch_norm=batch_norm, name='{}_conv'.format(name))
    
    return X


def UNET_right3D(X, X_list, channel, kernel_size=3, 
                 stack_num=2, activation='ReLU',
                 unpool=True, batch_norm=False, concat=True, name='right0'):
    
    '''
    The decoder block of U-net.
    
    Input
    ----------
        X: input tensor.
        X_list: a list of other tensors that connected to the input tensor.
        channel: number of convolution filters.
        kernel_size: size of 2-d convolution kernels.
        stack_num: number of convolutional layers.
        activation: one of the `tensorflow.keras.layers` interface, e.g., 'ReLU'.
        unpool: True or 'bilinear' for Upsampling2D with bilinear interpolation.
                'nearest' for Upsampling2D with nearest interpolation.
                False for Conv2DTranspose + batch norm + activation.
        batch_norm: True for batch normalization, False otherwise.
        concat: True for concatenating the corresponded X_list elements.
        name: prefix of the created keras layers.
        
    Output
    ----------
        X: output tensor.
    
    '''
    
    pool_size = 2
    
    X = decode_layer3D(X, channel, pool_size, unpool, 
                       activation=activation, batch_norm=batch_norm, name='{}_decode'.format(name))
    
    # linear convolutional layers before concatenation
    X = CONV_stack3D(X, channel, kernel_size, stack_num=1, activation=activation, 
                     batch_norm=batch_norm, name='{}_conv_before_concat'.format(name))
    if concat:
        # <--- *stacked convolutional can be applied here
        X = concatenate([X,]+X_list, axis=-1, name=name+'_concat')
    
    # Stacked convolutions after concatenation 
    X = CONV_stack3D(X, channel, kernel_size, stack_num=stack_num, activation=activation, 
                     batch_norm=batch_norm, name=name+'_conv_after_concat')
    
    return X

def unet_plus_3d(input_size, filter_num, n_labels,
                 stack_num_down=2, stack_num_up=2,
                 activation='ReLU', output_activation='Softmax', 
                 batch_norm=False, pool=True, unpool=True, deep_supervision=False, name='xnet'):
    '''
    U-net++ or nested U-net
    
    unet_plus_2d(input_size, filter_num, n_labels,
                 stack_num_down=2, stack_num_up=2,
                 activation='ReLU', output_activation='Softmax', 
                 batch_norm=False, pool=True, unpool=True, deep_supervision=False, name='xnet')
    
    ----------
    Zhou, Z., Siddiquee, M.M.R., Tajbakhsh, N. and Liang, J., 2018. Unet++: A nested u-net architecture 
    for medical image segmentation. In Deep Learning in Medical Image Analysis and Multimodal Learning 
    for Clinical Decision Support (pp. 3-11). Springer, Cham.
    
    Input
    ----------
        input_size: a tuple that defines the shape of input, e.g., (None, None, 3)
        filter_num: an iterable that defines number of filters for each \
                      down- and upsampling level. E.g., [64, 128, 256, 512]
                      the depth is expected as `len(filter_num)`
        n_labels: number of output labels.
        stack_num_down: number of convolutional layers per downsampling level/block. 
        stack_num_up: number of convolutional layers (after concatenation) per upsampling level/block.
        activation: one of the `tensorflow.keras.layers` or `keras_unet_collection.activations` interfaces, e.g., ReLU
        output_activation: one of the `tensorflow.keras.layers` or `keras_unet_collection.activations` interfaces or 'Sigmoid'.
                           Default option is Softmax
                           if None is received, then linear activation is applied.
        batch_norm: True for batch normalization.
        pool: True for maxpooling, False for strided convolutional layers.
        unpool: True for unpooling (i.e., reflective padding), False for transpose convolutional layers.
        deep_supervision: True for a model that supports deep supervision. Details see Zhou et al. (2018).
        name: prefix of the created keras layers.
        
    Output
    ----------
        X: a keras model 
    
    '''
    
    depth_ = len(filter_num)
    
    IN = Input(input_size)
    X = IN
    
    # allocate nested lists for collecting output tensors 
    X_nest_skip = [[] for _ in range(depth_)] 
    
    # downsampling blocks (same as in 'unet_2d')
    X = CONV_stack3D(X, filter_num[0],
                     stack_num=stack_num_down,
                     activation=activation, 
                     batch_norm=batch_norm, name='{}_down0'.format(name))
    X_nest_skip[0].append(X)
    for i, f in enumerate(filter_num[1:]):
        X = UNET_left3D(X, f, stack_num=stack_num_down,
                        activation=activation, 
                        pool=pool, batch_norm=batch_norm,
                        name='{}_down{}'.format(name, i+1))        
        X_nest_skip[0].append(X)
    
    for nest_lev in range(1, depth_):
        
        # subset filter numbers to the current upsampling level
        filter_num_sub = filter_num[:(depth_-nest_lev)]
        
        # loop over individual upsamling levels
        for i, f in enumerate(filter_num_sub[::-1]):
            
            # collecting previous downsampling outputs
            previous_skip = []
            for previous_lev in range(nest_lev):
                previous_skip.append(X_nest_skip[previous_lev][i])
                
            # upsamping block that concatenates all available (same feature map size) down-/upsampling outputs
            X_nest_skip[nest_lev].append(
                UNET_right3D(X_nest_skip[nest_lev-1][i+1],
                             previous_skip, filter_num[i], 
                             stack_num=stack_num_up,
                             activation=activation, name='xnet_{}{}'.format(nest_lev, i)))
            
    # output
    if deep_supervision:
        
        OUT_list = []
        print('----------\ndeep_supervision = True\nnames of output tensors are listed as follows (the last one is the final output):')
        
        for i in range(1, depth_):
            print('\t{}_output_sup{}'.format(name, i))
            OUT_list.append(CONV_output3D(X_nest_skip[i][0], n_labels, kernel_size=1, 
                                          activation=output_activation,
                                          name='{}_output_sup{}'.format(name, i)))
            
        print('\t{}_output_final'.format(name))
        OUT_list.append(CONV_output3D(X_nest_skip[-1][0],
                                      n_labels, kernel_size=1,
                                      activation=output_activation,
                                      name='{}_output'.format(name)))
        
    else:
        OUT = CONV_output3D(X_nest_skip[-1][0],
                            n_labels, kernel_size=1,
                            activation=output_activation,
                            name='{}_output'.format(name))
        OUT_list = [OUT,]
        
    # model
    model = Model(inputs=[IN], outputs=OUT_list, name='{}_model'.format(name))
    
    return model
