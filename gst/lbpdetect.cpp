#include <stdio.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <cv.h>                 // includes OpenCV definitions

#include "lbpdetect.h"

#define LBP_FILENAME "/usr/share/opencv/lbpcascades/lbpcascade_frontalface.xml"


#ifdef __cplusplus
extern "C" {
#endif

using namespace cv;

CascadeClassifier lbp_classifier;


int initd=0;
int lbp_init() {
	fprintf(stderr,"RUN DETECT - INIT\n");
	lbp_classifier.load( LBP_FILENAME );
	initd=1;
	return 0;
}

CvSeq * lbp_detectMultiScale(IplImage * frame_gray, IplImage * orig) {
	fprintf(stderr,"RUN DETECT\n");
	if (initd==0) {
		lbp_init();
	}
	std::vector<Rect> faces;
	lbp_classifier.detectMultiScale( frame_gray, faces, 1.4, 2, 0, Size(30, 30) );
	for( size_t i = 0; i < faces.size(); i++ ) {
		fprintf(stderr,"RUN DETECT - FACE\n");
		Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
		cvEllipse (orig, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0.0, 0.0, 360.0, CV_RGB (255, 0, 255),3, 8, 0);
		//ellipse( frame_gray, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );
	}
	return NULL;

}

#ifdef __cplusplus
}
#endif
