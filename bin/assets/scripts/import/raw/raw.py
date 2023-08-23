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
        
        self.title = 'Raw Handler'
        centralWidget = QWidget(self)
        mainLayout = QVBoxLayout(centralWidget)
        self.plainTextEdit = QPlainTextEdit()
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
            
            #self.plainTextEdit.appendPlainText(cmd)

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
        fin = open(flnm, 'rb')
        (self.voxelType) = numpy.fromfile(fin, dtype=numpy.int8, count=1)[0]
        (self.depth, self.width, self.height) = numpy.fromfile(fin, dtype=numpy.int32, count=3)
        fin.close()

        self.dim = (self.height, self.width, self.depth)
        self.headerBytes = 13
        
        self.load_entire_data()
        self.calculate_min_max()
        

        self.plainTextEdit.appendPlainText(flnm)
        self.plainTextEdit.appendPlainText('headerBytes = '+str(self.headerBytes))        
        self.plainTextEdit.appendPlainText('voxelType = '+str(self.voxelType))        
        self.plainTextEdit.appendPlainText('dimensions = '+str(self.depth)+' '+
                                           str(self.width)+' '+str(self.height))
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
    def load_entire_data(self) :
        self.plainTextEdit.appendPlainText('reading data')
        flnm = self.Filenames[0]
        fin = open(flnm, 'rb')

        # skip the header bytes
        fin.seek(self.headerBytes)
        
        self.data=0
        
        if self.voxelType == 0 : # UCHAR
            self.data = numpy.fromfile(fin, dtype=numpy.uint8, count=self.depth*self.width*self.height)
            self.rawMin = 0
            self.rawMax = 255
            
        if self.voxelType == 2 : # USHORT
            self.data = numpy.fromfile(fin, dtype=numpy.uint16, count=self.depth*self.width*self.height)
            self.rawMin = 0
            self.rawMax = 65535            

        fin.close()
        self.plainTextEdit.appendPlainText('data in memory')
    #--------------------

        
    #--------------------
    def calculate_min_max(self) :
        self.plainTextEdit.appendPlainText('calculate min max')
        self.dataMin = numpy.min(self.data)
        self.dataMax = numpy.max(self.data)
        self.plainTextEdit.appendPlainText('min max calculated')
    #--------------------


    #--------------------
    def gen_histogram(self):
        self.plainTextEdit.appendPlainText('calculate histogram')
        bins = list(range(self.rawMin, self.rawMax+2))
        self.histogram, b = numpy.histogram(self.data, bins)
        self.histogram.astype(numpy.int64)
        self.plainTextEdit.appendPlainText('histogram size : '+str(len(self.histogram)))
        self.plainTextEdit.appendPlainText('histogram calculated')
    #--------------------

    
    #--------------------
    def send_depth_slice(self, d):
        slice_size = self.width * self.height
        dstart = d * slice_size
        depth_slice = self.data[dstart:dstart+slice_size]                                
        self.send_data('depthslice', depth_slice)
    #--------------------

    
    #--------------------
    def send_rawvalue(self, d, w, h):
        pos = d*self.width*self.height + w*self.height + h
        val = self.data[pos:pos+1]
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
    
