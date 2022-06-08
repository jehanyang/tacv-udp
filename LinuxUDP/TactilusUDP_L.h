#include "udp_client_server.h"
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>

#define BUFLEN 16384             //Max length of buffer

// Author:  Jehan Yang
// Updated: 09/01/2021
// Version: 1.14
//
// V1.14: Added constructor to have option of 1 or 2 sensors
// V1.12 for getting forces and moments for two pressure sensors
namespace tactilus_udp_linux
{
	class TactilusUDP_L
	{
	
	public:
	// Added constructor that can tell Windows how many sensors to initialize	
	TactilusUDP_L(std::string src_serv, u_int src_port, double desired_x_pos, double desired_y_pos, std::string nsens);
	// Added constructor that has desired y_por for where to get moment about
	TactilusUDP_L(std::string src_serv, u_int src_port, double desired_x_pos, double desired_y_pos);
	// Initialize class with local port and local IP address
	TactilusUDP_L(std::string src_serv, u_int src_port, double desired_x_pos);
	
	// Destructor
	~TactilusUDP_L();
	
	// Send something to the address we shook hands with in initializer/constructor
	void send(std::string message);

	// Writes to buf internal variable what we receive, repeats many times if necessary
	int recv();
	
	// Returns buffer that we received on
	char* getbuf();
	
	// Get force in N, is a vector so could be 1 or 2 floats corresponding to number of sensors wanted
	std::vector<float> getforce();
	
	// Get moment in Nm
	std::vector<float> getmoment();
	
	// Get force in N and moment in Nm at same time
	std::vector<float> getforcemoment();

	// Get force in N and moment about y and moment about x in Nm at same time
	std::vector<float> getforcemoments();

	// Get force in N, sensornum is either 1, 2, or *
	std::vector<float> getforce(std::string sensornum);

	// Get moment in Nm, sensornum is either 1, 2, or *
	std::vector<float> getmoment(std::string sensornum);

	// Get force in N and moment in Nm at the same time, sensornum is either 1, 2, or *
	std::vector<float> getforcemoment(std::string sensornum);

	// Get force in N and moments in Nm at the same time, sensornum is either 1, 2, or *
	std::vector<float> getforcemoments(std::string sensornum);

	private:

	char buf[BUFLEN];
    struct sockaddr_in si_other;
    socklen_t slen;
    std::string server_addr;
    udp_client_server::udp_server* svr;
    char addrbuf[32];
	double x_des;
	double y_des;
	};
}
