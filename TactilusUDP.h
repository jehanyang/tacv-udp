#pragma once

#include <iostream>
#include "tactilus.h"
#include <cstdio>
#include <string>
#include <vector>

#include<winsock2.h>
#include<Ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 16000
#define FORCEBUFLEN 32

// Author:	Jehan Yang

namespace tactilus_udp {
	// C++ class to process Tautilus data and to send over UDP
	class TactilusUDP
	{


	public:

		//	Initialize everything as needed for sending Tactilus force data over UDP
		TactilusUDP(const char* dest_address, u_int src_port, u_int dest_port);
		//	char* dest_address			IPv4 address destination written as a string
		//	u_int src_port				local port that we are sending from
		//	u_int dest_port				remote port that we are sending to

		TactilusUDP(const char* dest_address, u_int src_port, u_int dest_port, u_int communicator);
		//	Initialize everything as needed
		/*	char* dest_address			IPv4 address destination written as a string (irrelevant if not communicator)
		//	u_int src_port				local port that we are sending from (irrelevant if not communicator)
		//	u_int dest_port				remote port that we are sending to (irrelevant if not communicator)
		//  u_int communicator          1 if this sensor is the commmunicator (the sensor object sending and receiving messages) and 0 if it is not
		*/

		~TactilusUDP();
		//	Clean up Tactilus class, not really sure when needed

		Tactilus* gettactilusid();
		//  Gets Tactilus id

		void send(std::string message);
		//	Takes a message and sends it to dest_address with src_port and dest_port as initialized
		
		void recv();
		//	Checks whether anything is in to be received to our address and src_port, writes to buf

		void update();
		//  Update the ringbuf with pressure readings
		
		char* getbuf();
		//	Returns pointer to first index of buffer of message received

		float* getpresbuftosend();
		//  Returns presbuftosend

		void updatepresbuftosend();
		//  Update presbuftosend

		std::string allpressurepads();
		//	Return string with all pressure readings in an array in [x,y] = P format [kPa]

		double estimateForce();
		//  Estimate force by multiplying areas with pressure [N]

		double* estimateCoP();
		//  Estimate location of center of pressure [mm]

		double estimateMoment_y (double x);
		//  Estimate moment at specified location x1: x_max should be 270 mm [Nm]

		double* estimateForceAndMoment_y(double x);
		//  Estimates moment and force, will be faster than independently sending both

		double* estimateForceAndMoment_yx(double x1, double y1);
		//  Estimates moment about two axes and force in z direction

		std::vector<double> estimateForceAndMoment_yx_somepadforces(double x1, double y1, u_int* padx, u_int* pady, u_int padnumber);
		//  Estimates moment about two axes, total force in z direction, and force from requested pads

		std::vector<double> TactilusUDP::estimateForceAndMoment_yx_frontbackforces(double x1, double y1);
		//  Estimates moment about two axes, total force in z direction, and force of front 64 pads and force of back 64 pads



	private:
		struct sockaddr_in si_other, srcaddr;
		int s, slen = sizeof(si_other); // s is number of socket that is initialized in constructor
		char buf[BUFLEN];
		WSADATA wsa;
		Tactilus* t;
		unsigned int rows, cols;

		float presbuftosend[128] = { 0 }; // this is what to send when asked for it. This may allow for sending repeated data
		float ringbuf[FORCEBUFLEN][128] = { 0 }; // this stores pressures in kPa
		int ringbufwritehead;

		// Note, in the reference frame, we define x as along the columns and y as along the rows
		// the origin is in the top left. x increases as we go up, y increases as we go left, which
		// coincides with the rows and columns indices decreasing.
		double areas[16][8] = {{   0,   0,   0,   0, 2/3,   1, 1/2,   0},
							   {   0,   0, 1/2,   1,   1,   1,   1, 1/3},
							   {   0,   0,   1,   1,   1,   1,   1, 2/3},
						       {   0,   1,   1,   1,   1,   1,   1,   1},
							   { 1/3,   1,   1,   1,   1,   1,   1,   1},
							   { 2/3,   1,   1,   1,   1,   1,   1,   1},
							   {   1,   1,   1,   1,   1,   1,   1, 1/4},
							   {   1,   1,   1,   1,   1,   1,   1,   0},
							   { 2/3,   1,   1,   1,   1,   1, 1/2,   0},
							   { 1/2,   1,   1,   1,   1,   1, 1/3,   0},
							   { 1/2,   1,   1,   1,   1,   1,   0,   0},
							   { 1/2,   1,   1,   1,   1,   1,   0,   0},
							   { 2/3,   1,   1,   1,   1,   1,   0,   0},   //        ^ X direction
							   { 2/3,   1,   1,   1,   1, 2/3,   0,   0},   //	      | 
							   { 1/2,   1,   1,   1,   1, 1/2,   0,   0},   //	      |
							   {   0, 1/2,   1,   1, 1/2,   0,   0,   0} }; // Y < ----  

	};
};
