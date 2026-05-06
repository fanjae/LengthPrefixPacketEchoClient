#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma warning (disable:4996)

bool RecvAll(SOCKET sock, char* buffer, int size) // 지정한 size 바이트를 전부 받을때까지 반복 수신
{
	int received = 0;

	while (received < size)
	{
		int ret = recv(sock, buffer + received, size - received, 0);
		// buffer + received : 저장 시작 위치
		// size - received : 앞으로 더 받아야 할 바이트 수

		// ret == 0 : 연결 종료, recv < 0 : 연결 에러
		if (ret <= 0)
			return false;

		// 바이트 수 누적
		received += ret;
	}
	return true;
}
bool SendAll(SOCKET sock, const char* buffer, int size)  // 지정한 size 바이트를 전부 보낼때까지 반복 송신
{
	int sent = 0;

	while (sent < size)
	{
		int ret = send(sock, buffer + sent, size - sent, 0);
		// buffer + received : 전송 시작 위치
		// size - sent : 앞으로 더 보내야할 바이트 수


		if (ret == SOCKET_ERROR)
			return false;

		// 바이트 수 누적
		sent += ret;
	}

	return true;
}

int main()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 소켓 연결
	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(7777);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cout << "Connect Failed.\n";
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::cout << "Connected to server.\n";

	while (true)
	{
		std::string message;
		std::cout << "> ";
		std::getline(std::cin, message);

		if (message == "exit")
			break;
		
		uint32_t bodyLength = static_cast<uint32_t>(message.size());
		uint32_t headLength = htonl(bodyLength);


		// 1. 길이 헤더 전송
		if (!SendAll(sock, reinterpret_cast<char*>(&headLength), sizeof(headLength)))
		{
			break;
		}

		// 2. 본문 전송
		if (!SendAll(sock, message.data(), bodyLength))
		{
			break;
		}

		// 3. 에코 응답 길이 헤더 수신
		uint32_t echoHeadLength = 0;

		if (!RecvAll(sock, reinterpret_cast<char*>(&echoHeadLength), sizeof(echoHeadLength)))
		{
			break;
		}
		
		uint32_t echoBodyLength = ntohl(echoHeadLength);

		if (echoBodyLength == 0 || echoBodyLength > 4096)
		{
			std::cout << "Invalid echo size.\n";
			break;
		}

		// 4. 에코 본문 수신
		std::vector<char> echoBody(echoBodyLength);

		if (!RecvAll(sock, echoBody.data(), echoBodyLength))
		{
			break;
		}

		std::string echoMessage(echoBody.begin(), echoBody.end());
		std::cout << "Echo : " << echoMessage << "\n";
	}

	closesocket(sock);
	WSACleanup();

	return 0;
}