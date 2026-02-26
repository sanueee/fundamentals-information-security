#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

int main() {
    // AF_INET = IPv4, SOCK_STREAM = TCP 
    // Константа AF_INET соответствует Internet-домену. Сокеты, размещённые в этом домене, могут использоваться для работы в любой IP-сети.
    // SOCK_STREAM - Передача потока данных с предварительной установкой соединения. Для реализации этого параметра используется протокол TCP.
    int sock = socket(AF_INET, SOCK_STREAM, 0); // socket() запрашивает у ОС свободный номер индекса файлового дескриптора, после чего возвращает его.
    if (sock < 0) {
        std::cerr << "creating socket error" << std::endl;
        return 1;
    }

    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(4444); // порт клиента
    inet_pton(AF_INET, "192.168.91.134", &clientAddr.sin_addr); // IP клиента

    if (connect(sock, (sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        std::cerr << "connection error" << std::endl;
        return 1;
    }
    std::cout << "connected" << std::endl;

    const char* filename = "C:\\Users\\sanueee\\Desktop\\2sem\\lab4\\test.txt";
    int bytes_sent = send(sock, filename, strlen(filename), 0);
    if (bytes_sent == -1) {
        std::cerr << "sending error: " << strerror(errno) << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "sent file: " << filename << std::endl;
}   