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
import pyacvd
import pymeshfix
import numpy

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

        
        
#--------------------
class ReMesh(QtCore.QObject) :
    meshChanged = QtCore.pyqtSignal(str, pyvista.core.pointset.PolyData)
    
    #--------------------
    def __init__(self, plotter) :
        super(ReMesh, self).__init__(None)
        self.mesh_flnm = 0
        self.plotter = plotter

        self.mesh_actor = 0

        self.chkbox = self.plotter.add_checkbox_button_widget(self.toggle_edges, value=False, color_on='g')
        self.plotter.add_text('Edges', position = (70, 10), name='chkbox')
        
        self.reduce = 0
        self.reduce = self.plotter.add_slider_widget(self.cluster_and_remesh,
                                                     rng=[1, 100],
                                                     value=10,
                                                     title='Percent Reduction',
                                                     pointa=(0.2, 0.1),
                                                     pointb=(0.8, 0.1),
                                                     style='modern')
    #--------------------
        
    #--------------------
    def set_mesh(self, mesh_flnm, meshOrig) :
        self.mesh_flnm = mesh_flnm
        self.mesh_orig = meshOrig
        
        self.cluster_and_remesh(1)
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
    def cluster_and_remesh(self, val) :
        if self.reduce == 0 :
            return

        nclus = int(self.reduce.GetSliderRepresentation().GetValue())
        
        num_clusters = self.mesh_orig.n_points * nclus * 0.01 # reduce to nclus percent        
        clus = pyacvd.Clustering(self.mesh_orig)
        clus.cluster(num_clusters)
        self.mesh = clus.create_mesh()

        #self.mesh = self.mesh_orig.decimate(1.0-nclus*0.01)

        #clus = pyacvd.Clustering(self.mesh)
        #clus.cluster(self.mesh.n_points)
        #self.mesh = clus.create_mesh()
        
        tstr = 'remesh v({}) f({})'.format(self.mesh.n_points, self.mesh.n_faces)
        self.plotter.add_text(tstr, font_size=14,
                              name='remesh_text')
        
        showedges = self.chkbox.GetRepresentation().GetState()
        self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                smooth_shading=True, specular=1.0,
                                                color='coral',
                                                show_edges=showedges, line_width=1,
                                                name='remesh')

        self.emitMeshChangedSignal()
    #----------------------


    #--------------------
    def save_mesh(self, flnm) :
        self.mesh.save(flnm)
    #--------------------
        
    #--------------------
    def save_mesh(self) :
        save_dir = QtCore.QFileInfo(self.mesh_flnm).absolutePath()
        flnm, ok = QtWidgets.QFileDialog.getSaveFileName(
            None,
            'Save Remesh Surface',
            save_dir,
            'Files (*.ply *.stl)'
        )
              
        if len(flnm) == 0 or ok == False :
            return

        if flnm[-4:] != '.ply' and flnm[-4:] != '.stl' :
            QtWidgets.QMessageBox.information(None, 'Error', 'Cannot save '+flnm+'\n .ply or .stl required')
            return
            
        
        self.mesh.save(flnm)

        QtWidgets.QMessageBox.information(None, 'Saved', flnm)
    #--------------------
        
#--------------------
        
#--------------------
class SmoothMesh(QtCore.QObject) :
    meshChanged = QtCore.pyqtSignal(str, pyvista.core.pointset.PolyData)
    
    #--------------------
    def __init__(self, plotter) :
        super(SmoothMesh, self).__init__(None)
        self.mesh_flnm = 0
        self.plotter = plotter

        self.mesh_actor = 0
        self.chkbox = 0
        self.strength = 0
        self.filter_quality = 0

        self.chkbox = self.plotter.add_checkbox_button_widget(self.toggle_edges, value=False, color_on='g')
        self.plotter.add_text('Edges', position = (70, 10), name='chkbox')
        
        self.strength = self.plotter.add_slider_widget(self.apply_smoothing,
                                                       rng=[1, 100],
                                                       value=10,
                                                       title='Strength',
                                                       pointa=(0.2, 0.2),
                                                       pointb=(0.8, 0.2),
                                                       style='modern') 
        self.strength.GetSliderRepresentation().SetSliderWidth(0.02)
        self.strength.GetSliderRepresentation().SetTubeWidth(0.02)
        self.strength.GetSliderRepresentation().SetTitleHeight(0.02)
        self.strength.GetSliderRepresentation().SetLabelHeight(0.02)
        
        self.filter_quality = self.plotter.add_slider_widget(self.apply_smoothing,
                                                             rng=[1, 100],
                                                             value=10,
                                                             title='Filter Quality',
                                                             pointa=(0.2, 0.1),
                                                             pointb=(0.8, 0.1),
                                                             style='modern')
        self.filter_quality.GetSliderRepresentation().SetSliderWidth(0.02)
        self.filter_quality.GetSliderRepresentation().SetTubeWidth(0.02)
        self.filter_quality.GetSliderRepresentation().SetTitleHeight(0.02 )
        self.filter_quality.GetSliderRepresentation().SetLabelHeight(0.02)
    #--------------------
        
    #--------------------
    def set_mesh(self, mesh_flnm, meshOrig) :
        self.mesh_flnm = mesh_flnm
        self.mesh_orig = meshOrig

        tstr = 'smooth v({}) f({})'.format(self.mesh_orig.n_points, self.mesh_orig.n_faces)
        self.plotter.add_text(tstr, font_size=14, name='title')
        
        self.apply_smoothing(1)
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

    #--------------------
    def apply_smoothing(self, val) :
        if self.strength == 0 or self.filter_quality == 0 :
            return

        iter = int(self.strength.GetSliderRepresentation().GetValue())
        self.strength.GetSliderRepresentation().SetValue(iter)

        quality = 0.99-0.009*self.filter_quality.GetSliderRepresentation().GetValue()
        
        self.mesh = self.mesh_orig.smooth(n_iter=iter,                                       
                                            relaxation_factor=quality,
                                            boundary_smoothing=False,
                                            feature_smoothing=False)
        
        showedges = self.chkbox.GetRepresentation().GetState()
        self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                smooth_shading=True, specular=0.2,
                                                color='beige',
                                                show_edges=showedges, line_width=1,
                                                name='smooth')

        self.emitMeshChangedSignal()
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
            'Save Smooth Surface',
            save_dir,
            'Files (*.ply *.stl)'
        )
              
        if len(flnm) == 0 or ok == False :
            return

        if flnm[-4:] != '.ply' and flnm[-4:] != '.stl' :
            QtWidgets.QMessageBox.information(None, 'Error', 'Cannot save '+flnm+'\n .ply or .stl required')
            return
            
        
        self.mesh.save(flnm)

        QtWidgets.QMessageBox.information(None, 'Saved', flnm)
    #--------------------
        
#--------------------
#--------------------
class MeshFix(QtCore.QObject) :
    meshChanged = QtCore.pyqtSignal(str, pyvista.core.pointset.PolyData)
    
    #--------------------
    def __init__(self, plotter) :
        super(MeshFix, self).__init__(None)
        self.mesh_flnm = 0
        self.plotter = plotter

        self.mesh_actor = 0

        self.chkbox = self.plotter.add_checkbox_button_widget(self.toggle_edges, value=False, color_on='g')
        self.plotter.add_text('Edges', position = (70, 10), name='chkbox')
    #--------------------
        
    #--------------------
    def set_mesh(self, mesh_flnm, meshOrig) :
        self.mesh_flnm = mesh_flnm
        self.mesh_orig = meshOrig
        
        self.fixmesh()
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
    def fixmesh(self) :
        #v = self.mesh_orig.points
        #f = self.mesh_orig.faces
        #f = f.reshape(-1,4)
        #f = numpy.delete(f, 0,1)

        meshfix = pymeshfix.MeshFix(self.mesh_orig)
        meshfix.repair(verbose=True, joincomp=True, remove_smallest_components=True)
        self.mesh = meshfix.mesh

        holes = meshfix.extract_holes()
        
        tstr = 'mesh fixed v({}) f({})'.format(self.mesh.n_points, self.mesh.n_faces)
        self.plotter.add_text(tstr, font_size=14,
                              name='meshfix_text')
        
        showedges = self.chkbox.GetRepresentation().GetState()
        self.mesh_actor = self.plotter.add_mesh(self.mesh,
                                                smooth_shading=True, specular=1.0,
                                                color='lightskyblue',
                                                show_edges=showedges, line_width=1,
                                                name='meshfix')
        if holes.n_points > 0 :
            self.plotter.add_mesh(holes,
                                  color='r',
                                  line_width=5,
                                  name='holes')

        
        self.emitMeshChangedSignal()
    #----------------------


    #--------------------
    def save_mesh(self, flnm) :
        self.mesh.save(flnm)
    #--------------------
        
    #--------------------
    def save_mesh(self) :
        save_dir = QtCore.QFileInfo(self.mesh_flnm).absolutePath()
        flnm, ok = QtWidgets.QFileDialog.getSaveFileName(
            None,
            'Save Fixed Mesh Surface',
            save_dir,
            'Files (*.ply *.stl)'
        )
              
        if len(flnm) == 0 or ok == False :
            return

        if flnm[-4:] != '.ply' and flnm[-4:] != '.stl' :
            QtWidgets.QMessageBox.information(None, 'Error', 'Cannot save '+flnm+'\n .ply or .stl required')
            return
            
        
        self.mesh.save(flnm)
        
        sendingPort = 7760
        sendingSocket = QUdpSocket()
        mesg = 'load '+flnm
        mesg = QtCore.QByteArray(mesg.encode())
        sendingSocket.writeDatagram(mesg, QHostAddress.LocalHost, sendingPort)

        QtWidgets.QMessageBox.information(None, 'Saved', flnm)
    #--------------------
        
#--------------------
        
#--------------------
#--------------------
        

#--------------------
class MyMainWindow(MainWindow) :
    def __init__(self, parent=None, mesh_flnm='') :
        QtWidgets.QMainWindow.__init__(self, parent)
        self.setWindowTitle('Smooth')
        
        self.frame = QtWidgets.QFrame()
        hlayout = QtWidgets.QHBoxLayout()
        self.plotter0 = QtInteractor(self.frame)
        self.plotter1 = QtInteractor(self.frame)
        self.plotter2 = QtInteractor(self.frame)
        self.plotter3 = QtInteractor(self.frame)

        hlayout.addWidget(self.plotter0)
        hlayout.addWidget(self.plotter1)
        hlayout.addWidget(self.plotter2)
        hlayout.addWidget(self.plotter3)
        self.signal_close.connect(self.plotter0.close)
        self.signal_close.connect(self.plotter1.close)
        self.signal_close.connect(self.plotter2.close)
        self.signal_close.connect(self.plotter3.close)

        self.frame.setLayout(hlayout)
        self.setCentralWidget(self.frame)


        self.mesh0 = OriginalMesh(self.plotter0)
        self.mesh1 = SmoothMesh(self.plotter1)
        self.mesh2 = ReMesh(self.plotter2)
        self.mesh3 = MeshFix(self.plotter3)

        self.mesh0.meshChanged.connect(self.mesh1.set_mesh)
        self.mesh1.meshChanged.connect(self.mesh2.set_mesh)
        self.mesh2.meshChanged.connect(self.mesh3.set_mesh)

        menubar = self.menuBar()
        file_menu = menubar.addMenu('File')

        save_action = QtWidgets.QAction('Save', self)
        save_action.triggered.connect(self.mesh3.save_mesh)
        file_menu.addAction(save_action)


        self.mesh0.set_mesh(mesh_flnm)

        self.plotter0.link_views_across_plotters(self.plotter1)
        self.plotter0.link_views_across_plotters(self.plotter2)
        self.plotter0.link_views_across_plotters(self.plotter3)
        
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
if __name__ == '__main__' :
    app = QtWidgets.QApplication(sys.argv)
    window = MyMainWindow(mesh_flnm = sys.argv[1])
    sys.exit(app.exec_())
