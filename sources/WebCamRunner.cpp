/*
 * WebCamRunner.cpp
 *
 *  Created on: Nov 19, 2015
 *      Author: Philipp Zajac
 *
 */

#include "WebCamRunner.h"

/*
 * Constructor
 * Creates the Webcam classes and the directory
 * unlock mutex
 *
 */
WebCamRunner::WebCamRunner() {


	this->m_reanmed = false;
	// Config reader
	cfg.readFile("webcam_config.cfg");
	const Setting &root = cfg.getRoot();
	const Setting &webcam_settings = root["webcam_settings"];

	// Check if settings exist
	if (!(webcam_settings.lookupValue("count",m_webcamCount)
			&& webcam_settings.lookupValue("frame_hight",m_frame_height)
			&& webcam_settings.lookupValue("frame_width",m_frame_width)
			&& webcam_settings.lookupValue("framerate",m_framerate))){
		string cerr = "ERROR: Setting not found in config file!";
		throw cerr;
	}

	// print config
//	cout << "count=" << m_webcamCount << "\n"
//			<< "frame_hight=" << m_frame_height << "\n"
//			<< "frame_width=" << m_frame_width << "\n"
//			<< "framerate=" << m_framerate << "\n" << endl;

	// set capture state true
	if(m_webcamCount < 1){
		string err = "ERR:webcamCount < 1";
		throw err;
	}

	m_state = true;
	// Create Webcams
	cout << "Creat Webcams" << endl;
	try {
		// Generate time stamp for directory_name and video files
		time_t timer;
		struct tm * timeinfo;
		time(&timer);
		timeinfo = localtime(&timer);

		// Generate directory_name
		string tstamp = asctime(timeinfo);

		directory_name = "./recordings";

		// create directory recordings if not existing
		if(mkdir(directory_name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)==-1);

		// create driectroy with time stamp
		directory_name = "./recordings/" + tstamp + "";

		// Erase '\n' in timestamp
		directory_name.erase(
				remove(directory_name.begin(), directory_name.end(), '\n'),
				directory_name.end());

		// transform to lowercase letters
		std::transform(directory_name.begin(), directory_name.end(), directory_name.begin(), ::tolower);

		// Replace spaces with '_'
		std::replace(directory_name.begin(), directory_name.end(), ' ', '_');

		// create directory for Recordings
		mkdir(directory_name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		m_state = true;

		cout << "Disable RightLigth" << endl;

		//check witch devices are found. (Test the first 50 possibilities)
		int devCount[m_webcamCount];
		int j = 0;
		for (int i = 0; i < m_webcamCount; i++) {
			stringstream str;

			string dev = "/dev/video" + std::to_string(j);
			struct stat buffer;

			while(true){
				// stat() checks if device exist
				if(stat(dev.c_str(),&buffer) == -1){
					std:cerr << "ERROR: /dev/video" << j << " not found" <<endl;
					j++;
				}else{
					//cout << "/dev/video" << j << "found" << endl;
					str << "v4l2-ctl -d /dev/video" << j << " -c exposure_auto_priority=0";
					devCount[i] = j;
					// Disables the RightLight functionality for Logitech Webcams with v4l2-ctl driver
					system(str.str().c_str());
					j++;
					break;
				}
				if(j>=50){
					// Throw error if not all devices are found in /dev
					string err = "ERROR: Not all devices found \n";
					throw err;
				}
			}
		}

		// Create webcam classes and unlock mutex
		for (int i = 0; i < m_webcamCount; i++) {
			// i+1 if build in camera is not used in for example notebook
			this->m_webcam[i] = new Webcam(devCount[i], m_frame_height, m_frame_width, m_framerate, directory_name, tstamp);
			m_mutex[i].unlock();
			record[i] = true;
		}
		m_mutex_start.unlock();
	} catch (string &e) {
		throw;
	}
	// Unlock all mutexes

}

/*
 *  Destructor
 *  Delete Webcams for safe shutdown
 */
WebCamRunner::~WebCamRunner() {

}

/*
 * write captured frame to file
 */
void WebCamRunner::writeVideo(int i){
	m_mutex_start.lock();
	while(m_state){
		while(record[i]);
		m_mutex[i].lock();
		m_webcam[i]->setWebcamStream(&m_captuer_frame[i]);
		record[i] = true;
		m_mutex[i].unlock();
	}
}

// Video capture thread
thread WebCamRunner::writeVideoThread(int i){
	return thread([=] {writeVideo(i);});
}

// starts the Video recording
void WebCamRunner::startWebcamCapture() {
	// set capture state true
	m_state = true;

	int frame_count;
	int fps;

	try {
		// Initialize webcams
		cout << "Initialize Webcams" << endl;
		for (int i = 0; i < m_webcamCount; i++) {
			this->m_webcam[i]->getWebcamStream(&m_captuer_frame[i]);
		}

		// start video thread an wait until first frame is captured
		m_mutex_start.lock();
		for(int i=0;i<m_webcamCount;i++){
			 video_thread[i] = this->writeVideoThread(i);
		}

		cout << "start capture:" << m_webcamCount << endl;
		fps = m_webcam[0]->getFramerate();
		frame_count = 0;

		// start Audio capturing process with gstreamer
		pid_t pid = fork();
		if (pid == 0) {
			string arg = "location=./"+directory_name+"/audio.wav";
			char *argbuff = (char*)arg.c_str();
			char *argv[] = {"gst-launch", "alsasrc", "device=hw:1,0" ,"!" ,"audioconvert" ,"!", "audioresample" ,"!" ,"wavenc" ,"!", "filesink", argbuff, NULL};
			execv("/usr/bin/gst-launch", argv);
			cout << "execl failed";
			exit(EXIT_FAILURE);
		}

		// start the capturing loop
		do {

			for (int i = 0; i < m_webcamCount; i++) {
				m_mutex[i].lock();
				m_webcam[i]->getWebcamStream(&m_captuer_frame[i]);
				record[i] = false;
				m_mutex[i].unlock();
			}

			m_mutex_start.unlock();
			frame_count++;

		} while (m_state || ((frame_count % fps) != 0));

		// Wait for video_threads to stop
		for(int i=0;i<m_webcamCount;i++){
			m_mutex[i].lock();
			record[i] = false;
			m_mutex_start.unlock();
			video_thread[i].join();
		}

		// shutdown audio porcess
		kill(pid, SIGTERM);
		cout<<"end"<<endl;
		// Combine Sound and Video
		//avconv -i Webcam1.avi -i audio.wav -acodec copy -vcodec copy Webcam1withaudio.avi

		string webcam_name[m_webcamCount];
		for (int i = 0; i < m_webcamCount; i++) {
			webcam_name[i] = m_webcam[i]->getFilename();
			delete m_webcam[i];
		}

		for(int sec=0;sec<10;sec++){
			sleep(1);
			if(this->m_reanmed==true){
				break;
			}
		}

		pid_t pid2 = fork();

		if (pid2 == 0) {
			for (int i = 0; i < m_webcamCount; i++) {
				stringstream str;
				//cout << m_webcam[i]->getFilename() << endl;
				//cout << directory_name << endl;
				str << "avconv -i \"" << webcam_name[i] << "\" -i \"" << directory_name << "/audio.wav\"" << " -c:v libx264 -c:a aac -strict experimental \"./" << directory_name << "/WebcamFile" << i << ".mp4\"" ;
				//str << "avconv -i \"" << webcam_name[i] << "\" -i \"" << directory_name << "/audio.wav\"" << " -c:v copy -c:a copy \"" << directory_name << "/WebcamFile" << i << ".avi\"" ;
				cout << str.str() << endl;
				system(str.str().c_str());
			}
			// Remove temp files
			stringstream remove;
			remove << "rm -r \"" << directory_name << "\"/tmp*";
			system(remove.str().c_str());

			cout << "Renamed = " << this->m_reanmed << endl;
			if(this->m_reanmed==true){
				cout << "Rename folder" << endl;
				int result =
						rename(this->getDirectoryName().c_str(),
								(this->getDirectoryName() + "_" + this->m_new_name).c_str());
				if (result != 0) {
					string err = "Error renaming file";
					throw err;
				}
			}
			cout << "Webcam Thread stoped" << endl;
		}

	} catch (string &e) {
		throw;
	}
}

// stops the video capturing loop
void WebCamRunner::stopWebcamCapture() {
	m_state = false;
}

// returns the directoy name
string WebCamRunner::getDirectoryName() {
	return directory_name;
}

void WebCamRunner::setNewName(string name) {
	this->m_new_name = name;
	this->m_reanmed = true;
}
