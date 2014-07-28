drishti
=======

Source code for Drishti.
added drishti.pro at toplevel.

Latest News

Latest source code available on https://github.com/AjayLimaye/drishti
Code on google code site will not be updated.
Windows, Mac and Linux executables for version 2.5 available for download from the releases section. 

27/July/2014
  Migration to Qt-5.2.1 and libQGLViewer-2.5.1

Drishti
  added : Save information for Drishti Prayog.  Drishti projects can now be viewed in Drishti Prayog.
  added : Image captions to allow for embedded text/images/web pages/movies in Drishti project.
  added : Keyframe title - used in Drishti Prayog.
  fixed : light generation for blend operations.
  Both upscaling as well as downscaling of volumes possible.
  Reposition captions and image captions using normalised screen coordinates.
  Simplified screen shadows (toggled by pressing "1").  Simplified Lighting Panel.

DrishtiImport
  fixed : bug in VGI (VG Studio Max) plugin.
  Handle NaN values for int and floats in raw, rawslabs and rawslices.

---------------------------------------------------------------------------

06/Dec/2013

DrishtiImport

    VGI importer (VG Studio MAX)
    fixed : tiff plugin bug which caused crash when importing large number of files. 

Drishti

    new lighting - ambient occlusion, multiple coloured lights, transfer functions as light source.
    reslice (realign/reorient) volumes faster.
    reslice, getvolume and getsurface area calculations now done on GPU.
    cross section images and cross section area using clip plane.
    viewport for clip plane
    viewport for path
    save small volume sections using reslice for clip plane.
    save straightened volumes using reslice for path. 

    meshpaint plugin for repainting meshes generated using mesh plugin. 

20/Jun/2013

    Windows, Mac and Linux executables for version 2.3.1 available for download from the Downloads section. 

DrishtiImport

    NIFTI importer
    NRRD importer 

Drishti

    Individual stereo settings for viewports
    saving camera FieldOfView value to project
    fixed : physical coordinates for plugins
    fixed : lighting 

15/May/2013

    Windows, Mac and Linux executables for version 2.3.1 available for download from the Downloads section. 

DrishtiPaint

    Slice-by-slice segmentation.
    Extraction of raw data by tag value.
    Visualize segmented data straight in Drishti Render. 

3/Apr/2013

    Windows, Mac and Linux executables for version 2.3 available for download from the Downloads section. 

DrishtiImport

    Fixed
        voxelsize in DICOM plugin 

Drishti

    New Additions
        Viewports for seamlessly switching between local and global view of data. Youtube playlist https://www.youtube.com/playlist?list=PLOdV4qS_2jOUwJg4K3Mc9TmRRyfkairuo&feature=view_all
        Continuous point addition for paths.
        New "tag" option (in addition to color and opacity) for "mix" command when viewing multiple volumes. When "tag" is specified, affected volume is tinged with tag colors. Volume 1 voxel value is used as tag value to choose appropriate tag color. This can be used to "paint" segmentation (in volume 1) on top of original volume (volume 0). 

25/Jan/2013

    Windows, Mac and Linux executables for version 2.2 available for download from the Downloads section. 

DrishtiImport

    New Additions
        Save data as 16-bit volume
        Allow saving of processed volume to a single raw file
        DICOM and MetaImage format plugin via ITK library
        Save output in MetaImage(.mhd) format 

Drishti

    New Additions
        16-bit volume visualization using 1D transfer functions
        Delete keyframes using selectRegion
        Allow saving of opacity/pruned/tagged volumes to single raw file
        Crosseye stereo for 3D TVs (press "5" to toggle)
        Fullscreen without menubar
        Blends
            Union for blend operations
            Magnify region using blends 
        Crops
            Mop crop
            Grid/box hatching implemented in crop
            Union for crop operations 
        Networks
            Load networks from text files 
        Clip Planes
            Textured slices using clip planes
            Mop clip
            Display grid, captions and images on clipplanes 
        Paths
            Addcamerapath using path as a guide
            Blend/carve operations possible with paths
            Mop patch, paint, paintpatch, carve, restore, set 
        MOPs
            Significant additions to mop operators 
            average, blend, carve, chessboard, cityblock,
                close, copy, copyfromsaved, copytosaved,
                dilate, dilateedge, edge, fusepatch, histogram,
                hatch, invert, itk, localmax, localthickness,
                max, maxvalue, min, nop, open, paint, rdilate,
                removepatch, sat0, sat1, setvalue, shrink,
                shrinkwrap, smoothchannel, swap, tag, thicken,
                update, xor 
                Mop itk - apply ITK filters to mask buffer
                Separate help for mops
                Mop operations from paths, crops and clip planes.
                Mop paint for first volume in multiple volumes 
        ITK Plugins
            Apply ITK filters to volume data (different from "mop itk")
            Signed distance map
            Binary thinning - skeletonization
            Connected component labelling
            Edge preserving smoothing
            Mean, median, binomial, discrete and recursive gaussian 
        Mesh Generation
            Handles mop channels
            Mesh for 16-bit data using opacity 

    Fixed
        Reslice volume using clip planes
        SetPruneTexture : RGBA -to- BGRA for texture format.
        Pruning fixed for mesh generation 

26/Oct/2012

    Windows executable for pre-alpha release of Drishti version 3.0 can be downloaded from the Downloads section. 

    There is a change in file format from version 2 to version 3. In ver 2, the volume was saved in raw format spread across multiple files. In ver 3, the data is stored in compressed blocked HDF5 format. 

    Ver 3 will support both raycasting and sliced based rendering modes, whereas, ver 2 only supports sliced based rendering. 

    Ver 3 will support visualization of very large volumes (~50GB). 

    Help video available on YouTube to give you a feel of the software https://www.youtube.com/playlist?list=PLOdV4qS_2jOWOlZO3yZOCSljVDOawRh1l&feature=view_all 

03/Sep/2012

    Windows and MacOSX executables can be downloaded from the Downloads section. 

    Mop : morphological operations on empty space skipping mask buffer. Rendering is limited to non-zero region in the mask. Mask buffer is created from drag volume. Various operations such as dilate, erode, shrinkwrap, cityblock, thickness, localmax and carve are supported. This should enable users to "skin" (remove top layer) to see inside, create fiducial, measure volume of empty space inside the sample, etc.
    Mesh : mesh generation plugin. Creates transfer function coloured triangular mesh.
    Paths : added ability to crop/blend/slice with paths.
    Captions : now display framenumber, volume number, interpolated numeric values and dial.
    GraphML : loaded for visualizing networks.
    Vectors : view, colour and filter vector data based on vector length.
    Scatter plots : view, colour and filter point data based on point size. 

    Metaimage : Metaimage file import/export implemented in the importer. 

    Fixed :
        hsv colour problem in colour dialog.
        rgb-to-bgr colour swap for loaded images.
        alpha-premultipled .png images now match screen image.
        incorrect image when saving with size different from screen size.
        incorrect shadows for images with size different from screen size.
        resetting of various settings when loading another data.
        incorrect image blurring when saved to disk.
        incorrect background transparency when rendering in offscreen buffer.
        croping in RGB volumes.
        Updating information from VolumeInformation panel on to respective .pvl.nc files.
        Disappearance of path length/angle text when axis were displayed.
        Shader error while displaying opacity within color map. 

Note Windows 64-bit version does not have the importer module. Please use drishtiimport.exe from the 32-bit version.

04/Oct/2011

    Scalebar : animated horizontal/vertical scalebars available.
    Grids : add grid using selected set of points (possible use in landmark generation).
    floatprecision to control number of significant digits displayed.
    Use non-native file dialog : users can now drag directories in the sidebar and use them as bookmarks
    indexed path
    loadpathgroup
    nobackgroundrender for batch rendering to save alpha channel.
    Ctrl+D for transfer function duplication.
    resetcamera
    Support for keyframes without saved lookup tables - useful when keyframes are generated outside of Drishti.
    Linear interpolation of colour/voxel values between two volumes when handling double volumes. 

    Fixed :
        copy-paste bug in keyframes.
        zero denominator in plygon intersection calculation.
        updating of .pvl.nc file via volumeinformationwidget optimized.
        various memory leaks
        position interpolation bug in user interface.
        ATI specific bugs 

08/April/2011

64-bit windows executable version now available for download.

    Crosseye stereo
    Interpolatevolumes possible when working on double volumes.
    Clearview using blend.
    Orthographic camera option.
    Cycle/wave repeat option for timeseries volumes.
    Enable/disablevolumeupdates
    Showangle option in paths.
    Countcells for counting isolated regions.
    Saveopacityvolume and maskrawvolume for RGB/A volumes.
    Improved point rendering - added : loadbarepoints, removebarepoints, pointsize, pointcolor, enablegrabpoints, disablegrabpoints
    Ctrl+c copy image to clipboard
    Added non-zero voxel count percentage when calculating volume. 

09/Feb/2011

Updated drishtiimport.exe available in Downloads section. Just replace the existing drishtiimport.exe.
This version will allow you do perform batch processing and isosurface (mesh) generation.
For batch processing :

    select multiple files while loading (much the same as input for time series volumes).
    the files need not be of same size (as needed for timeseries).
    can perform subsampling on every volume
    can perform "Set Slice 0 to top"
    data set trimming cannot be used.
    will set .pvl.nc filenames based on original volume file names, saving directory can be different. 

17/Jan/2011

    Undo/Redo facility now available (Ctrl+Z/Ctrl+Y)
    Inbuilt help for various dialogs and panels (Ctrl+H)
    modulate color and opacity for one volume based on other volumes (mix)
    color bar/legend
    rotated captions
    addrotationanimation 

25/Oct/2010

    Improved captions using paths.
    Search keyframes for a given text string. 

20/Oct/2010

    Implemented "showthicknessprofile" option for paths.
    Plugin version of DrishtiImport now part of source tree. The source code for this tool resides in tools/import directory and the plugin source codes are under tools/import/plugin/<plugin name> directory. Dynamic libraries are stored in bin/plugin directory. Users can create their own import plugins and copy into the bin/plugin directory.
    drishtibin-VS2008.zip removed from the downloads. Download drishtibin.zip instead. Unzip drishtibin.zip in a directory. The executables will be available in the bin directory within the unzipped directory. 

13/Sep/2010

    Save texsizereducefraction to project file. 

26/Aug/2010

    reslicevolume using a given clipplane. 

10/Aug/2010

    Anaglyph Stereo implemented : Red-Cyan and Red-Blue images
    Add clipplane given 3 points.
    savesliceimage for a given clipplane.
    reorientcamera in the direction of normal to a given clipplane.
    Fixed : bug in focal distance computation when using keyframe editor. 