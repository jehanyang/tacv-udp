/*
	Simple udp client
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include "Tactilus.h"
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include<winsock2.h>
#include<Ws2tcpip.h>
#include<math.h>
#include"TactilusUDP.h"
#include<chrono>
#include<thread>
#include <mutex>


#pragma comment(lib,"ws2_32.lib") //Winsock Library
#pragma comment(lib,"core.lib")


#define SERVER "10.7.0.11"		//ip address of bbb over usb
#define SRCPORT 23498	//The port on which to send from for permissions(?) purposes
#define DSTPORT 29292	//The port on which to send data to bbb

// Author:	Jehan Yang
// Updated:	06/07/2022
// Version: 1.16

// V1.10: Added a second sensor. The sensor with ID ending in 102 is always registered as the second Tactilus object that connects. The sensor with ID ending in 103 seems to always register as the first Tactilus object that connects. 
// V1.11: Added code so that force and moment for both sensors can be sent at one time, which is likely closest to our use case.
// V1.12: Added code to get moment about x and y axes
// V1.13: Rewrote main loop to be two threads. One is constantly waiting for a message and sends immediately. The other is always updating the data structure with new pressure readings. 
// V1.14: Allowed option of choosing whether 1 or 2 sensors to be used
// V1.15: Found that the way to cause the fewest delays is to constantly send from Windows. Multithreading is unnecessary except for updating both at once (not for updating and waiting/sending). Also added ability to get specific pressure pad forces.
// V1.16: Cleaned up code, deleted unnecessary comments and deleted some unused code. 

namespace tactilus_udp
{
	TactilusUDP::TactilusUDP(const char* dest_address, u_int src_port, u_int dest_port)
		//	Initialize everything as needed
		/*	char* dest_address			IPv4 address destination written as a string
		//	u_int src_port				local port that we are sending from (irrelevant if not communicator)
		//	u_int dest_port				remote port that we are sending to (irrelevant if not communicator)
		//  u_int communicator          1 if this sensor is the commmunicator (the sensor object sending and receiving messages) and 0 if it is not
		*/
	{
		// This part initializes the Tactilus data querying object
		this->t = new Tactilus();

		printf("Connecting to Tactilus...");
		t->connect(true); //do this first so that the next t->connect(true); doesn't take seconds to happen
		printf("Connected.\n");

		this->ringbufwritehead = 0;
		this->rows = this->t->rowCount();
		this->cols = this->t->columnCount();

		// Makes float[][] that characterizes areas of each pad in mm^2
		double padarea = 13.0 *17.2;
		for (unsigned int i = 0; i < 16; ++i)
		{
			for (unsigned int j = 0; j < 8; ++j)
			{
				areas[i][j] = areas[i][j] * padarea;
			}
		}

		//Initialize winsock
		printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			printf("Failed. Error Code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		printf("Initialised.\n");

		//Create a socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
		{
			printf("Could not create socket : %d", WSAGetLastError());
		}
		printf("Socket created...");

		//setup address structure
		memset((char *)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET; // designates that we are using IPv4 addresses
		si_other.sin_port = htons(dest_port);
		si_other.sin_addr.s_addr = inet_addr(dest_address);

		memset((char *)&srcaddr, 0, sizeof(srcaddr));
		srcaddr.sin_family = AF_INET; // designates that we are using IPv4 addresses
		srcaddr.sin_port = htons(src_port);
		srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		//Bind, tying our socket to the srcaddr information (local IP address, source port)
		if (bind(s, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) == SOCKET_ERROR)
		{
			printf("Bind failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		puts("Bind done...\n");

		printf("Setting socket to non-blocking...");
		u_long mode = 1;  // 1 to enable non-blocking socket, this allows recv to time out and not run forever
		ioctlsocket(s, FIONBIO, &mode);
		printf("Set.\n");

	}

	TactilusUDP::TactilusUDP(const char* dest_address, u_int src_port, u_int dest_port, u_int communicator)
		//	Initialize everything as needed
		/*	char* dest_address			IPv4 address destination written as a string
		//	u_int src_port				local port that we are sending from (irrelevant if not communicator)
		//	u_int dest_port				remote port that we are sending to (irrelevant if not communicator)
		//  u_int communicator          1 if this sensor is the commmunicator (the sensor object sending and receiving messages) and 0 if it is not
		*/
	{
		// This part initializes the Tactilus data querying object
		this->t = new Tactilus();

		printf("Connecting to Tactilus...");
		t->connect(true); //do this first so that the next t->connect(true); doesn't take seconds to happen
		printf("Connected.\n");

		this->ringbufwritehead = 0;
		this->rows = this->t->rowCount();
		this->cols = this->t->columnCount();

		// Makes float[][] that characterizes areas of each pad in mm^2
		double padarea = 13.0 *17.2;
		for (unsigned int i = 0; i < 16; ++i)
		{
			for (unsigned int j = 0; j < 8; ++j)
			{
				areas[i][j] = areas[i][j] * padarea;
			}
		}

		//Initialize winsock
		printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			printf("Failed. Error Code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		printf("Initialised.\n");

		//Create a socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
		{
			printf("Could not create socket : %d", WSAGetLastError());
		}
		printf("Socket created...");

		//setup address structure
		memset((char *)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET; // designates that we are using IPv4 addresses
		si_other.sin_port = htons(dest_port);
		si_other.sin_addr.s_addr = inet_addr(dest_address);

		memset((char *)&srcaddr, 0, sizeof(srcaddr));
		srcaddr.sin_family = AF_INET; // designates that we are using IPv4 addresses
		srcaddr.sin_port = htons(src_port);
		srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		//Bind, tying our socket to the srcaddr information (local IP address, source port)
		if (bind(s, (struct sockaddr *)&srcaddr, sizeof(srcaddr)) == SOCKET_ERROR && communicator==1)
		{
			printf("Bind failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		if (communicator == 1)
		{
			puts("Bind done...\n");
			printf("Setting socket to non-blocking...");
			u_long mode = 1;  // 1 to enable non-blocking socket, this allows recv to time out and not run forever (may not be necessary)
			ioctlsocket(s, FIONBIO, &mode);
			printf("Set.\n");
		}	

	}

	TactilusUDP::~TactilusUDP()
		//	Clean up Tactilus class
	{
		closesocket(s);
		WSACleanup();
	}

	Tactilus * TactilusUDP::gettactilusid()
	{
		return this->t;
	}

	void TactilusUDP::send(std::string message)
		//	Takes a message and sends it to dest_address with src_port and dest_port as initialized
	{
		if (sendto(s, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
	}

	void TactilusUDP::recv()
		//	Checks whether anything is in to be received to our address and src_port
		//  If there isn't, then just leave after setting buf = all \0s
	{
		memset(buf, '\0', BUFLEN);

		//auto start = std::chrono::high_resolution_clock::now();

		if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == SOCKET_ERROR) // if you desire to block and repeatedly get pressure readings for one sensor (however, we want both sensors to update repeatedly), should use while instead of if

		{
			//auto start = std::chrono::high_resolution_clock::now(); // this was used for timing the clock


			if (WSAGetLastError() == WSAEWOULDBLOCK) // I think this kind of error means nothing was received and recvfrom should repeat
			{
				// The following code is really just left over from 1.0, when only 1 sensor is used
				// the reason for the following block of code is that this used to update indefinitely the ring buffer until some actual request is made. Now with two sensors, we don't want to block the other sensor from updating. 

				//this->t->scan();
				//float* value = t->matrix();
				//for (unsigned int i = 0; i < 128; ++i, ++value) // Gaussian smoothing implementation
				//{

				//	if (i == 0) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value + 1) * (1.0f / 8) + *(value + 8) * (1.0f / 8) + *(value + 9) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // top left corner case
				//	}
				//	else if (i == 7) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 8) * (1.0f / 8) + *(value + 7) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // top right corner case
				//	}
				//	else if (i == 120) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value - 7) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // bottom left corner case
				//	}
				//	else if (i == 127) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value - 9) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // bottom right corner case
				//	}
				//	else if (i < 8) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 1) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
				//			*(value + 7) * (1.0f / 16) + *(value + 9) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // top wall sans corners
				//	}
				//	else if (i % 8 == 0) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
				//			*(value + 9) * (1.0f / 16) + *(value - 7) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // left wall sans corners
				//	}
				//	else if (i % 8 == 7) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
				//			*(value - 9) * (1.0f / 16) + *(value + 7) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // right wall sans corners
				//	}
				//	else if (i > 119) {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) +
				//			*(value - 7) * (1.0f / 16) + *(value - 9) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // bottom wall sans corners
				//	}
				//	else {
				//		this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
				//			*(value - 7) * (1.0f / 16) + *(value - 9) * (1.0f / 16) + *(value + 7) * (1.0f / 16) + *(value + 9) * (1.0f / 16)) * 6.8947572932f); // middle pads
				//	}
				//	// 6.8947572932 is the conversion factor, 1 psi = 6.8947572932 kPa
				//	// ringbuf therefore stores in kPa
				//}
				//ringbufwritehead = (ringbufwritehead + 1) % FORCEBUFLEN;
				
			}
			else {
				printf("recvfrom() failed with error code : %d", WSAGetLastError());
				exit(EXIT_FAILURE);
			}
		}
	}

	void TactilusUDP::update() {
		this->t->scan();
		float* value = t->matrix();
		for (unsigned int i = 0; i < 128; ++i, ++value) // Gaussian smoothing implementation
		{

			if (i == 0) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value + 1) * (1.0f / 8) + *(value + 8) * (1.0f / 8) + *(value + 9) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // top left corner case
			}
			else if (i == 7) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 8) * (1.0f / 8) + *(value + 7) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // top right corner case
			}
			else if (i == 120) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value - 7) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // bottom left corner case
			}
			else if (i == 127) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value - 9) * (1.0f / 16)) * 6.8947572932f) * 16.0f / 9; // bottom right corner case
			}
			else if (i < 8) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 1) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
					*(value + 7) * (1.0f / 16) + *(value + 9) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // top wall sans corners
			}
			else if (i % 8 == 0) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
					*(value + 9) * (1.0f / 16) + *(value - 7) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // left wall sans corners
			}
			else if (i % 8 == 7) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
					*(value - 9) * (1.0f / 16) + *(value + 7) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // right wall sans corners
			}
			else if (i > 119) {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) +
					*(value - 7) * (1.0f / 16) + *(value - 9) * (1.0f / 16)) * 6.8947572932f) * 4.0f / 3; // bottom wall sans corners
			}
			else {
				this->ringbuf[ringbufwritehead][i] = ((*value * (1.0f / 4) + *(value - 1) * (1.0f / 8) + *(value + 1) * (1.0f / 8) + *(value - 8) * (1.0f / 8) + *(value + 8) * (1.0f / 8) +
					*(value - 7) * (1.0f / 16) + *(value - 9) * (1.0f / 16) + *(value + 7) * (1.0f / 16) + *(value + 9) * (1.0f / 16)) * 6.8947572932f); // middle pads
			}
			// 6.8947572932 is the conversion factor, 1 psi = 6.8947572932 kPa
			// ringbuf therefore stores in kPa
		}
		ringbufwritehead = (ringbufwritehead + 1) % FORCEBUFLEN;
	}

	char* TactilusUDP::getbuf()
		//	Returns pointer to first index of buffer of message received
	{
		return buf;
	}

	float* TactilusUDP::getpresbuftosend()
		//  Returns presbuftosend
	{
		return presbuftosend;
	}

	void TactilusUDP::updatepresbuftosend()
		//  Update presbuftosend
	{
		int head = 0;
		float avgcurrkPa = 0;

		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				presbuftosend[head] = avgcurrkPa / FORCEBUFLEN; // to take the avg
				avgcurrkPa = 0;
			}
		}
	}

	std::string TactilusUDP::allpressurepads()
		//	Return string with all pressure readings in an array in [x,y] = P format; Units of kPa
	{
		char msg[BUFLEN];
		std::string concatstr = "";
		int head = 0;
		float avgcurrkPa = 0;

		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				if (snprintf(msg, sizeof(msg), "[%d,%d] = %f \n", c, r, avgcurrkPa) < 0)
				{
					throw ERROR_DS_ENCODING_ERROR;
				}
				concatstr.append(msg);
				avgcurrkPa = 0;
			}
		}
		return concatstr;
	}

	double TactilusUDP::estimateForce()
		//  Estimate force by multiplying areas with pressure, in N
	{
		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;

		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double) avgcurrkPa * 1000);
				// Force will be in Newtons
				avgcurrkPa = 0;
			}
		}
		return force;
	}

	double* TactilusUDP::estimateCoP()
		//  Estimate location of center of pressure, in mm
		//  x or p[0] should go from back to front of foot.
		//  y or p[1] should go from inside to outside of foot [will be anti-parallel from left foot to right foot].
	{
		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;
		double force_per_mm_rows[16] = { 0 };
		double force_per_mm_cols[8] = { 0 };
		static double p[2] = { 0 };
		p[0] = 0;
		p[1] = 0;
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double) avgcurrkPa * 1000);
				// Force will be in Newtons

				// We want to get force per mm_rows to help in the numreator in the CoP calculation
				force_per_mm_rows[r] = force_per_mm_rows[r] + this->areas[r][c] / 17.2 * avgcurrkPa / 1000;
				force_per_mm_cols[c] = force_per_mm_cols[c] + this->areas[r][c] / 9.0 * avgcurrkPa / 1000;
				avgcurrkPa = 0;
			}
		}
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			p[0] = p[0] + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
		}
		p[0] = p[0] / force;
		for (unsigned int c = 0; c < this->cols; ++c)
		{
			p[1] = p[1] + force_per_mm_cols[c] * 9.0 * (100 - (c * 9.0 + 9.0 / 2.0)); // (c * 9.0 + 9.0/2.0) are the -y center locations of pads
		}
		p[1] = p[1] / force;

		return p;
	}

	double TactilusUDP::estimateMoment_y(double x1)
		//  Estimate value of moment at specified location x1: x_max should be 270mm. 
		//  x should go from back of foot to front of foot
		//  Returns in Nm
	{
		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;
		double force_per_mm_rows[16] = { 0 };
		double force_per_mm_cols[8] = { 0 };
		double x0 = { 0 };
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double) avgcurrkPa * 1000);
				// Force will be in Newtons

				// We want to get force per mm_rows to help in the numreator in the CoP calculation
				force_per_mm_rows[r] = force_per_mm_rows[r] + this->areas[r][c] / 17.2 * avgcurrkPa / 1000;

				// Force will be in Newtons
				avgcurrkPa = 0;
			}
		}
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
		}
		if (force == 0) {
			return 0; // Likely means no pressure on sensor at all
		}
		x0 = x0 / force;

		return -(x0 - x1)*force / 1000; // Divide by 1000 to get Nm
	}

	double* TactilusUDP::estimateForceAndMoment_y(double x1)
		//  Estimate value of moment at specified location x1: x_max should be 270mm. 
		//  x should go from back of foot to front of foot
		//  Returns in N and Nm
	{
		static double retforceandmoment[2];

		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;
		double force_per_mm_rows[16] = { 0 };
		double force_per_mm_cols[8] = { 0 };
		double x0 = { 0 };
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double) avgcurrkPa * 1000);
				// Force will be in Newtons

				// We want to get force per mm_rows to help in the numreator in the CoP calculation
				force_per_mm_rows[r] = force_per_mm_rows[r] + this->areas[r][c] / 17.2 * avgcurrkPa / 1000;

				// Force will be in Newtons
				avgcurrkPa = 0;
			}
		}
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
		}

		x0 = x0 / force;

		retforceandmoment[0] = force;
		if (force == 0) {
			retforceandmoment[1] = 0; // Likely means no pressure on sensor at all
		}
		else {
			retforceandmoment[1] = -(x0 - x1)*force / 1000.0; // Divide by 1000 to get Nm
		}
		return retforceandmoment;
	}

	double* TactilusUDP::estimateForceAndMoment_yx(double x1, double y1) {
		//  Estimate force, value about y axis of moment at specified location x1 [x_max should be 270mm], and value about x axis of moment at specified location y1 [y_max should be 100mm]
		//  x should go from back of foot to front of foot, y should go from inside of foot to outside of foot [will be anti-parallel from left foot to right foot]
		//  Returns in N and Nm
		//  NOTE: be careful of the signs of moments! They may not be consistent depending on which foot. Make sure to figure out what the signs of the moments should be further down the algorithm

		static double retforceandmoment[3];

		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;
		double force_per_mm_rows[16] = { 0 };
		double force_per_mm_cols[8] = { 0 };
		double x0 = { 0 };
		double y0 = { 0 };
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa, head is like c except head doesn't go to 0 after ++r (remember ringbuf is indexed by time firstly and a vector of 128 secondly)
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double)avgcurrkPa * 1000);
				// Force will be in Newtons

				// We want to get force per mm_rows to help in the numreator in the CoP calculation
				force_per_mm_rows[r] = force_per_mm_rows[r] + this->areas[r][c] / 17.2 * avgcurrkPa / 1000;
				force_per_mm_cols[c] = force_per_mm_cols[c] + this->areas[r][c] / 13.0 * avgcurrkPa / 1000; 

				// Force will be in Newtons
				avgcurrkPa = 0;
			}
		}
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
		}
		for (unsigned int c = 0; c < this->cols; ++c)
		{
			y0 = y0 + force_per_mm_cols[c] * 13.0 * (100 - (c * 13.0 + 13.0 / 2.0)); // (c * 13.0 + 13.0/2.0) are the -y center locations of pads
		}

		x0 = x0 / force; // the all-important dividing by total force to get mm for CoP (the for loop above made x0 or y0 into total moment about y axis or x axis)
		y0 = y0 / force;

		retforceandmoment[0] = force;
		if (force == 0) {
			retforceandmoment[1] = 0; // Likely means no pressure on sensor at all
			retforceandmoment[2] = 0;
		}
		else {
			retforceandmoment[1] = -(x0 - x1)*force / 1000.0; // Divide by 1000 to get Nm
			retforceandmoment[2] = (y0 - y1)*force / 1000.0;
		}
		return retforceandmoment;
	}

	std::vector<double> TactilusUDP::estimateForceAndMoment_yx_somepadforces(double x1, double y1, u_int* padx, u_int* pady, u_int padnumber) {
		//  Estimate force, value about y axis of moment at specified location x1 [x_max should be 270mm], and value about x axis of moment at specified location y1 [y_max should be 100mm]
		//  x should go from back of foot to front of foot, y should go from inside of foot to outside of foot [will be anti-parallel from left foot to right foot]
		//  Will also return forces from requested pads, given by x coordinates in padx and y coordinates in pady
		//  Returns in N and Nm
		//  NOTE: be careful of the signs of moments! They may not be consistent depending on which foot. Make sure to figure out what the signs of the moments should be further down the algorithm

		static std::vector<double> retforceandmoment(padnumber + 3);

		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;
		u_int currentpad = 0;
		double force_per_mm_rows[16] = { 0 };
		double force_per_mm_cols[8] = { 0 };
		double x0 = { 0 };
		double y0 = { 0 };
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa, head is like c except head doesn't go to 0 after ++r (remember ringbuf is indexed by time firstly and a vector of 128 secondly)
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double)avgcurrkPa * 1000);
				// Force will be in Newtons

				// We want to get force per mm_rows to help in the numreator in the CoP calculation
				force_per_mm_rows[r] = force_per_mm_rows[r] + this->areas[r][c] / 17.2 * avgcurrkPa / 1000;
				force_per_mm_cols[c] = force_per_mm_cols[c] + this->areas[r][c] / 13.0 * avgcurrkPa / 1000;

				for (u_int j = 0; j < padnumber; ++j)
				{
					if (r == padx[j] && c == pady[j]) {
						retforceandmoment[3 + j] = (this->areas[padx[j]][pady[j]] / 1e6) * ((double)avgcurrkPa * 1000);
					}
				}

				// Force will be in Newtons
				avgcurrkPa = 0;
			}
		}
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
		}
		for (unsigned int c = 0; c < this->cols; ++c)
		{
			y0 = y0 + force_per_mm_cols[c] * 13.0 * (100 - (c * 13.0 + 13.0 / 2.0)); // (c * 13.0 + 13.0/2.0) are the -y center locations of pads
		}

		x0 = x0 / force; // the all-important dividing by total force to get mm for CoP (the for loop above made x0 or y0 into total moment about y axis or x axis)
		y0 = y0 / force;

		retforceandmoment[0] = force;
		if (force == 0) {
			retforceandmoment[1] = 0; // Likely means no pressure on sensor at all
			retforceandmoment[2] = 0;
		}
		else {
			retforceandmoment[1] = -(x0 - x1)*force / 1000.0; // Divide by 1000 to get Nm
			retforceandmoment[2] = (y0 - y1)*force / 1000.0;
		}
	
		return retforceandmoment;
	}
	std::vector<double> TactilusUDP::estimateForceAndMoment_yx_frontbackforces(double x1, double y1) {
		//  Estimate force, value about y axis of moment at specified location x1 [x_max should be 270mm], and value about x axis of moment at specified location y1 [y_max should be 100mm]
		//  x should go from back of foot to front of foot, y should go from inside of foot to outside of foot [will be anti-parallel from left foot to right foot]
		//  Also returns force of front 64 pads and force of back 64 pads
		//  Returns in N and Nm
		//  NOTE: be careful of the signs of moments! They may not be consistent depending on which foot. Make sure to figure out what the signs of the moments should be further down the algorithm

		static std::vector<double> retforceandmoment(5);
		retforceandmoment[3] = 0;
		retforceandmoment[4] = 0;

		double force = 0;
		int head = 0;
		float avgcurrkPa = 0;
		u_int currentpad = 0;
		double force_per_mm_rows[16] = { 0 };
		double force_per_mm_cols[8] = { 0 };
		double x0 = { 0 };
		double y0 = { 0 };
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			for (unsigned int c = 0; c < this->cols; ++c, ++head)
			{
				for (unsigned int i = 0; i < FORCEBUFLEN; ++i)
				{
					// ringbuf stores kPa, head is like c except head doesn't go to 0 after ++r (remember ringbuf is indexed by time firstly and a vector of 128 secondly)
					avgcurrkPa = avgcurrkPa + ringbuf[(ringbufwritehead - 1 + i) % FORCEBUFLEN][head];
				}
				avgcurrkPa = avgcurrkPa / FORCEBUFLEN; // to take the avg
				//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
				force = force + (this->areas[r][c] / 1e6) * ((double)avgcurrkPa * 1000);
				// Force will be in Newtons

				// We want to get force per mm_rows to help in the numreator in the CoP calculation
				force_per_mm_rows[r] = force_per_mm_rows[r] + this->areas[r][c] / 17.2 * avgcurrkPa / 1000;
				force_per_mm_cols[c] = force_per_mm_cols[c] + this->areas[r][c] / 13.0 * avgcurrkPa / 1000;

				if (r < 8) {
					retforceandmoment[3] = retforceandmoment[3] + (this->areas[r][c] / 1e6) * ((double)avgcurrkPa * 1000);
				}
				else {
					retforceandmoment[4] = retforceandmoment[4] + (this->areas[r][c] / 1e6) * ((double)avgcurrkPa * 1000);
				}

				// Force will be in Newtons
				avgcurrkPa = 0;
			}
		}
		for (unsigned int r = 0; r < this->rows; ++r)
		{
			x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
		}
		for (unsigned int c = 0; c < this->cols; ++c)
		{
			y0 = y0 + force_per_mm_cols[c] * 13.0 * (100 - (c * 13.0 + 13.0 / 2.0)); // (c * 13.0 + 13.0/2.0) are the -y center locations of pads
		}

		x0 = x0 / force; // the all-important dividing by total force to get mm for CoP (the for loop above made x0 or y0 into total moment about y axis or x axis)
		y0 = y0 / force;

		retforceandmoment[0] = force;
		if (force == 0) {
			retforceandmoment[1] = 0; // Likely means no pressure on sensor at all
			retforceandmoment[2] = 0;
		}
		else {
			retforceandmoment[1] = -(x0 - x1) * force / 1000.0; // Divide by 1000 to get Nm
			retforceandmoment[2] = (y0 - y1) * force / 1000.0;
		}

		return retforceandmoment;
	}
}

std::string msg;
char* recvmsg;
int recvmsglen;
double desiredmomentx;
double desiredmomenty;
double* cop;
double* forcemoment;
u_int padnumbersize = 2;
std::vector<double> forcemomentvec(2+padnumbersize);
double force, momentx, momenty;
float presbuftosend1[128], presbuftosend2[128];
std::mutex presbuftosendX_mutex;
u_int numsensreq; //number of sensors requested by BBB at beginning of main
u_int padx_des[2] = { 13, 2 };
u_int pady_des[2] = { 2, 4 };
double pad1force, pad2force;

// Note, in the reference frame, we define x as along the columns and y as along the rows
		// the origin is in the top left. x increases as we go up, y increases as we go left, which
		// coincides with the rows and columns indices decreasing.
double areas[16][8] = { {   0,   0,   0,   0, 2/3,   1, 1/2,   0},
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

//  sa stands for standalone, meaning not in a class
std::string sa_allpressurepads(float* presbuftosendX)
//	Return string with all pressure readings in an array in [x,y] = P format; Units of kPa
{
	char msg[BUFLEN];
	std::string concatstr = "";
	int head = 0;

	for (unsigned int r = 0; r < 16; ++r)
	{
		for (unsigned int c = 0; c < 8; ++c, ++head)
		{
			if (snprintf(msg, sizeof(msg), "[%d,%d] = %f \n", c, r, presbuftosendX[head]) < 0)
			{
				throw ERROR_DS_ENCODING_ERROR;
			}
			concatstr.append(msg);
		}
	}
	return concatstr;
}

double sa_estimateForce(float* presbuftosendX)
//  Estimate force by multiplying areas with pressure, in N
{
	double force = 0;
	int head = 0;

	for (unsigned int r = 0; r < 16; ++r)
	{
		for (unsigned int c = 0; c < 8; ++c, ++head)
		{
			//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
			force = force + (areas[r][c] / 1e6) * ((double) presbuftosendX[head] * 1000);
			// Force will be in Newtons
		}
	}
	return force;
}

double* sa_estimateCoP(float* presbuftosendX)
//  Estimate location of center of pressure, in mm
//  x or p[0] should go from back to front of foot.
//  y or p[1] should go from inside to outside of foot [will be anti-parallel from left foot to right foot].
{
	double force = 0;
	int head = 0;
	double force_per_mm_rows[16] = { 0 };
	double force_per_mm_cols[8] = { 0 };
	static double p[2] = { 0 };
	p[0] = 0;
	p[1] = 0;
	for (unsigned int r = 0; r < 16; ++r)
	{
		for (unsigned int c = 0; c < 8; ++c, ++head)
		{
			//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
			force = force + (areas[r][c] / 1e6) * ((double) presbuftosendX[head] * 1000);
			// Force will be in Newtons

			// We want to get force per mm_rows to help in the numreator in the CoP calculation
			force_per_mm_rows[r] = force_per_mm_rows[r] + areas[r][c] / 17.2 * presbuftosendX[head] / 1000;
			force_per_mm_cols[c] = force_per_mm_cols[c] + areas[r][c] / 9.0 * presbuftosendX[head] / 1000;
		}
	}
	for (unsigned int r = 0; r < 16; ++r)
	{
		p[0] = p[0] + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
	}
	p[0] = p[0] / force;
	for (unsigned int c = 0; c < 8; ++c)
	{
		p[1] = p[1] + force_per_mm_cols[c] * 9.0 * (100 - (c * 9.0 + 9.0 / 2.0)); // (c * 9.0 + 9.0/2.0) are the -y center locations of pads
	}
	p[1] = p[1] / force;

	return p;
}

double sa_estimateMoment_y(float* presbuftosendX, double x1)
//  Estimate value of moment at specified location x1: x_max should be 270mm. 
//  x should go from back of foot to front of foot
//  Returns in Nm
{
	double force = 0;
	int head = 0;
	double force_per_mm_rows[16] = { 0 };
	double force_per_mm_cols[8] = { 0 };
	double x0 = { 0 };
	for (unsigned int r = 0; r < 16; ++r)
	{
		for (unsigned int c = 0; c < 8; ++c, ++head)
		{
			//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
			force = force + (areas[r][c] / 1e6) * ((double)presbuftosendX[head] * 1000);
			// Force will be in Newtons

			// We want to get force per mm_rows to help in the numreator in the CoP calculation
			force_per_mm_rows[r] = force_per_mm_rows[r] + areas[r][c] / 17.2 * presbuftosendX[head] / 1000;

			// Force will be in Newtons
			presbuftosendX[head] = 0;
		}
	}
	for (unsigned int r = 0; r < 16; ++r)
	{
		x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
	}
	if (force == 0) {
		return 0; // Likely means no pressure on sensor at all
	}
	x0 = x0 / force;

	return -(x0 - x1)*force / 1000; // Divide by 1000 to get Nm
}

double* sa_estimateForceAndMoment_y(float* presbuftosendX, double x1)
//  Estimate value of moment at specified location x1: x_max should be 270mm. 
//  x should go from back of foot to front of foot
//  Returns in N and Nm
{
	static double retforceandmoment[2];

	double force = 0;
	int head = 0;
	double force_per_mm_rows[16] = { 0 };
	double force_per_mm_cols[8] = { 0 };
	double x0 = { 0 };
	for (unsigned int r = 0; r < 16; ++r)
	{
		for (unsigned int c = 0; c < 8; ++c, ++head)
		{
			//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
			force = force + (areas[r][c] / 1e6) * ((double)presbuftosendX[head] * 1000);
			// Force will be in Newtons
 
			force_per_mm_rows[r] = force_per_mm_rows[r] + areas[r][c] / 17.2 * presbuftosendX[head] / 1000;

		}
	}
	for (unsigned int r = 0; r < 16; ++r)
	{
		x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
	}

	x0 = x0 / force;

	retforceandmoment[0] = force;
	if (force == 0) {
		retforceandmoment[1] = 0; // Likely means no pressure on sensor at all
	}
	else {
		retforceandmoment[1] = -(x0 - x1)*force / 1000.0; // Divide by 1000 to get Nm
	}
	return retforceandmoment;
}

double* sa_estimateForceAndMoment_yx(float* presbuftosendX, double x1, double y1) {
	//  Estimate force, value about y axis of moment at specified location x1 [x_max should be 270mm], and value about x axis of moment at specified location y1 [y_max should be 100mm]
	//  x should go from back of foot to front of foot, y should go from inside of foot to outside of foot [will be anti-parallel from left foot to right foot]
	//  Returns in N and Nm
	//  NOTE: be careful of the signs of moments! They may not be consistent depending on which foot. Make sure to figure out what the signs of the moments should be further down the algorithm

	static double retforceandmoment[3];

	double force = 0;
	int head = 0;
	double force_per_mm_rows[16] = { 0 };
	double force_per_mm_cols[8] = { 0 };
	double x0 = { 0 };
	double y0 = { 0 };
	for (unsigned int r = 0; r < 16; ++r)
	{
		for (unsigned int c = 0; c < 8; ++c, ++head)
		{
			//           Divide by 1e6 to get m^2   Multiply by 1000 to get Pa
			force = force + (areas[r][c] / 1e6) * ((double)presbuftosendX[head] * 1000);
			// Force will be in Newtons

			force_per_mm_rows[r] = force_per_mm_rows[r] + areas[r][c] / 17.2 * presbuftosendX[head] / 1000;
			force_per_mm_cols[c] = force_per_mm_cols[c] + areas[r][c] / 13.0 * presbuftosendX[head] / 1000;

		}
	}
	for (unsigned int r = 0; r < 16; ++r)
	{
		x0 = x0 + force_per_mm_rows[r] * 17.2 * (270 - (r * 17.2 + 17.2 / 2.0)); // (r * 17.2 + 17.2/2.0) are the -x center locations of pads
	}
	for (unsigned int c = 0; c < 8; ++c)
	{
		y0 = y0 + force_per_mm_cols[c] * 13.0 * (100 - (c * 13.0 + 13.0 / 2.0)); // (c * 13.0 + 13.0/2.0) are the -y center locations of pads
	}

	x0 = x0 / force; // the all-important dividing by total force to get mm for CoP (the for loop above made x0 or y0 into total moment about y axis or x axis)
	y0 = y0 / force;

	retforceandmoment[0] = force;
	if (force == 0) {
		retforceandmoment[1] = 0; // Likely means no pressure on sensor at all
		retforceandmoment[2] = 0;
	}
	else {
		retforceandmoment[1] = -(x0 - x1)*force / 1000.0; // Divide by 1000 to get Nm
		retforceandmoment[2] = (y0 - y1)*force / 1000.0;
	}
	return retforceandmoment;
}

unsigned long send_counter;
u_int rel_send_counter;

void updateandsend(tactilus_udp::TactilusUDP& tact1, tactilus_udp::TactilusUDP& tact2, float* presbuftosend1, float* presbuftosend2)
{
	while (1)
	{
		if (tact1.gettactilusid() != tact2.gettactilusid()) {
			std::thread x(&tactilus_udp::TactilusUDP::update, &tact1);
			std::thread y(&tactilus_udp::TactilusUDP::update, &tact2);
			x.join();
			y.join();
		}
		else {
			tact1.update();
		}

		forcemomentvec = tact1.estimateForceAndMoment_yx_frontbackforces(desiredmomentx, desiredmomenty);
		force = forcemomentvec[0];
		momenty = forcemomentvec[1];
		momentx = forcemomentvec[2];
		pad1force = forcemomentvec[3];
		pad2force = forcemomentvec[4];
		msg = std::to_string(force);
		msg.append(",");
		msg.append(std::to_string(momenty));
		msg.append(",");
		msg.append(std::to_string(momentx));
		msg.append(",");
		msg.append(std::to_string(pad1force));
		msg.append(",");
		msg.append(std::to_string(pad2force));

		if (tact1.gettactilusid() != tact2.gettactilusid()) {
			forcemomentvec = tact2.estimateForceAndMoment_yx_frontbackforces(desiredmomentx, desiredmomenty);
			force = forcemomentvec[0];
			momenty = forcemomentvec[1];
			momentx = forcemomentvec[2];
			pad1force = forcemomentvec[3];
			pad2force = forcemomentvec[4];
			msg.append(",");
			msg.append(std::to_string(force));
			msg.append(",");
			msg.append(std::to_string(momenty));
			msg.append(",");
			msg.append(std::to_string(momentx));
			msg.append(",");
			msg.append(std::to_string(pad1force));
			msg.append(",");
			msg.append(std::to_string(pad2force));
		}

		tact1.send(msg);
	}
}

int main(void)
{
	tactilus_udp::TactilusUDP *tact1;
	tact1 = new tactilus_udp::TactilusUDP(SERVER, SRCPORT, DSTPORT, 1);
	
	tact1->send("handshake");
	printf("Handshake sent.\n");
	while (recvmsglen == 0) {
		tact1->recv();
		recvmsg = tact1->getbuf();
		recvmsglen = strlen(recvmsg);
	}

	std::string recvmsgstr(recvmsg, recvmsg + recvmsglen);
	std::string temp;

	// the following is used to break up recvmsgstr string to a vector of strings
	std::stringstream ss(recvmsgstr);
	std::vector<std::string> words;

	while (getline(ss, temp, ',')) {
		words.push_back(temp);
	}

	desiredmomentx = std::stod(words[0]);
	desiredmomenty = std::stod(words[1]);

	if (words[2].compare("1") == 0)
	{
		tact1->update();
		for (unsigned int i = 0; i < 128; ++i) {
			presbuftosend1[i] = tact1->getpresbuftosend()[i];
		}
		updateandsend(*tact1, *tact1, presbuftosend1, presbuftosend2);
	}
	else if (words[2].compare("2") == 0)
	{
		tactilus_udp::TactilusUDP *tact2;
		tact2 = new tactilus_udp::TactilusUDP(SERVER, SRCPORT, DSTPORT, 0);
		tact1->update();
		tact2->update();
		for (unsigned int i = 0; i < 128; ++i) {
			presbuftosend1[i] = tact1->getpresbuftosend()[i];
			presbuftosend2[i] = tact2->getpresbuftosend()[i];
		}
		updateandsend(*tact1, *tact2, presbuftosend1, presbuftosend2);
	}

	return 0;
}
