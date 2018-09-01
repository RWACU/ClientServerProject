#include "SocketHandler.h"	// Server interface

// Constant expressions to be used throughout Client //
//constexpr char IPADDRESS[] = "###.###.###.###";			// IP Address of the server
constexpr char IPADDRESS[] = "192.168.0.11";			// IP Address of the server
constexpr unsigned PORT = 54010;							// Listening port # on the Server

// When server recieves a message from a client 
void MessageRecievedHandler(SocketHandler *server, int client, std::string msg)
{
	server->Send(client, msg);
}

int main()
{
	// Create new server through SocketHandler interface
	SocketHandler server(IPADDRESS, PORT, MessageRecievedHandler);

	// Run server if it has initialized properly; exit with error code "-1" if it has not
	if (server.initializeWinsock())
	{
		server.run();
	}
	else
	{
		return -1;
	}
		
	return 0;
}