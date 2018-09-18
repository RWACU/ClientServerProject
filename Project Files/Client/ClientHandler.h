#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <string>		// String interface
#include <map>			// Map interface
#include <ws2tcpip.h>	// Winsock Functions
#include "CommonSC.h"

// Forward declare class for function pointer alias
class ClientHandler;

// Incoming message function pointer used as "MessageRecievedHandler"
using MessageRecievedHandlerFP = void(*)(ClientHandler* listener, int socketID, std::string msg);
using UsernameSpecHandlerFP = bool(*)(std::string username);
using PasswordSpecHandlerFP = bool(*)(std::string password);

// Class designated to handle listener socket, and all clients
class ClientHandler : protected CSC
{
public:
// Automatic Member Functions
	// Ctor Instantiating ipAddress, port and message handler from ClientSource.cpp
	ClientHandler(std::string initIPAddress,     int initPort, MessageRecievedHandlerFP MH, UsernameSpecHandlerFP US, PasswordSpecHandlerFP PS) :
				  	ipAddress(initIPAddress), 	port(initPort),  MessageRecievedHandler(MH), UsernameSpecHandler(US), PasswordSpecHandler(PS)
				  	{
				  		initializeWinsock();
				  		clientConnect();
				  	}
	// Dtor closing socket and terminates use of Ws2_32.dll within ws2_32.lib
	~ClientHandler() { closesocket(serverSocket); WSACleanup(); }

// Public Member Functions

	// Starting Member Function (User has control of running processes through Function Pointer Handlers)
	bool	runClient();
	
private:
// Private Member Functions

	// Initialize Connection Member Functions
	bool	initializeWinsock();
	bool	clientConnect();

	std::string&& encryptRawPassword(std::unique_ptr<std::string>& unencryptedPassword, std::string& username);

	// Encryption Member Functions
	bool 	encryptSendVerify(std::string& hPassword, std::string& serverRandomNumber);
	bool	enterUsernamePassword(std::string& username, std::unique_ptr<std::string>& password, bool newUserFlag);

	void	updateSockets(SOCKET directedSocket) override;

	bool	verifyUsername(std::string& username);
	bool	verifyPassword(std::string& password);

// Private Member Variables
	
	// In Decending Order (Alignment - Frequency of use)
	SOCKET						serverSocket;			// Socket to communicate with the server
	MessageRecievedHandlerFP	MessageRecievedHandler;	// Function Pointer for dealing with incoming messages
	UsernameSpecHandlerFP		UsernameSpecHandler;	
	PasswordSpecHandlerFP		PasswordSpecHandler;
	int							port;					// Port number supplied from ClientSource.cpp during construction
	std::string					ipAddress;				// IP Address number supplied from ClientSource.cpp during construction
};

#endif