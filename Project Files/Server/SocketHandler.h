#ifndef SOCKETHANDLER	// Only include this interface once
#define SOCKETHANDLER

#include <string>		// String interface
#include <map>			// Map interface
#include <ws2tcpip.h>	// Winsock Functions

// Forward declare class for function pointer alias
class SocketHandler;
// Incoming message function pointer used as "MessageRecievedHandler"
using MessageRecievedHandlerFP = void(*)(SocketHandler* listener, int socketID, std::string msg);
// Incoming message function pointer used as "NewClientHandler"
using NewClientHandlerFP = void(*)(SocketHandler* listener, int socketID);

// Class designated to handle listener socket, and all clients
class SocketHandler
{
public:
// Public Member Functions
	// Ctor; Instantiating IP, port and message handler from ServerSource.cpp
	SocketHandler(	std::string ipAddress,  int port,	  MessageRecievedHandlerFP mHandler,	NewClientHandlerFP cHandler ) :
					m_ipAddress(ipAddress), m_port(port), MessageRecievedHandler(mHandler),		NewClientHandler(cHandler) {}

	// Dtor closing remaining sockets and terminates use of Ws2_32.dll within ws2_32.lib
	~SocketHandler() { WSACleanup(); }

	bool			initializeWinsock();								// Initialize Winsock
	void			run();												// Recieve loop
	void			Send(int clientSocket, std::string msg)				// Send a message to the specified client
						{ send(clientSocket, msg.c_str(), msg.size() + 1, 0); }
	void			broadcastMSG(SOCKET userSocket, std::string msg);	// Broadcast message to all clients
	std::string&	getClientID(unsigned clientSocket)
			{ return clientID[clientSocket]; }							// Return ID associated with client
	

	void			disconnectClient(SOCKET quittingClient);

	

	/*Header/Handshake protocol*/								/*
		*Note: Everytime the Sender sends a command/data the	**
		*reciever will send back a confirmation of 2 characters	**

		** Send function identifier (4 characters)				**
		** Send file name with extension						**
		** Send total size of file								**
		** Send data in chuncks of 1024 bytes					**
	*//*														*/

	void			sendFile(std::string &filePath, unsigned client);
	

private:
// Private Member Functions
	SOCKET	createListenSocket();								// Create the socket designated as the listener
	void	acceptNewClient();									// Accept incoming client request

	//MESSY
	int		sendVerifyHandshake(SOCKET verificationSocket, int iResult);

// Private Member Variables
	std::string							m_ipAddress;			// IP Address number supplied from ServerSource during construction
	int									m_port;					// Port number supplied from ServerSource during construction
	MessageRecievedHandlerFP			MessageRecievedHandler;	// Function handler for dealing with incoming messages
	NewClientHandlerFP					NewClientHandler;		// Function handler for when a new client connects
	fd_set								master;					// Set of all current connections
	SOCKET								listenSocket;			// Listening socket to accept new clients
	std::map<unsigned, std::string>		clientID;				// Users assigned to this server instance
};

#endif