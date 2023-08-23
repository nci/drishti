from __future__ import absolute_import

import tensorflow as tf
import tensorflow.keras.layers as layers
from tensorflow.keras.layers import ReLU, LeakyReLU, PReLU, ELU
import tensorflow.keras.models as models
import tensorflow.keras.activations as activations
import tensorflow.keras.metrics as metrics


# Defining the encoder's down-sampling blocks.
def encoder_block(inputs, n_filters, kernel_size, strides, activation):
    encoder = layers.Conv2D(filters=n_filters,
                            kernel_size=kernel_size,
                            strides=strides,
                            padding='same',
                            use_bias=False)(inputs)
    encoder = layers.BatchNormalization()(encoder)
    encoder = layers.Activation(activation)(encoder)
    encoder = layers.Conv2D(filters=n_filters,
                            kernel_size=kernel_size,
                            padding='same',
                            use_bias=False)(encoder)
    encoder = layers.BatchNormalization()(encoder)
    encoder = layers.Activation(activation)(encoder)
    return encoder


# Defining the decoder's up-sampling blocks.
def upscale_blocks(inputs, n_filters, activation):
    n_upscales = len(inputs)
    upscale_layers = []

    for i, inp in enumerate(inputs):
        p = n_upscales - i
        u = layers.Conv2DTranspose(filters=n_filters,
                                   kernel_size=3,
                                   strides=2**p,
                                   padding='same')(inp)

        for i in range(2):
            u = layers.Conv2D(filters=n_filters,
                              kernel_size=3,
                              padding='same',
                              use_bias=False)(u)
            u = layers.BatchNormalization()(u)
            u = layers.Activation(activation)(u)
            u = layers.Dropout(rate=0.4)(u)

        upscale_layers.append(u)
    return upscale_layers


# Defining the decoder's whole blocks.
def decoder_block(layers_to_upscale, inputs, n_filters, activation):
    upscaled_layers = upscale_blocks(layers_to_upscale, n_filters, activation)

    decoder_blocks = []

    for i, inp in enumerate(inputs):
        d = layers.Conv2D(filters=n_filters,
                          kernel_size=3,
                          strides=2**i,
                          padding='same',
                          use_bias=False)(inp)
        d = layers.BatchNormalization()(d)
        d = layers.Activation(activation)(d)
        d = layers.Conv2D(filters=n_filters,
                          kernel_size=3,
                          padding='same',
                          use_bias=False)(d)
        d = layers.BatchNormalization()(d)
        d = layers.Activation(activation)(d)

        decoder_blocks.append(d)

    decoder = layers.concatenate(upscaled_layers + decoder_blocks)
    decoder = layers.Conv2D(filters=n_filters*4,
                            kernel_size=3,
                            strides=1,
                            padding='same',
                            use_bias=False)(decoder)
    decoder = layers.BatchNormalization()(decoder)
    decoder = layers.Activation(activation)(decoder)
    decoder = layers.Dropout(rate=0.4)(decoder)

    return decoder


def unet3_plus_2d(input_size,
                  filter_num,
                  up_filters,
                  n_labels,
                  activation='ReLU',
                  output_activation='sigmoid'):
    inputs = layers.Input(input_size)

    X = inputs
    X = encoder_block(X, n_filters=filter_num[0], kernel_size=3, strides=1, activation=activation)

    X_list = [X]
    for i, f in enumerate(filter_num[1:]) :
        X = encoder_block(X, n_filters=f, kernel_size=3, strides=2, activation=activation)
        X_list.append(X)

    Y_list = []
    for i in range(len(X_list)-1) :
        l2u = [X_list[-1]] + Y_list
        inp = X_list[::-1][i+1:]
        X = decoder_block(layers_to_upscale=l2u, inputs=inp, n_filters=up_filters, activation=activation)
        Y_list.append(X)

    output = layers.Conv2D(filters=n_labels,
                           kernel_size=1,
                           padding='same',
                           activation=output_activation)(X)

    model = models.Model(inputs, output)
    return model

    #e1 = encoder_block(inputs, n_filters=32, kernel_size=3, strides=1, activation)
    #e2 = encoder_block(e1, n_filters=64, kernel_size=3, strides=2, activation)
    #e3 = encoder_block(e2, n_filters=128, kernel_size=3, strides=2, activation)
    #e4 = encoder_block(e3, n_filters=256, kernel_size=3, strides=2, activation)
    #e5 = encoder_block(e4, n_filters=512, kernel_size=3, strides=2, activation)

    #d4 = decoder_block(layers_to_upscale=[e5], inputs=[e4, e3, e2, e1], activation)
    #d3 = decoder_block(layers_to_upscale=[e5, d4], inputs=[e3, e2, e1], activation)
    #d2 = decoder_block(layers_to_upscale=[e5, d4, d3], inputs=[e2, e1], activation)
    #d1 = decoder_block(layers_to_upscale=[e5, d4, d3, d2], inputs=[e1], activation)
