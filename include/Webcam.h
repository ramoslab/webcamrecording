/*
 * WebcamTest.h
 *
 *  Created on: Oct 15, 2015
 *      Author: Philipp Zajac
 *
 *  Webcam recording with OPENCV for more info:
 *  http://docs.opencv.org/2.4/modules/highgui/doc/reading_and_writing_images_and_video.html
 */
#include <opencv2/opencv.hpp>
#include <time.h>

using namespace std;
using namespace cv;

#ifndef WEBCAM_H_
#define WEBCAM_H_

class Webcam {
private:
	// Opencv capture and writer objects
	VideoCapture m_capture;
	VideoWriter m_video;

	// Video frame width and height
	int m_frame_width;
	int m_frame_height;
	// Video framerate
	int m_framerate;
	// Webcam number
	int m_cam_nr;
	// Capture codec
	int m_codec;

	// Name of video file
	string m_file_name;

	void createVideoWriter();
	void createVideoCapturer();

public:
	Webcam(int cam_nr, int frame_height, int frame_width, int framerate, string dirctory_name, string tstamp);
	virtual ~Webcam();

	bool getWebcamStream(Mat *frame);
	void setWebcamStream(Mat *frame);
	int getFramerate() const;
	string getFilename();
};

#endif /* WEBCAMTEST_H_ */
