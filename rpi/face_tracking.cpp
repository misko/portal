#include <omp.h> 
#include "opencv2/objdetect/objdetect.hpp"
 #include "opencv2/highgui/highgui.hpp"
 #include "opencv2/imgproc/imgproc.hpp"

 #include <iostream>
 #include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>


#define M1PIN	16
#define M2PIN	20
#define ENPIN	19


void move(int x, int d) {
	digitalWrite(ENPIN,0);	 //disable
	digitalWrite(M1PIN,x);
	digitalWrite(M2PIN,1-x);
	digitalWrite(ENPIN,1);	
	delay(d);
	digitalWrite(ENPIN,0);	 //disable
}

 using namespace std;
 using namespace cv;

 /** Function Headers */
 void detectAndDisplay( Mat &frame , Mat &frame_gray);
void capture_frame(Mat &frame, Mat &frame_gray,  CvCapture * capture);

 CascadeClassifier face_cascade;
 CascadeClassifier profile_cascade;
 CascadeClassifier eyes_cascade;
 string window_name = "Capture - Face detection";
 RNG rng(12345);

 int width,height;

void rotate(cv::Mat& src, double angle, cv::Mat& dst, cv::Mat& invert) {
    int len = std::max(src.cols, src.rows);
    cv::Point2f pt(len/2., len/2.);
    cv::Mat r = cv::getRotationMatrix2D(pt, angle, 1.0);

    cv::warpAffine(src, dst, r, cv::Size(len, len));
   
    invertAffineTransform(r,invert);
}

Point RotateBackPoint(const Point& dstPoint, const Mat& invertMat)
{
    cv::Point orgPoint;
    orgPoint.x = invertMat.at<double>(0,0)*dstPoint.x + invertMat.at<double>(0,1)*dstPoint.y + invertMat.at<double>(0,2);
    orgPoint.y = invertMat.at<double>(1,0)*dstPoint.x + invertMat.at<double>(1,1)*dstPoint.y + invertMat.at<double>(1,2);
    return orgPoint;
}

 /** @function main */
 int main( int argc, const char** argv )
 {
   width = atoi(argv[1]);
   height = atoi(argv[2]);
   /** Global variables */
   //String face_cascade_name = "/home/pi/opencv-2.4.10/data/haarcascades/haarcascade_frontalface_default.xml";
   String face_cascade_name = String(argv[3]);
   //String face_cascade_name = "/home/pi/opencv-2.4.10/data/haarcascades/haarcascade_frontalface_alt_tree.xml";
   //String face_cascade_name = "/home/pi/opencv-2.4.10/data/lbpcascades/lbpcascade_frontalface.xml";
   //String profile_cascade_name = "/home/pi/opencv-2.4.10/data/haarcascades/haarcascade_profileface.xml";
   //String profile_cascade_name = "/home/pi/opencv-2.4.10/data/lbpcascades/lbpcascade_profileface.xml";
   String profile_cascade_name = String(argv[4]);
   //String eyes_cascade_name = "/home/pi/opencv-2.4.10/data/haarcascades/haarcascade_mcs_mouth.xml";


   CvCapture* capture;
   Mat frame;

   //-- 1. Load the cascades
   if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
   if( !profile_cascade.load( profile_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
   //if( !eyes_cascade.load( eyes_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };

   //-- 2. Read the video stream
   capture = cvCaptureFromCAM( -1 );
   cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH,width); // width of viewport of camera
   cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, height); // height of ...


if (!capture) {
	fprintf(stderr,"MAJOR ERROR\n");	
	exit(1);
}


        wiringPiSetupGpio();
        //lets setup enable pin
        pinMode(M1PIN,OUTPUT);
        pinMode(M2PIN,OUTPUT);
        pinMode(ENPIN,OUTPUT);
        digitalWrite(ENPIN,0); //Turn the motor off

/*omp_lock_t frame_lock[2];
omp_lock_t capture_lock[2];

omp_init_lock(&frame_lock[0]);
omp_init_lock(&frame_lock[1]);

int tid,current_frame;
omp_set_num_threads(2);
#pragma omp parallel private(tid)
{
	current_frame=0;
	if (tid==0) {
		while (true) {
				
		}
	} else {
		while (true) {
			
		}
	}
tid = omp_get_thread_num();
	fprintf(stderr,"I am %d\n",tid);
}
exit(1);*/

   Mat frame_gray;
   if( capture )
   {
     while( true )
     {
       //frame = cvQueryFrame( capture );
       capture_frame(frame,frame_gray, capture);

   //-- 3. Apply the classifier to the frame
       if( !frame.empty() )
       { detectAndDisplay( frame, frame_gray ); }
       //{ detectAndDisplay( frame ); }
       else
       { printf(" --(!) No captured frame -- Break!"); break; }

       int c = waitKey(10);
       if( (char)c == 'c' ) { break; }
      }
   }
   return 0;
 }

void capture_frame(Mat &frame, Mat &frame_gray,  CvCapture * capture) {
  frame = cvQueryFrame( capture );
  cvtColor( frame, frame_gray, CV_BGR2GRAY );
  equalizeHist( frame_gray, frame_gray );
}

int i=0; 
int j=0;
int k=0;
/** @function detectAndDisplay */
void detectAndDisplay( Mat &frame , Mat &frame_gray) {
  std::vector<Rect> faces;
  /*Mat frame_gray;

  cvtColor( frame, frame_gray, CV_BGR2GRAY );
  equalizeHist( frame_gray, frame_gray );*/

  float avg_x=0,n_x=0; 
 
  if (i++%10!=0) {
	  //-- Detect faces
	  //face_cascade.detectMultiScale( frame_gray, faces, 1.3, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_DO_ROUGH_SEARCH|CV_HAAR_DO_CANNY_PRUNING, Size(30, 30) );
	  float c = 1.0;
	  Mat invert;
	  switch (k++%8) {
	  case 0:
	  case 4:
		rotate(frame_gray,0,frame_gray,invert);
		break;
	  case 1:
          case 5:
		c = 0.5;
		rotate(frame_gray,-25,frame_gray,invert);
		break;
	  case 2:
	  case 6:
 		c = 0.25;
		rotate(frame_gray,+25,frame_gray,invert);
		break;
	  case 3:
 		c = 0.1;
		rotate(frame_gray,-45,frame_gray,invert);
		break;
	  case 7:
 		c = 0.6;
		rotate(frame_gray,+45,frame_gray,invert);
		break;
          }
	  face_cascade.detectMultiScale( frame_gray, faces, 1.3, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_DO_ROUGH_SEARCH|CV_HAAR_DO_CANNY_PRUNING, Size(30, 30) );
	  //face_cascade.detectMultiScale( frame_gray, faces, 2, 2, 0, Size(30, 30) );


 
	  for( size_t i = 0; i < faces.size(); i++ ) {
	    Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
	    center=RotateBackPoint(center,invert);
	    avg_x+=center.x;
	    n_x+=1;
	    ellipse( frame, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 0, c, 255 ), 4, 8, 0 );
	  } 
  } else {
	  //-- Detect faces
	  //profile_cascade.detectMultiScale( frame_gray, faces, 1.2, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_DO_ROUGH_SEARCH|CV_HAAR_DO_CANNY_PRUNING, Size(30, 30) );

	  bool flipped=false;
  	  if (j++%2==0) {
		flip(frame_gray,frame_gray,1);
 		flipped=true;
          }
	  profile_cascade.detectMultiScale( frame_gray, faces, 1.3, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT|CV_HAAR_DO_ROUGH_SEARCH|CV_HAAR_DO_CANNY_PRUNING, Size(30, 30) );
	  
	  //profile_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0, Size(30, 30) );
	  //face_cascade.detectMultiScale( frame_gray, faces, 2, 2, 0, Size(30, 30) );
	  for( size_t i = 0; i < faces.size(); i++ ) {
	    if (flipped) {
	     faces[i].x=height-faces[i].x;
	    }
	    Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
	    avg_x+=center.x;
	    n_x+=1;
	    ellipse( frame, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );
	  }
  

  }
  if (n_x>0) {
	  avg_x/=n_x;
	 fprintf(stderr,"%f %d %f\n",avg_x, height,abs(avg_x-(height/2)));
	  if (avg_x>0 && abs(avg_x-(height/2))>30) {
		if (avg_x>height/2) {
			move(0,50);
		} else {
			move(1,50);
		}
	  }
  }
  //-- Show what you got
  //imshow( window_name, frame );
  cvNamedWindow("Name", CV_WINDOW_NORMAL);
  //cvSetWindowProperty("Name", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
  imshow( "Name", frame );
  //imshow( "Name", frame_gray );
  //cvShowImage("Name", frame);
 }

