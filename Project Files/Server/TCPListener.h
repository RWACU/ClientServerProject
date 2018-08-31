 #pragma once

#include <string>
#include <map>

#include <ws2tcpip.h>					// Winsock Functions

#pragma comment(lib, "ws2_32.lib")		// Winsock Library fuke

// Forward declare class
class TCPListener;

typedef void(*MessageRecievedHandler)(TCPListener* listener, int socketID, std::string msg);

// TODO: callback to data recieved

class TCPListener
{
public:

	TCPListener(	std::string ipAddress,  int port,     MessageRecievedHandler handler) :
					m_ipAddress(ipAddress), m_port(port), MessageRecieved(handler) {}

	~TCPListener() { WSACleanup(); }

	// Send a message to the specified client
	void Send(int clientSocket, std::string msg) { send(clientSocket, msg.c_str(), msg.size() + 1, 0); }

	//initialize Winsock
	bool Init();

	// Recieve loop
	void run();

private:
	// Create a Socket
	SOCKET createListenSocket();

	//Broadcast message to all clients
	void broadcastMSG(SOCKET sock, std::string msg);

	void acceptNewClient();

	std::string								m_ipAddress;
	int										m_port;
	MessageRecievedHandler					MessageRecieved;
	fd_set									master;
	SOCKET									listening;
	std::map<unsigned int, std::string>		users;
};