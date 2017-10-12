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


	this->m_renamed = false;
	// Config reader
	cfg.readFile("webcam_config.cfg");
	const Setting &root = cfg.getRoot();
	const Setting &webcam_settings = root["webcam_settings"];

	// Check if settings exist
	if (!(webcam_settings.lookupValue("count",m_webcamCount)
			&& webcam_settings.lookupValue("frame_height",m_frame_height)
			&& webcam_settings.lookupValue("frame_width",m_frame_width)
			&& webcam_settings.lookupValue("framerate",m_framerate))){
		string cerr = "ERROR: Setting not found in config file!";
		throw cerr;
	}

	// set capture state true
	if(m_webcamCount < 1){
		string err = "ERROR: webcamCount < 1";
		throw err;
	}
        
        // Check if there are more webcams connected to the computer than specified in the config file
        cout << "Checking available cameras" << endl;
        DIR *dpdf;
        struct dirent *epdf;
        int cams_found = 0;

        dpdf = opendir("/sys/class/video4linux/");
        if (dpdf != NULL){
            // Check how many cameras are found
            while (epdf = readdir(dpdf)) {
                string fname = epdf->d_name;
                string target = "video";
                size_t found = fname.find(target);

                if (found < fname.length()) {
                    cams_found++;
                    cout << "Found camera: " << fname << endl;
                }
            }
        } else {
            cout << "Problem opening /sys/class/video4linux/" << endl;
        }

        closedir(dpdf);

        if (cams_found < m_webcamCount) {
            cout << "Number of cameras connected less then specified in configuration file." << endl;
            cout << "Using only available cameras: " << cams_found << endl;
            m_webcamCount = cams_found;
        }

        if (cams_found > 0) {

	// Create Webcams
        } else {
            cout << "No cameras found" << endl;
        }

}

/*
 *  Destructor
 *  Delete Webcams for safe shutdown
 */
WebCamRunner::~WebCamRunner() {

}
/*
 * Helper function to execute system calls and return the output
 */
string WebCamRunner::execsysc(const char* cmd) {
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    return result;
}

/*
 * Open webcams
 */
void WebCamRunner::openWebcams(){
	cout << "Opening cameras" << endl;
        
        DIR *dpdf;
        struct dirent *epdf;
        int cams_found = 0;
	
        m_state = true;
	
        try {
		// Generate time stamp for directory_name and video files
		time_t timer;
		struct tm * timeinfo;
		time(&timer);
		timeinfo = localtime(&timer);

		// Generate directory_name
                char tstamp [15];
                strftime(tstamp,15,"%y%m%d-%H%M%S",timeinfo);

		directory_name = "./recordings";

		// create directory recordings if not existing
		if(mkdir(directory_name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)==-1);

		// create directory with time stamp
		directory_name = directory_name + "/" + tstamp;

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

                int open_cameras = 0;
                dpdf = opendir("/sys/class/video4linux/");
                if (dpdf != NULL){
                    // Check how many cameras are found
                    while (epdf = readdir(dpdf)) {
                        if (open_cameras < m_webcamCount) {
                            // Find cameras 
                            string fname = epdf->d_name;
                            string target = "video";
                            size_t found = fname.find(target);
                            
                            // Open camera
                            if (found < fname.length()) {
				// Disables the RightLight functionality for Logitech Webcams with v4l2-ctl driver
			        stringstream str;
                                str << "v4l2-ctl -d " << "/dev/" << fname << " -c exposure_auto_priority=0";
			        //devCount[i] = j;
		                cout << "Disabling RightLight" << endl;
				system(str.str().c_str());
		                
                                // Get actual system device id of the current camera
                                int cam_id = stoi(fname.substr(5));

                                // Create webcam object	
			        this->m_webcam[open_cameras] = new Webcam(cam_id, m_frame_height, m_frame_width, m_framerate, directory_name, tstamp);
			        m_mutex[open_cameras].unlock();
			        record[open_cameras] = true;

                                open_cameras++;
                                cout << "Opened camera: " << fname << endl;
                            }

                        }
                    }
		
                    m_mutex_start.unlock();
        

                    string sys_audiohw = execsysc("arecord -l | grep 'HD Pro Webcam C920'");
                    audio_device_id = stoi(sys_audiohw.substr(5,1));
                    cout << "Setting correct hardware id of audio device (" << audio_device_id << ")" << endl;
                
                } else {
                    cout << "Problem opening /sys/class/video4linux/" << endl;
                }

                closedir(dpdf);
		
	} catch (string &e) {
		throw;
	}

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
		cout << "Initializing Webcams" << endl;
		for (int i = 0; i < m_webcamCount; i++) {
			this->m_webcam[i]->getWebcamStream(&m_captuer_frame[i]);
		}

		// start video thread an wait until first frame is captured
		m_mutex_start.lock();
		for(int i=0;i<m_webcamCount;i++){
			 video_thread[i] = this->writeVideoThread(i);
		}

		cout << "Starting capturing of video (CAM " << m_webcamCount << ")" << endl;
		fps = m_webcam[0]->getFramerate();
		frame_count = 0;

		// start Audio capturing process with gstreamer
		pid_t pid = fork();
		if (pid == 0) {
			string arg = "location=./"+directory_name+"/audio.wav";
                        string hw = "device=hw:";
                        hw += audio_device_id;
                        hw += ",0";
                        const char* hw_c = hw.c_str();
                        cout << "Using audio hardware: " << hw << endl;
			char *argbuff = (char*)arg.c_str();

                        vector<string> gst_args = {"gst-launch", "alsasrc", hw_c ,"!" ,"audioconvert" ,"!", "audioresample" ,"!" ,"wavenc" ,"!", "filesink", argbuff, NULL};
                        const char **argv = new const char* [gst_args.size()+2];
                        for (int j=0; j < gst_args.size()+1; ++j) {
                            argv[j+1] = gst_args[j].c_str();
                        }

                        argv[gst_args.size()+1] = NULL;


			execv("/usr/bin/gst-launch", (char **)argv);
			cout << "execl for recording the audio stream failed\n";
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

		// shutdown audio process
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
			if(this->m_renamed==true){
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

			cout << "Renamed = " << this->m_renamed << endl;
			if(this->m_renamed==true){
				cout << "Renaming folder" << endl;
				int result =
						rename(this->getDirectoryName().c_str(),
								(this->getDirectoryName() + "_" + this->m_new_name).c_str());
				if (result != 0) {
					string err = "Error renaming file";
					throw err;
				}
			}
			cout << "Webcam Thread stopped" << endl;
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
	this->m_renamed = true;
}
