import sys
import os

from PyQt5 import QtCore, QtWidgets

from PyQt5.QtNetwork import (
    QLocalSocket,
    QLocalServer,
    QUdpSocket,
    QHostAddress
)

import pyvista
from pyvistaqt import BackgroundPlotter, QtInteractor, MainWindow
import numpy
import scipy


#--------------------
class OriginalMesh(QtCore.QObject) :
    meshChanged = QtCore.pyqtSignal(str, pyvista.core.pointset.PolyData)
    
    #--------------------
    def __init__(self, plotter) :
        super(OriginalMesh, self).__init__(None)
        self.mesh_flnm = 0        
        self.plotter = plotter

        self.mesh_actor = 0
        self.chkbox = 0
    #--------------------
        
    #--------------------
    def set_mesh(self, flnm) :
        self.mesh_flnm = flnm
        self.mesh = pyvista.read(self.mesh_flnm)

        self.rgb = False
        self.rgb_data = 0
        if 'RGB' in self.mesh.point_data :
            self.rgb_data = self.mesh.point_data['RGB']
            self.rgb = True

        tstr = 'original v({}) f({})'.format(self.mesh.n_points, self.mesh.n_faces)
        self.plotter.add_text(tstr, font_size=14)

        if self.rgb :
            self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                    smooth_shading=True, specular=0.2,
                                                    rgb=True, scalars=self.rgb_data,
                                                    name='mesh')
        else :
            self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                    smooth_shading=True, specular=0.2,
                                                    color='aliceblue',
                                                    name='mesh')

            
        self.chkbox = self.plotter.add_checkbox_button_widget(self.toggle_edges, value=False, color_on='g')
        self.plotter.add_text('Edges', position = (70, 10), name='chkbox')


        self.emitMeshChangedSignal()
    #--------------------
        
    #--------------------
    def toggle_edges(self, value) :
        self.mesh_actor.prop.show_edges = value
    #--------------------

    #--------------------
    def emitMeshChangedSignal(self) :
        self.meshChanged.emit(self.mesh_flnm, self.mesh)
    #--------------------

#--------------------


class MeshCurvature(QtCore.QObject) :
    meshChanged = QtCore.pyqtSignal(str, pyvista.core.pointset.PolyData)
    
    def __init__(self, plotter) :
        super(MeshCurvature, self).__init__(None)
        self.mesh_flnm = 0
        self.plotter = plotter

        self.mesh = 0
        self.mesh_actor = 0
        self.curv_actor = 0
        self.text_actor = 0

        self.chkbox = self.plotter.add_checkbox_button_widget(self.toggle_edges, value=False, color_on='g')
        self.plotter.add_text('Edges', position = (70, 10), name='chkbox')

        # smoothing too slow - just do it from within DrishtiMesh
        #self.smooth = self.plotter.add_slider_widget(self.smooth_point_colors,
        #                                             rng=[0, 100],
        #                                             value=0,
        #                                             title='Smooth Color',
        #                                             pointa=(0.2, 0.2),
        #                                             pointb=(0.8, 0.2),
        #                                             style='modern') 
        #self.smooth.GetSliderRepresentation().SetSliderWidth(0.02)
        #self.smooth.GetSliderRepresentation().SetTubeWidth(0.02)
        #self.smooth.GetSliderRepresentation().SetTitleHeight(0.02)
        #self.smooth.GetSliderRepresentation().SetLabelHeight(0.02)
        
    #--------------------
    def set_mesh(self, mesh_flnm, meshOrig) :
        self.mesh_flnm = mesh_flnm
        self.mesh_orig = meshOrig

        self.mesh = self.mesh_orig.copy()

        self.calc_curvature()
    #--------------------

    #--------------------
    def toggle_edges(self, value) :
        if self.mesh_actor == 0 :
            return
        
        self.mesh_actor.prop.show_edges = value
    #--------------------

    #--------------------
    def emitMeshChangedSignal(self) :
        self.meshChanged.emit(self.mesh_flnm, self.mesh)
    #--------------------

        
    #----------------------
    def calc_curvature(self) :
        self.curv = self.mesh.curvature(curv_type='gaussian')
        self.mesh['curvature'] = self.curv
        cmin = numpy.percentile(self.curv, 10)
        cmax = numpy.percentile(self.curv, 90)        
        
        showedges = self.chkbox.GetRepresentation().GetState()
        self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                scalars='curvature',
                                                cmap='RdYlBu',
                                                clim=[cmin, cmax],
                                                smooth_shading=True, specular=0.2,
                                                show_edges=showedges, line_width=1,
                                                name='curv')

        self.emitMeshChangedSignal()

        self.gen_point_colors()
        self.gen_adjacency_matrix()
    #----------------------

    
    #----------------------
    def gen_point_colors(self) :
        self.lookup_table = self.mesh_actor.mapper.lookup_table

        nverts = self.mesh.n_points
        self.rgb = numpy.zeros([nverts, 3], numpy.uint8)        
        #self.rgb = numpy.zeros([nverts, 3])        
        for i in range(nverts) :
            color = numpy.array(self.lookup_table.map_value(self.curv[i])[0:3])
            self.rgb[i] = 255*color

        #print(self.rgb.max())
    #----------------------

    
    #----------------------
    def gen_adjacency_matrix(self) :
        self.faces = self.mesh.faces.reshape((-1,4))[:,1:4]

        #nverts = self.mesh.n_points
        #self.adj = scipy.sparse.dok_matrix((nverts, nverts), dtype=numpy.uint8)
        ## assuming triangulate surface, we can ignore face type value
        #for i in range(len(faces)) :
        #    n1, n2, n3 = faces[i]
        #    self.adj[n1,n2] = 1
        #    self.adj[n1,n3] = 1
        #    self.adj[n2,n1] = 1
        #    self.adj[n2,n3] = 1
        #    self.adj[n3,n1] = 1                        
        #    self.adj[n3,n2] = 1
    #----------------------


    #----------------------
    def smooth_point_colors(self, val) :
        if self.mesh == 0 :
            return
        
        nverts = self.mesh.n_points

        self.smooth_rgb = self.rgb
        self.smooth_rgb = self.smooth_rgb.astype(numpy.float32)
        
        niter = int(val)
        nfaces = len(self.faces)
        for i in range(niter) :
            count = numpy.zeros(nverts)
            rgbI = self.smooth_rgb
            self.smooth_rgb = numpy.zeros([nverts,3], dtype=numpy.float32)
            for f in range(nfaces) :
                n1, n2, n3 = self.faces[f]
                
                c1 = rgbI[n1]
                c2 = rgbI[n2]
                c3 = rgbI[n3]
                
                color = (c1+c2+c3)/3
                
                self.smooth_rgb[n1] += color
                self.smooth_rgb[n2] += color
                self.smooth_rgb[n3] += color
                count[n1] += 1
                count[n2] += 1
                count[n3] += 1

            for n in range(nverts) :
                if count[n] > 0 :
                    self.smooth_rgb[n] /= count[n]
            #------------------
        #------------------

        self.smooth_rgb = self.smooth_rgb.astype(numpy.uint8)

        self.mesh.point_data['RGB'] = self.smooth_rgb
        
        showedges = self.chkbox.GetRepresentation().GetState()
        self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                smooth_shading=True, specular=0.2,
                                                rgb=True, scalars=self.smooth_rgb,
                                                show_edges=showedges, line_width=1,
                                                name='curv')
    #--------------------



    #--------------------
    def save_mesh(self, flnm) :
        self.mesh.save(flnm)
    #--------------------
        
    #--------------------
    def save_mesh(self) :
        save_dir = QtCore.QFileInfo(self.mesh_flnm).absolutePath()
        flnm, ok = QtWidgets.QFileDialog.getSaveFileName(
            None,
            'Save Simplified Surface',
            save_dir,
            'Files (*.ply *.obj *.stl)'
        )
              
        if len(flnm) == 0 or ok == False :
            return

        if flnm[-4:] != '.obj' and flnm[-4:] != '.ply' and flnm[-4:] != '.stl' :
            QtWidgets.QMessageBox.information(None, 'Error', 'Cannot save '+flnm+'\n .ply .obj or .stl required')
            return

 
        self.mesh.save(flnm, texture=self.rgb)

        
        sendingPort = 7760
        sendingSocket = QUdpSocket()
        mesg = 'load '+flnm
        mesg = QtCore.QByteArray(mesg.encode())
        sendingSocket.writeDatagram(mesg, QHostAddress.LocalHost, sendingPort)

        QtWidgets.QMessageBox.information(None, 'Saved', flnm)
    #--------------------        

#--------------------
        

#--------------------
class MyMainWindow(MainWindow) :
    def __init__(self, parent=None, mesh_flnm='') :
        QtWidgets.QMainWindow.__init__(self, parent)
        self.setWindowTitle('Simplify')
        
        self.frame = QtWidgets.QFrame()
        hlayout = QtWidgets.QHBoxLayout()
        self.plotter0 = QtInteractor(self.frame)
        self.plotter1 = QtInteractor(self.frame)

        hlayout.addWidget(self.plotter0)
        hlayout.addWidget(self.plotter1)
        self.signal_close.connect(self.plotter0.close)
        self.signal_close.connect(self.plotter1.close)

        self.frame.setLayout(hlayout)
        self.setCentralWidget(self.frame)


        self.mesh0 = OriginalMesh(self.plotter0)
        self.mesh1 = MeshCurvature(self.plotter1)

        self.mesh0.meshChanged.connect(self.mesh1.set_mesh)

        menubar = self.menuBar()
        file_menu = menubar.addMenu('File')

        save_action = QtWidgets.QAction('Save', self)
        save_action.triggered.connect(self.mesh1.save_mesh)
        file_menu.addAction(save_action)

        self.mesh0.set_mesh(mesh_flnm)

        self.plotter0.link_views_across_plotters(self.plotter1)
        
        self.show()        
        
        #--- to kill the process ---
        self.listeningSocket = QUdpSocket()
        res = self.listeningSocket.bind(QHostAddress.LocalHost, 7770)
        self.listeningSocket.readyRead.connect(self.readyRead)

        
    def readyRead(self) :        
        while self.listeningSocket.hasPendingDatagrams() :
            datagram = self.listeningSocket.receiveDatagram()                       
            data = bytes(datagram.data()).decode()
            if data.find(':') != -1 :
                cmd, payload = data.split(' : ')
            else :
                cmd = data
            
            if cmd == 'exit' :
                sys.exit()
#--------------------
#--------------------

        
#--------------------
if __name__ == '__main__' :
    app = QtWidgets.QApplication(sys.argv)
    window = MyMainWindow(mesh_flnm = sys.argv[1])
    sys.exit(app.exec_())
