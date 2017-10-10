/*
 * main.cpp
 *
 *  Created on: Nov 5, 2015
 *      Author: Philipp Zajac
 *
 *	Server Program to capture up to 3 Webcams.
 *
 */

#include "NetworkListener.h"
#include <string.h>

using namespace std;
/*
 * argv[0] = port for Network connection
 */
int main(int argc, const char** argv) {
	cout << "Server Started" << endl;
	// initalizes Server Class
	NetworkListener *net;

	// if no port is given use standard port 60000
	if (argc > 1) {
		net = new NetworkListener((unsigned short int) atoi(argv[1]));
	} else {
		net = new NetworkListener();
	}
	net->udplisten();
}

