#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

int main() {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "socket error" << std::endl;
        return 1;
    }

    // SO_REUSEADDR — чтобы порт не зависал после перезапуска
    const int enable = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(4444);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // слушаем на всех интерфейсах

    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "bind error" << std::endl;
        return 1;
    }

    listen(serverSock, 1);
    std::cout << "waiting for client..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) continue;

        std::cout << "client connected: " << inet_ntoa(clientAddr.sin_addr) << std::endl;

        std::string filename;
        std::cout << "enter file path to delete: ";
        std::getline(std::cin, filename);

        send(clientSock, filename.c_str(), filename.size(), 0);
        std::cout << "to delete: " << filename << std::endl;

        close(clientSock);
    }

    close(serverSock);
    return 0;
}