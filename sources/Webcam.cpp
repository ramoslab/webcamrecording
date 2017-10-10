/*
 * WebcamTest.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: Philipp Zajac
 *
 *  Webcam recording with OPENCV for more info:
 *  http://docs.opencv.org/2.4/modules/highgui/doc/reading_and_writing_images_and_video.html
 */

#include "Webcam.h"

using namespace std;

/*****
 * Constructor
 * Initialize capture and video object
 * Initialize frame height and width
 * Generate filename with time stamp
 * @param: cam_nr Device number
 * @param: frame_height Frame pixel hight
 * @param: frame_width Frame pixel width
 * @param: framerate
 * @param: dirctory_name
 * @param: tstamp Start time of the capturing (used for debbuging)
 */
Webcam::Webcam(int cam_nr, int frame_height, int frame_width, int framerate, string dirctory_name, string tstamp) {

	// Video frame width and height 1280x720
	m_frame_width = frame_width;
	m_frame_height = frame_height;
	// Video framerate 24
	m_framerate = framerate;
	// Video Codec
	m_codec = CV_FOURCC('M', 'J', 'P', 'G');
	// Webcam number 0 default 1-3
	m_cam_nr = cam_nr;

	stringstream file_name_ss;
	string file_name;

	file_name_ss << dirctory_name << "/" << "tmpWebcam" << m_cam_nr
			<< ".avi";

	file_name = file_name_ss.str();

//	cout <<file_name<<endl;
//	cout <<dirctory_name<<endl;

	m_file_name = file_name;
	try {
		// Create VideoCapture
		this->createVideoCapturer();
		// Create VideoWriter
		this->createVideoWriter();
	} catch (string &e) {
		throw;
	}
}

/*****
 * Destructor
 * Release capture and video object.
 */
Webcam::~Webcam() {
	m_capture.release();
	m_video.release();
}

// Create VideoCapture
void Webcam::createVideoCapturer() {
	//Initialize capture object with cam_nr and check if it was successful

	VideoCapture capture(m_cam_nr);
	if (!capture.isOpened()) {
		string err = "ERROR: Webcam not found : CAM" + std::to_string(m_cam_nr);
		throw err;
	}

	m_capture = capture;
	// Set Webcam settings
	m_capture.set(CV_CAP_PROP_FRAME_WIDTH, m_frame_width);
	m_capture.set(CV_CAP_PROP_FRAME_HEIGHT, m_frame_height);
	m_capture.set(CV_CAP_PROP_FPS, m_framerate);
	//cout << m_framerate << endl;
}

// Create VideoWriter
void Webcam::createVideoWriter() {

	//Initialize video object and check if it was successful
	// Write in .avi file
	VideoWriter video(m_file_name, m_codec, m_framerate,
			Size(m_frame_width, m_frame_height), true);
	if (!video.isOpened()) {
		string err = "ERROR: Video file can,t be generated for CAM"
				+ std::to_string(m_cam_nr);
		throw err;
	}
	m_video = video;
}

/*****
 * capture frame
 */
bool Webcam::getWebcamStream(Mat *frame) {
	if (!m_capture.isOpened()) {
		string err = "ERROR: Webcam not found during capture : CAM"
				+ std::to_string(m_cam_nr);
		throw err;
	}
	m_capture.read(*frame);
	return frame;
}

/*****
 * write frame
 */
void Webcam::setWebcamStream(Mat *frame) {
	if (!m_video.isOpened()) {
		string err = "ERROR: Video file not found CAM"
				+ std::to_string(m_cam_nr);
		throw err;
	}
	m_video.write(*frame);
}

int Webcam::getFramerate() const {
	return m_framerate;
}

string Webcam::getFilename(){
	return m_file_name;
}
