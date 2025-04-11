import os
import sys
import numpy

from PyQt5.QtCore import (
    QByteArray,
    QDataStream,
    QFile
)

from PyQt5.QtNetwork import (
    QLocalSocket,
    QLocalServer,
    QTcpSocket,
    QUdpSocket,
    QHostAddress
)
    
from PyQt5.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QMessageBox,
    QProgressDialog,
    QPlainTextEdit,
    QVBoxLayout
)

import select
import socket


class MainWindow(QMainWindow) :

    #--------------------
    def __init__(self, app):
        super().__init__()
        
        self.title = 'Numpy Handler'
        centralWidget = QWidget(self)
        mainLayout = QVBoxLayout(centralWidget)
        self.plainTextEdit = QPlainTextEdit()
        self.plainTextEdit.appendPlainText('NumPy Handler started')
        mainLayout.addWidget(self.plainTextEdit)
        self.setCentralWidget(centralWidget)
        self.resize(500,300)
        self.show()

        self.listeningSocket = QUdpSocket()
        res = self.listeningSocket.bind(QHostAddress.LocalHost, 7760)
        self.listeningSocket.readyRead.connect(self.readyRead)

        self.sendingPort = 7761
        self.sendingSocket = QUdpSocket()
    #--------------------
        
    
    #--------------------
    #--------------------
        
        
    #--------------------
    def readyRead(self) :
        while self.listeningSocket.hasPendingDatagrams() :
            datagram = self.listeningSocket.receiveDatagram()                       
            data = bytes(datagram.data()).decode()

            if data.find(':') != -1 :
                cmd, payload = data.split(' : ')
            else :
                cmd = data
            
            self.plainTextEdit.appendPlainText(cmd)

            if cmd == 'exit' :
                sys.exit()

            if cmd == 'mmap' :
                self.mmap_flnm = payload
                
            if cmd == 'setfiles' :
                flnms = payload.split(' , ')
                self.setFiles(flnms)
            
            if cmd == 'histogram' :
                self.send_data('histogram', self.histogram)
            
            if cmd == 'depthslice' :
                slc = int(payload) # slice number expected
                self.send_depth_slice(slc)

            if cmd == 'rawvalue' :
                d, w, h = payload.split(' , ')
                d = int(d)
                w = int(w)
                h = int(h)
                self.send_rawvalue(d,w,h)


    #--------------------
    def setFiles(self, flnms) :
        self.Filenames = flnms
        for fl in self.Filenames :
            self.plainTextEdit.appendPlainText(fl)
            
        flnm = self.Filenames[0]
        self.plainTextEdit.appendPlainText(flnm)
    
        self.headerBytes = 0
        self.plainTextEdit.appendPlainText('headerBytes = '+str(self.headerBytes))        

        self.data = numpy.load(flnm, mmap_mode='r')

        self.voxelType = 0
        if self.data[0,0,0].dtype == numpy.dtype('B') :
            self.voxelType = 0
        if self.data[0,0,0].dtype == numpy.dtype('b') :
            self.voxelType = 1
        if self.data[0,0,0].dtype == numpy.dtype('u2') :
            self.voxelType = 2
        if self.data[0,0,0].dtype == numpy.dtype('i2') :
            self.voxelType = 3
        if self.data[0,0,0].dtype == numpy.dtype('i4') :
            self.voxelType = 4
        if self.data[0,0,0].dtype == numpy.dtype('f4') :
            self.voxelType = 5

        self.plainTextEdit.appendPlainText('voxelType = '+str(self.voxelType))        

        (self.depth, self.width, self.height) = self.data.shape
        self.plainTextEdit.appendPlainText('dimensions = '+str(self.depth)+' '+
                                           str(self.width)+' '+str(self.height))
        

        self.calculate_min_max()        
        self.plainTextEdit.appendPlainText('data min max = '+str(self.dataMin)+' '+str(self.dataMax))
        
        mesg = 'voxeltype : ' + str(self.voxelType)
        mesg = QByteArray(mesg.encode())
        res = self.sendingSocket.writeDatagram(mesg, QHostAddress.LocalHost, self.sendingPort)
        
        
        mesg = 'header : '+str(self.headerBytes)
        mesg = QByteArray(mesg.encode())
        res = self.sendingSocket.writeDatagram(mesg, QHostAddress.LocalHost, self.sendingPort)
        
        mesg = 'dim : '+str(self.depth)+' , '+str(self.width)+' , '+str(self.height)
        mesg = QByteArray(mesg.encode())
        res = self.sendingSocket.writeDatagram(mesg, QHostAddress.LocalHost, self.sendingPort)
        
        mesg = 'rawminmax : '+str(self.rawMin)+' , '+str(self.rawMax)
        mesg = QByteArray(mesg.encode())
        res = self.sendingSocket.writeDatagram(mesg, QHostAddress.LocalHost, self.sendingPort)
    
        self.gen_histogram()

    #--------------------

        
    #--------------------
    def calculate_min_max(self) :
        self.plainTextEdit.appendPlainText('calculate min max')
        self.dataMin = numpy.min(self.data)
        self.dataMax = numpy.max(self.data)
        self.rawMin = self.dataMin
        self.rawMax = self.dataMax
        self.plainTextEdit.appendPlainText('min max calculated')
    #--------------------


    #--------------------
    def gen_histogram(self):
        self.plainTextEdit.appendPlainText('calculate histogram')
        if self.voxelType < 2 :
            self.histogram, b = numpy.histogram(self.data, bins=256)
        else :
            self.histogram, b = numpy.histogram(self.data, bins=65536)
        self.histogram.astype(numpy.int64)
        self.plainTextEdit.appendPlainText('histogram size : '+str(len(self.histogram)))
        self.plainTextEdit.appendPlainText('histogram calculated')
    #--------------------

    
    #--------------------
    def send_depth_slice(self, d):
        depth_slice = self.data[d, :]
        self.send_data('depthslice', depth_slice)
    #--------------------

    
    #--------------------
    def send_rawvalue(self, d, w, h):
        val = self.data[d, w, h]
        self.send_data('rawvalue', val)
    #--------------------



    #--------------------
    # id_str is string identifying data
    # data is numpy array
    def send_data(self, id_str, data) :
        f = QFile(self.mmap_flnm)
        f.open(QFile.ReadWrite) 
        nele = len(data)
        f.write(nele.to_bytes(4, 'little'))
        f.write(bytes(data))
        f.close()
        
        id = QByteArray(id_str.encode())
        self.sendingSocket.writeDatagram(id, QHostAddress.LocalHost, self.sendingPort)
    #--------------------

    
#--------------------
if __name__ == '__main__' :
    app = QApplication(sys.argv)
    ex = MainWindow(app)
    sys.exit(app.exec())
    
