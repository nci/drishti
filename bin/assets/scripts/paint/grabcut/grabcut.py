'''
===============================================================================
Interactive Image Segmentation using GrabCut algorithm.
This sample shows interactive image segmentation using grabcut algorithm.
USAGE:
    python grabcut.py <filename>
README FIRST:
    Two windows will show up, one for input and one for output.
    At first, in input window, draw a rectangle around the object using
mouse right button. Then press 'n' to segment the object (once or a few times)
For any finer touch-ups, you can press any of the keys below and draw lines on
the areas you want. Then again press 'n' for updating the output.
Key '0' - To select areas of sure background
Key '1' - To select areas of sure foreground
Key '2' - To select areas of probable background
Key '3' - To select areas of probable foreground
Key 'n' - To update the segmentation
Key 'r' - To reset the setup
Key 's' - To save the results
===============================================================================
'''

# Python 2/3 compatibility
import paintmod
import numpy as np
import cv2
import sys


BLUE = [255,0,0]        # rectangle color
RED = [0,0,255]         # PR BG
GREEN = [0,255,0]       # PR FG
BLACK = [0,0,0]         # sure BG
WHITE = [255,255,255]   # sure FG

DRAW_BG = {'color' : BLACK, 'val' : 0}
DRAW_FG = {'color' : WHITE, 'val' : 1}
DRAW_PR_FG = {'color' : GREEN, 'val' : 3}
DRAW_PR_BG = {'color' : RED, 'val' : 2}

# setting up flags
rect = (0,0,1,1)
drawing = False         # flag for drawing curves
rectangle = False       # flag for drawing rect
rect_over = False       # flag to check if rect drawn
rect_or_mask = 100      # flag for selecting rect or mask mode
value = DRAW_FG         # drawing initialized to FG
thickness = 1           # brush thickness

labelValue = 1
sliceNum = 0
prevSlice = -1
img = cv2.cvtColor(np.zeros((100,100), np.uint8), cv2.COLOR_GRAY2BGR)
img2 = cv2.cvtColor(np.zeros((100,100), np.uint8), cv2.COLOR_GRAY2BGR)
mask = np.zeros((100,100), np.uint8)
output = np.zeros((100,100), np.uint8)
ix = 0
iy = 0
dim = np.zeros(3,np.int32)


UINT8 = 1
MAXVALUE = 255
DTYPE = np.uint8


class paint_data :
    def __init__(self) :
        self.paint_obj = 0
        self.volume = 0
        self.mask = 0
        self.lut = 0
        self.depth = 0
        self.width = 0
        self.height = 0

print('paint_data declared')
pd = paint_data()


def set_paint_data(py_obj) :
    pd.paint_obj = py_obj
    pd.volume = py_obj.get_volume_view()
    pd.mask = py_obj.get_mask_view()
    pd.lut = py_obj.get_lut_view()
    pd.depth = py_obj.depth
    pd.width = py_obj.width
    pd.height = py_obj.height
    dim[0] = pd.depth
    dim[1] = pd.width
    dim[2] = pd.height
    print(pd.depth*pd.width*pd.height)
    print(pd.depth, pd.width, pd.height)
    print(pd.volume.shape)
    
def init() :
    print('init grabcut')
    pd.volume = pd.volume.reshape(dim)  

    
def process_volume() :
    grabCut()
    


def onmouse(event,x,y,flags,param):
    global img,img2,drawing,value,mask,rectangle,rect,rect_or_mask,ix,iy,rect_over

    # Draw Rectangle
    if event == cv2.EVENT_RBUTTONDOWN:
        rectangle = True
        ix,iy = x,y

    elif event == cv2.EVENT_MOUSEMOVE:
        if rectangle == True:
            img = img2.copy()
            cv2.rectangle(img,(ix,iy),(x,y),BLUE,1)
            rect = (min(ix,x),min(iy,y),abs(ix-x),abs(iy-y))
            rect_or_mask = 0

    elif event == cv2.EVENT_RBUTTONUP:
        rectangle = False
        rect_over = True
        cv2.rectangle(img,(ix,iy),(x,y),BLUE,1)
        rect = (min(ix,x),min(iy,y),abs(ix-x),abs(iy-y))
        rect_or_mask = 0
        print(" Now press the key 'n' a few times until no further change \n")
        sys.stdout.flush();
        
    # draw touchup curves

    if event == cv2.EVENT_LBUTTONDOWN:
        if rect_over == False:
            print("first draw rectangle \n")
        else:
            drawing = True
            cv2.circle(img,(x,y),thickness,value['color'],-1)
            cv2.circle(mask,(x,y),thickness,value['val'],-1)

    elif event == cv2.EVENT_MOUSEMOVE:
        if drawing == True:
            cv2.circle(img,(x,y),thickness,value['color'],-1)
            cv2.circle(mask,(x,y),thickness,value['val'],-1)

    elif event == cv2.EVENT_LBUTTONUP:
        if drawing == True:
            drawing = False
            cv2.circle(img,(x,y),thickness,value['color'],-1)
            cv2.circle(mask,(x,y),thickness,value['val'],-1)

            
def on_press(k) :
    global img,img2,drawing,value,mask,rectangle,rect,rect_or_mask,ix,iy,rect_over,imgdata

    if k == 27: # esc to exit
        exit()
    elif k == ord('0'): # BG drawing
        print(" mark background regions with left mouse button \n")
        value = DRAW_BG
    elif k == ord('1'): # FG drawing
        print(" mark foreground regions with left mouse button \n")
        value = DRAW_FG
    elif k == ord('2'): # PR_BG drawing
        value = DRAW_PR_BG
    elif k == ord('3'): # PR_FG drawing
        value = DRAW_PR_FG
    elif k == ord('s'): # save image
        mask2 = np.where((mask==1) + (mask==3),labelValue,0).astype('uint16')
        print(mask2.shape)
        slc_size = dim[1]*dim[2]
        slc_start = sliceNum * slc_size
        pd.mask[slc_start:slc_start+slc_size] = np.reshape(mask2,-1)
        pd.paint_obj.update_slice_view()
        pd.paint_obj.update_3d_view()
        print('processed slice')
    elif k == ord('r'): # reset everything
        print("resetting \n")
        rect = (0,0,1,1)
        drawing = False
        rectangle = False
        rect_or_mask = 100
        rect_over = False
        value = DRAW_FG
        img = img2.copy()
        mask = np.zeros(img.shape[:2],dtype = np.uint8) # mask initialized to PR_BG
        output = np.zeros(img.shape,np.uint8)           # output image to be shown
    elif k == ord('n'): # segment the image
        print(""" For finer touchups, mark foreground and background after pressing keys 0-3
        and again press 'n' \n""")
        if (rect_or_mask == 0):         # grabcut with rect
            bgdmodel = np.zeros((1,65),np.float64)
            fgdmodel = np.zeros((1,65),np.float64)
            cv2.grabCut(img2,mask,rect,bgdmodel,fgdmodel,1,cv2.GC_INIT_WITH_RECT)
            rect_or_mask = 1
        elif rect_or_mask == 1:         # grabcut with mask
            bgdmodel = np.zeros((1,65),np.float64)
            fgdmodel = np.zeros((1,65),np.float64)
            cv2.grabCut(img2,mask,rect,bgdmodel,fgdmodel,1,cv2.GC_INIT_WITH_MASK)
    sys.stdout.flush();

    
def on_change(val):
    global sliceNum, img, img2
    sliceNum = val
    img = cv2.resize(pd.volume[sliceNum,:,:], (dim[1],dim[2]))
    img = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)
    img2 = img.copy()                               # a copy of original image
    
    
def grabCut() :
    global img,img2,mask,output,drawing,value,rectangle,rect,rect_or_mask,ix,iy,rect_over,imgdata,sliceNum,prevSlice

    img = cv2.resize(pd.volume[sliceNum,:,:], (dim[1],dim[2]))
    img = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)
    img2 = img.copy()                               # a copy of original image
    mask = np.zeros(img.shape[:2],dtype = np.uint8) # mask initialized to PR_BG
    output = np.zeros(img.shape,np.uint8)           # output image to be shown

    # input and output windows
    cv2.namedWindow('output', cv2.WINDOW_KEEPRATIO)
    cv2.namedWindow('input', cv2.WINDOW_KEEPRATIO)

    h, w = img.shape[0:2]
    neww = 800
    newh = int(neww*(h/w))
    cv2.resizeWindow('input', neww, newh)
    cv2.resizeWindow('output', neww, newh)

    cv2.setMouseCallback('input',onmouse)
    cv2.moveWindow('input',img.shape[1]+10,90)

    cv2.createTrackbar('slider', 'input', 0, dim[0]-1, on_change)

    print(" Instructions: \n")
    print(" Draw a rectangle around the object using right mouse button \n")
    sys.stdout.flush();

    
    while(1):
        cv2.imshow('output',output)
        cv2.imshow('input',img)
                    
        imageCopy = img.copy()
        cv2.putText(imageCopy, str(sliceNum), (0, imageCopy.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (255, 0, 255), 4)
        cv2.imshow('input', imageCopy)

        k = cv2.waitKey(1)
        on_press(k) 

        mask2 = np.where((mask==1) + (mask==3),255,0).astype('uint8')
        output = cv2.bitwise_and(img2,img2,mask=mask2)
        output[:,:,1] = mask2

    cv2.destroyAllWindows()


if __name__ == '__main__':
    # print documentation
    print(__doc__)
