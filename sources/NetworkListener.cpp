/*
 * NetwrokListener.cpp
 *
 *  Created on: Dec 3, 2015
 *      Author: Philipp Zajac
 *
 */

#include "NetworkListener.h"

/*
 * start Webcamrunner
 */
void NetworkListener::startWebcam() {
	runner->startWebcamCapture();
}

/**
 * stop Webcamrunner
 */
void NetworkListener::stopWebcam() {
	runner->stopWebcamCapture();
}

/**
 * start thread function
 */
thread NetworkListener::startThread() {
	return std::thread([=] {this->startWebcam();});
}

/**
 * stop thread function
 */
thread NetworkListener::stopThread() {
	return std::thread([=] {this->stopWebcam();});
}

/*
 * Default constructor
 */
NetworkListener::NetworkListener() {
	m_port = 60000;
	m_buffer_size = 256;
	runner = NULL;
}

/*
 * Constructor for individual port
 */
NetworkListener::NetworkListener(unsigned short int port) {
	m_port = port;
	m_buffer_size = 256;
	runner = NULL;
}

/*
 * Logfile function
 */
void NetworkListener::log(const char *e){
	// Timestamp for logfile
	time_t timer;
	struct tm * timeinfo;
	time(&timer);
	timeinfo = localtime(&timer);

	// Create logfile directory if not exist
	string directory_name = "log";
	mkdir(directory_name.c_str(),
	S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	// Create and write Exception into logfile
	stringstream file_name;
	file_name << "log/" << asctime(timeinfo) << "logfile.txt";
	ofstream log_file(file_name.str(),
			std::ios_base::out | std::ios_base::app);
	log_file << e << endl;
	log_file.close();
}

/*
 * UPD Server function
 */
void NetworkListener::udplisten() {
	// Server address
	struct sockaddr_in serv_addr;
	// Client address
	struct sockaddr_in cli_addr;
	// Data buffer
	char buffer[m_buffer_size];
	string start = "start";
	string stop = "stop";
	// Socket vars
	int server_sock, rc, n;
	socklen_t server_length;
	socklen_t client_length;
	const int option_value = 1;

	// Thread vars
	thread start_thread;
	thread stop_thread;
	bool runs = false;

	// Open UDP Server Socket
	server_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_sock < 0) {
		perror("ERROR: opening socket");
		return;
	}

	// Get Server Dates
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(m_port);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Bind socket
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option_value,
			sizeof(int));
	server_length = sizeof(serv_addr);
	rc = ::bind(server_sock, (struct sockaddr *) &serv_addr, server_length);
	if (rc < 0) {
		perror("ERORR: on binding");
		return;
	}
	string folder_name = "<task-id>";

	// Listening Loop
	cout << "Listen for UDP Signal" << endl;
	while (true) {
		memset(buffer, 0, m_buffer_size);
		client_length = sizeof(cli_addr);
		n = recvfrom(server_sock, buffer, m_buffer_size, 0,
				(struct sockaddr *) &cli_addr, &client_length);
		if (n < 0) {
			cout << "No Data received" << endl;
			continue;
		}

		cout << "Received: " << buffer << endl;
		try {

			// If start command is received
			if (strcmp(buffer, start.c_str()) == 0 && runs == false) {
				runs = true;
				runner = new WebCamRunner();

				start_thread = this->startThread();
				cout << "Webcam Thread started" << endl;
			}

			// if top command is received
			else if (strcmp(buffer, stop.c_str()) == 0 && runs == true) {

				runs = false;
				stop_thread = this->stopThread();
				runner->setNewName(folder_name);
				stop_thread.join();
				start_thread.join();

			}

			// if no command is received set folder-name
			else {
				folder_name = buffer;
			}

		} catch(const FileIOException &fioex){
			std::cerr << "I/O error while reading file." << std::endl;
			this->log(fioex.what());
			break;
		} catch(const ParseException &pex) {
			std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
			<< " - " << pex.getError() << std::endl;
			this->log(pex.what());
			break;
		} catch(const SettingNotFoundException &nfex) {
			std::cerr << nfex.what() << endl;
			this->log(nfex.what());
			break;
		} catch (string &e) {
			std:cerr << e << endl;
			this->log(e.c_str());
			break;
		}

	}
	delete runner;
}

NetworkListener::~NetworkListener() {
	// --
}

