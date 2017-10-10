/*
 * NetwrokListener.h
 *
 *  Created on: Dec 3, 2015
 *      Author: Philipp Zajac
 *
 *	UDP Network Listener
 *  Listens on given port (standard 60000) for <folder-name><command>.
 *  If 'start" is received as command, the program starts Webcam capturing.
 *  If "stop" is received as command, the program stops Webcam capturing.
 *  The folder-name can be set at start or stop of the program.
 *
 *  Up to 3 Webcams can be captured.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <thread>
#include <time.h>
#include <stdio.h>
#include <fstream>

#include "WebCamRunner.h"

#ifndef NETWROKLISTENER_H_
#define NETWROKLISTENER_H_

class NetworkListener {
private:
	unsigned short int m_port;
	int m_buffer_size;
	WebCamRunner *runner;

	// starts the Webcamrunner
	void startWebcam();

	// stops the Webcamrunner
	void stopWebcam();

	// Class threads functions
	thread startThread();
	thread stopThread();

	void log(const char *e);

public:
	NetworkListener();
	NetworkListener(unsigned short int port);
	virtual ~NetworkListener();

	// UPD Server function
	void udplisten();
};

#endif /* NETWROKLISTENER_H_ */
