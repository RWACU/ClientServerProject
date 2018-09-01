#ifndef SOCKETHANDLER	// Only include this interface once
#define SOCKETHANDLER

#include <string>		// String interface
#include <map>			// Map interface
#include <ws2tcpip.h>	// Winsock Functions

// Forward declare class for function pointer alias
class SocketHandler;

// Incoming message function pointer used as "MessageRecievedHandler"
using MessageRecievedHandlerFP = void(*)(SocketHandler* listener, int socketID, std::string msg);

// Class designated to handle listener socket, and all clients
class SocketHandler
{
public:
// Public Member Functions
	// Ctor; Instantiating IP, port and message handler from ServerSource.cpp
	SocketHandler(	std::string ipAddress,  int port,	  MessageRecievedHandlerFP handler) :
					m_ipAddress(ipAddress), m_port(port), MessageRecievedHandler(handler) {}

	// Dtor closing remaining sockets and terminates use of Ws2_32.dll within ws2_32.lib
	~SocketHandler() { WSACleanup(); }

	
	bool initializeWinsock();									// Initialize Winsock
	void run();													// Recieve loop
	void Send(int clientSocket, std::string msg)				// Send a message to the specified client
			{ send(clientSocket, msg.c_str(), msg.size() + 1, 0); }

private:
// Private Member Functions
	SOCKET	createListenSocket();								// Create the socket designated as the listener
	void	broadcastMSG(SOCKET userSocket, std::string msg);	// Broadcast message to all clients
	void	acceptNewClient();									// Accept incoming client request

// Private Member Variables
	std::string							m_ipAddress;			// IP Address number supplied from ServerSource during construction
	int									m_port;					// Port number supplied from ServerSource during construction
	MessageRecievedHandlerFP			MessageRecievedHandler;	// Function handler for dealing with incoming messages
	fd_set								master;					// Set of all current connections
	SOCKET								listenSocket;			// Listening socket to accept new clients
	std::map<unsigned, std::string>		users;					// Users assigned to this server instance
};

#endif