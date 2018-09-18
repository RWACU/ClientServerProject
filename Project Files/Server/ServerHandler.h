#ifndef SERVERHANDLER_H	// Only include this interface once
#define SERVERHANDLER_H

#include <string>		// String interface
#include <map>			// Map interface
#include <ws2tcpip.h>	// Winsock Functions
#include "CommonSC.h"

// Forward declare class for function pointer using declarations
class ServerHandler;
// Incoming message function pointer used as "MessageRecievedHandler"
using MessageRecievedHandlerFP 	= void(*)(ServerHandler* listener, int socketID, std::string msg);
// Incoming message function pointer used as "NewClientHandler"
using NewClientHandlerFP 		= void(*)(ServerHandler* listener, int socketID);

// Class designated to handle listener socket, and all clients
class ServerHandler : protected CSC
{
public:
// Public Member Functions
	// Ctor; Instantiating IP, port and message handler from ServerSource.cpp
	ServerHandler(	std::string ipAddress,  int port,	  MessageRecievedHandlerFP mHandler,	NewClientHandlerFP cHandler ) :
					m_ipAddress(ipAddress), m_port(port), MessageRecievedHandler(mHandler),		NewClientHandler(cHandler)
	{
		// Startup Winsock and create servers listening socket
		if (!initializeWinsock() || !createListenSocket())
		{
			WSACleanup();
			exit(-1);
		}
		// Clear any issues with file descriptor container, add newly created listener to container
		FD_ZERO(&master);
		FD_SET(listenSocket, &master);
	}

	// Dtor closing remaining sockets and terminates use of Ws2_32.dll within ws2_32.lib
	~ServerHandler() { WSACleanup(); }

	void			run();												// Recieve loop
	void			broadcastMSG(SOCKET userSocket, std::string msg);	// Broadcast message to all clients
	std::string&	getClientID(unsigned clientSocket)					// Return ID associated with client
					{ return clientID[clientSocket]; }
	
	void			disconnectClient(SOCKET quittingClient);

private:
// Private Member Functions
	bool		initializeWinsock();								// Initialize Winsock
	bool		createListenSocket();								// Create the socket designated as the listener
	void		acceptNewClient();									// Accept incoming client request
	bool		clientLogin(SOCKET newClientSocket, char host[NI_MAXHOST]);

	void		updateSockets(SOCKET directedSocket) override { disconnectClient(directedSocket); }
	bool		checkUsernameExists(std::string& username);
	bool		addNewUser(std::string& username, std::string& hPassword);
	std::string&&	sendRandomNumber(SOCKET directedSocket);

	bool		encryptRecieveVerify(std::string& hPassword, std::string& randomNumber, bool usersExistance, char* secretData, SOCKET newClient);

// Private Member Variables

	// In Decending Order (Alignment - Frequency of use)
	SOCKET							listenSocket;			// Listening socket to accept new clients
	fd_set							master;					// Set of all current connections
	std::map<unsigned, std::string>	clientID;				// Users assigned to this server instance
	MessageRecievedHandlerFP		MessageRecievedHandler;	// Function handler for dealing with incoming messages
	NewClientHandlerFP				NewClientHandler;		// Function handler for when a new client connects
	std::string						m_ipAddress;			// IP Address number supplied from ServerSource during construction
	int								m_port;					// Port number supplied from ServerSource during construction
};

#endif