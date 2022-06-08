#include "udp_client_server.h"
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include"TactilusUDP_L.h"

#define SERVER "10.7.0.11"   //IP address of UDP Server received on
#define PORT 29292             //The port on which to listen for incoming data
#define NUMBERPADS 2

// Runs during signal interrupt ctrl-c
/*void signal_callback_handler(int signum) {
	std::cout << "Caught signal " << signum <<std::endl;
	tact.~TactilusUDP_L();
	exit(signum);
}*/

int main()
{
    struct timespec start_r, stop_r, curr_time; // initialize structures
    float realtime;
    tactilus_udp_linux::TactilusUDP_L tact = 
        tactilus_udp_linux::TactilusUDP_L(SERVER, PORT, 10, 5, "2"); // 10mm is how far from the back of the foot the y moment will be calculated, 5mm is how far from the inside of the insole the x moment will be calculated, 1 (or 2) is how many sensors are used
    char msg[BUFLEN];
    std::string msgstring;
	
    float force1, momentx1, momenty1;
    float padforce1[NUMBERPADS];
    float force2, momentx2, momenty2;
    float padforce2[NUMBERPADS];
    std::vector<float> forcemoments;
	std::vector<float> forcemomentstemp;
	
    // Register signal and signal hadnler
    //signal(SIGINT, signal_callback_handler);
    // start communication	
	unsigned long recv_counter = 0;
    while (1)
    {
	// The below commented code can be used to send any request that has been implemented on Windows, e.g.
	// "force", "moment10", "pressure", "cop","force,moments10.0,5.0","force,moment10.0"
	/*memset(msg, 0, sizeof(msg));
        printf("Enter message : ");
        if (fgets(msg, sizeof(msg), stdin)) 
        {
            msg[strcspn(msg, "\n")] = '\0';
        }
		
	msgstring = std::string(msg);
	tact.send(msgstring);
        tact.recv();
        puts(tact.getbuf());*/
	
	// Get the starting time
        /*if(clock_gettime(CLOCK_REALTIME,&start_r) == -1) {
	    std::cout << "clock realtime error!" << std::endl;
	}	**/    
		
	//forcemoments = tact.getforcemoments("*");
	//force1 = forcemoments[0];
	//momenty1 = forcemoments[1];
	//momentx1 = forcemoments[2];
	//force2 = forcemoments[3];
	//momenty2 = forcemoments[4];
	//momentx2 = forcemoments[5];
	// Get the starting time
	usleep(4000);
	/*
    if(clock_gettime(CLOCK_REALTIME,&start_r) == -1) {
	    std::cout << "clock realtime error!" << std::endl;
	}*/
	recv_counter++;	
	forcemomentstemp = tact.getforcemoments("1");
	if (forcemomentstemp[0] < 0.0) {
		/*if(clock_gettime(CLOCK_REALTIME,&stop_r) == -1) {
	    std::cout << "clock realtime error!" << std::endl;
		}
		// Compute the difference
		else {
	    realtime = (stop_r.tv_sec - start_r.tv_sec) + ((float)(stop_r.tv_nsec - start_r.tv_nsec) / (float)1e9);
		}*/
        // print the result
		std::cout << recv_counter << " Time: " << realtime << std::endl;
		continue;
	} else {
		while(forcemomentstemp[0] >= 0.0) {
			forcemoments = forcemomentstemp;
			forcemomentstemp = tact.getforcemoments("1");
		}
		/*if(clock_gettime(CLOCK_REALTIME,&stop_r) == -1) {
	    std::cout << "clock realtime error!" << std::endl;
		}
		// Compute the difference
		else {
	    realtime = (stop_r.tv_sec - start_r.tv_sec) + ((float)(stop_r.tv_nsec - start_r.tv_nsec) / (float)1e9);
		}
        // print the result
		std::cout << recv_counter << " Time: " << realtime << std::endl;*/
	}
	
	force1 = forcemoments[0];
	momenty1 = forcemoments[1];
	momentx1 = forcemoments[2];
	for (u_int i = 0; i < NUMBERPADS; ++i){
		padforce1[i] = forcemoments[3+i];
	}
	force2 = forcemoments[3+NUMBERPADS];
	momenty2 = forcemoments[4+NUMBERPADS];
	momentx2 = forcemoments[5+NUMBERPADS];
	for (u_int i = 0; i < NUMBERPADS; ++i){
		padforce2[i] = forcemoments[6+NUMBERPADS+i];
	}
	
        printf("The force is %f N\n", force1);
        printf("The moment about y is %f Nm\n", momenty1);
	printf("The moment about x is %f Nm\n", momentx1);
	printf("The pad forces are");
	for (u_int i = 0; i < NUMBERPADS; ++i){
		printf(" %f N", padforce1[i]);
	}
	printf("\n");
	printf("The second force is %f N\n", force2);
	printf("The second moment about y is %f Nm\n", momenty2);
	printf("The second moment about x is %f Nm\n", momentx2);
	printf("The second set of pad forces are");
	for (u_int i = 0; i < NUMBERPADS; ++i){
		printf(" %f N", padforce2[i]);
	}
	printf("\n");
        //printf("Requesting at ~200Hz\n");
        //sleep(0.005); // I don't think this sleep is necessary? Requesting by itself takes about 3-4 ms per loop
	/*	
        if(clock_gettime(CLOCK_REALTIME,&stop_r) == -1) {
	    std::cout << "clock realtime error!" << std::endl;
	}
	// Compute the difference
	else {
	    realtime = (stop_r.tv_sec - start_r.tv_sec) + ((float)(stop_r.tv_nsec - start_r.tv_nsec) / (float)1e9);
	}
        // print the result
	std::cout << "Time: " << realtime << std::endl;*/

    }
    
    tact.~TactilusUDP_L();
    return 0;
}

