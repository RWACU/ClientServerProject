#ifndef COMMONSC_H
#define COMMONSC_H

#include <string>
#include <ws2tcpip.h>	// Winsock Functions

class CSC
{
protected:
	// Default Ctor and Dtor
	CSC() = default;
	~CSC() = default;

	// Deny copying Server or Client object, only allow move operation
	CSC(CSC&&) = default;
	CSC& operator=(CSC&&) = default;
	CSC(const CSC&) = delete;
	CSC& operator=(const CSC&) = delete;

	// Update Servers File Descriptor list - Close Client
	virtual void updateSockets(SOCKET directedSocket) = 0;

	// Handle Window
	void clearCinErrors();
	void clearScreen();
	void pause();

	// Send packets sent from calling socket - Any client for ServerHandler, Only server for ClientHandler
	bool	sendPackets(SOCKET& directedSocket, const char* output, uint64_t outputSize, bool packetFlag = TRUE);
	bool	sendPackets(SOCKET& directedSocket, const std::string& output);

	// Recieve packets sent from calling socket - Any client for ServerHandler, Only server for ClientHandler
	bool	recievePackets(SOCKET& directedSocket, char* inputBuffer, bool packetFlag = TRUE);
	bool	recievePackets(SOCKET& directedSocket, std::string& inputBuffer);

	// Send and Recieve a file as a directed 
	void	sendFile(std::string &filePath, SOCKET directedSocket);
	bool	recieveFile(SOCKET& directedSocket);
	
	// Use Argon2ID to encrypt password/salt combination
	std::string&& encryptArgon2ID(std::string& password, std::string& randomstring, char* secretData, unsigned t_cost = 2, unsigned m_cost = 16, unsigned parallelism = 1);
};

#endif