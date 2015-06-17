/*****************************************************************   
* This program uses function histogram() in Ch
*****************************************************************/
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>

using namespace cv;

int main(int argc, char* argv[])
{


    CvCapture * capture = cvCaptureFromCAM(-1);
    CvMemStorage* storage = cvCreateMemStorage(0);

    CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*)cvLoad( "/home/pi/opencv-2.4.10/data/haarcascades/haarcascade_frontalface_default.xml" );
    double scale = 1.3;

    static CvScalar colors[] = { {{0,0,255}}, {{0,128,255}}, {{0,255,255}}, 
    {{0,255,0}}, {{255,128,0}}, {{255,255,0}}, {{255,0,0}}, {{255,0,255}} };

    // Detect objects
    Mat img, img_gray;
    while (true) {
            img = cvQueryFrame( capture );
	    img_gray = cvCreateImage (cvGetSize(img), IPL_DEPTH_8U, 1);
	    cvClearMemStorage( storage );
	    CvSeq* objects = cvHaarDetectObjects( img, cascade, storage, 1.1, 4, 0, cvSize( 40, 50 ));

	    CvRect* r;
	    // Loop through objects and draw boxes
	    for( int i = 0; i < (objects ? objects->total : 0 ); i++ ){
		r = ( CvRect* )cvGetSeqElem( objects, i );
		cvRectangle( img, cvPoint( r->x, r->y ), cvPoint( r->x + r->width, r->y + r->height ),
		    colors[i%8]);
	    }

	    imShow( "Output", img );
	    cvWaitKey(200);
    }


    return 0;
}
