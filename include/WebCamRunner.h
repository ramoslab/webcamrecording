/*
 * WebCamRunner.h
 *
 *  Created on: Nov 19, 2015
 *      Author: Philipp Zajac
 *
 * Handles the Webcam streaming
 *
 */

#include "Webcam.h"
#include <thread>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sstream>
#include <libconfig.h++>
#include <dirent.h>

using namespace libconfig;

#ifndef WEBCAMRUNNER_H_
#define WEBCAMRUNNER_H_

class WebCamRunner {
private:
	// record state
	bool m_state;

	// WebCamRunner state
	bool runner_state;

	// Count of webcam captured 1-3
	int m_webcamCount;
	int m_frame_width;
	int m_frame_height;
	int m_framerate;
        int audio_device_id;

	static const int max_webcam_count = 3;
	// Webcam objects
	Webcam *m_webcam[max_webcam_count];
	// Capture frames
	Mat m_captuer_frame[max_webcam_count];

	// Video thread obejct
	thread video_thread[max_webcam_count];

	// Synchronization mutexes for frame capturing
	mutex m_mutex[max_webcam_count];
	mutex m_mutex_start;
	bool record[max_webcam_count];

	// File location
	string directory_name;

	// Config class
	Config cfg;

	//File Renaming
	bool m_renamed;
	string m_new_name;

        //Helper function for system calls
        string execsysc(const char*);  

	// Class thread function
	thread writeVideoThread(int i);

	// write MAT to file
	void writeVideo(int i);

public:
	WebCamRunner();
	virtual ~WebCamRunner();

        // Open the webcams
        void openWebcams();

	// Start the webcam capturing
	void startWebcamCapture();
	// Stops the webcam capturing
	void stopWebcamCapture();

	// Returns the directory name
	string getDirectoryName();

	// Sets the new name for the the file path.
	void setNewName (string name);

	// Returns the runner state
	bool getRunnerState();
};

#endif /* WEBCAMRUNNER_H_ */
