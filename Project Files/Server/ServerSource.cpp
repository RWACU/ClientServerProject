#include <iostream>
#include <string>
#include "ServerHandler.h"								// Server interface

// Constant expressions to be used throughout Client //
constexpr char IPADDRESS[] = "###.###.###.###";			// IP Address of the server
constexpr unsigned PORT = 10000;						// Listening port # on the Server

// When server recieves a message from a client 
void MessageRecievedHandler(ServerHandler *server, int client, std::string inputBuffer)
{
	std::cout << "Paste the filepath to the file you would like to send..." << std::endl;
	std::string filePath;
	std::cin >> filePath;
	//server->sendFile(filePath, client);
}

// When server recieves a message from a client 
void NewClientHandler(ServerHandler *server, int client)
{
	//// Send a welcome message to the connected client
	std::string welcomeMemberMsg = "Welcome to the Server";
	//server->Send(client, std::move(welcomeMemberMsg));
}

int main()
{
	// Create new server through SocketHandler interface
	ServerHandler server(IPADDRESS, PORT, &MessageRecievedHandler, &NewClientHandler);
		
	return 0;
}