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
class SimplifyMesh(QtCore.QObject) :
    meshChanged = QtCore.pyqtSignal(str, pyvista.core.pointset.PolyData)
    
    #--------------------
    def __init__(self, plotter) :
        super(SimplifyMesh, self).__init__(None)
        self.mesh_flnm = 0
        self.plotter = plotter

        self.mesh_actor = 0

        self.chkbox = self.plotter.add_checkbox_button_widget(self.toggle_edges, value=True, color_on='g')
        self.plotter.add_text('Edges', position = (70, 10), name='chkbox')
        
        self.reduce = 0
        self.reduce = self.plotter.add_slider_widget(self.cluster_and_remesh,
                                                     rng=[1, 100],
                                                     value=20,
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
            'Save Simplified Surface',
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
        self.mesh1 = SimplifyMesh(self.plotter1)

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
