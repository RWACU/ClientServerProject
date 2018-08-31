#include <iostream>
#include <string>
#include <ws2tcpip.h>	//Socket Library for windows
#include "TCPListener.h"

#pragma comment (lib, "ws2_32.lib")

/*    TODO    *//*
	-Get rid of extraneous information
	-Rename variables and functions to more meaningful names
		(The current ones were used as it was based on a tutorial
		and the dichotomy between variables names could have caused
		issues
	-Redo comments to explain rationale
	-Add better error handling in Listener class
*/			  /**/

void Listener_MessageRecieved(TCPListener *listener, int client, std::string msg)
{
	listener->Send(client, msg);
}

int main()
{
	TCPListener server("192.168.0.11", 54010, Listener_MessageRecieved);

	if (server.Init())
	{
		server.run();
	}
		
	return 0;
}