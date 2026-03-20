// lab12_util.exe create <путь>
// lab12_util.exe read   <путь>
// lab12_util.exe write  <путь> <текст>
// lab12_util.exe edit   <путь> <пользователь> <allow|deny> <read|write|full>

#include <windows.h>
#include <aclapi.h>
#include <iostream>
#include <string>

void PrintLastError(const std::string& context)
{
    DWORD err = GetLastError();
    LPSTR msg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, err, 0, (LPSTR)&msg, 0, nullptr);
    std::cerr << "[error] " << context << ": "
              << (msg ? msg : "unknown error")
              << " (code " << err << ")\n";
    LocalFree(msg);
}

int CmdCreate(const std::string& path)
{
    std::cout << "[create] creating: " << path << "\n";

    HANDLE hFile = CreateFileA( 
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_NEW, // только если файла ещё нет
        FILE_ATTRIBUTE_NORMAL, // обычные атрибуты
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFile");
        std::cerr << "[result] access denied or file existing\n";
        return 1;
    }

    CloseHandle(hFile);
    std::cout << "[result] created file\n";
    return 0;
}

int CmdRead(const std::string& path)
{
    std::cout << "[READ] reading: " << path << "\n";

    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING, // файл должен существовать
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFile");
        std::cerr << "[result] access denied or file not found.\n";
        return 1;
    }

    char buffer[512];
    DWORD bytesRead = 0;
    std::cout << "[text] ";
    while (ReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        std::cout << buffer;
    }

    CloseHandle(hFile);
    return 0;
}

int CmdWrite(const std::string& path, const std::string& text)
{
    std::cout << "[write] writing in: " << path << "\n";

    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_ALWAYS, // открыть если есть, создать если нет
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFile");
        std::cerr << "[result] access denied\n";
        return 1;
    }

    SetFilePointer(hFile, 0, nullptr, FILE_END);

    DWORD bytesWritten = 0;
    std::string line = text + "\n";
    WriteFile(hFile, line.c_str(), (DWORD)line.size(), &bytesWritten, nullptr);

    std::cout << "[result] wrote " << bytesWritten << " bytes\n";

    CloseHandle(hFile);
    return 0;
}

int CmdEdit(const std::string& path, const std::string& username, const std::string& aceType, const std::string& accessLevel)
{
    std::cout << "[edit] path: " << path << "\n";
    std::cout << "sub: " << username << "\n";
    std::cout << "type ACE: " << aceType << "\n";
    std::cout << "access: " << accessLevel << "\n";

    DWORD accessMask = 0;
    if      (accessLevel == "read")  accessMask = GENERIC_READ;
    else if (accessLevel == "write")
    accessMask = FILE_WRITE_DATA
               | FILE_APPEND_DATA
               | FILE_WRITE_ATTRIBUTES
               | FILE_WRITE_EA;
    else if (accessLevel == "full")  accessMask = GENERIC_ALL;
    else
    {
        std::cerr << "[error] access level may be: read | write | full\n";
        return 1;
    }

    // SID — уникальный идентификатор субъекта в Windows
    BYTE  sidBuffer[256] = {0};
    DWORD sidSize        = sizeof(sidBuffer);
    char  domainName[256] = {0};
    DWORD domainSize     = sizeof(domainName);
    SID_NAME_USE sidType;

    if (!LookupAccountNameA(
            nullptr,            // локальная машина
            username.c_str(),   // имя пользователя
            sidBuffer,          // буфер для SID
            &sidSize,
            domainName,
            &domainSize,
            &sidType))
    {
        PrintLastError("LookupAccountName");
        std::cerr << "[result] user not found\n";
        return 1;
    }

    PSID pSid = (PSID)sidBuffer;

    PACL  pOldDacl = nullptr;
    PSECURITY_DESCRIPTOR pSD = nullptr;

    DWORD result = GetNamedSecurityInfoA(
        path.c_str(),
        SE_FILE_OBJECT,         // объект — файл или каталог
        DACL_SECURITY_INFORMATION,
        nullptr, nullptr,
        &pOldDacl,              // получаем текущий DACL
        nullptr,
        &pSD                    // дескриптор безопасности
    );

    if (result != ERROR_SUCCESS)
    {
        SetLastError(result);
        PrintLastError("GetNamedSecurityInfo");
        return 1;
    }

    EXPLICIT_ACCESS_A ea = {0};
    ea.grfAccessPermissions = accessMask;
    ea.grfAccessMode = (aceType == "deny") ? DENY_ACCESS : GRANT_ACCESS;
    ea.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName   = (LPSTR)pSid;

    PACL pNewDacl = nullptr;
    result = SetEntriesInAclA(
        1,          // количество новых ACE
        &ea,        // массив новых ACE
        pOldDacl,   // старый DACL (новый ACE добавится к нему)
        &pNewDacl   // результирующий DACL
    );

    if (result != ERROR_SUCCESS)
    {
        SetLastError(result);
        PrintLastError("SetEntriesInAcl");
        LocalFree(pSD);
        return 1;
    }

    result = SetNamedSecurityInfoA(
        (LPSTR)path.c_str(),
        SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION,
        nullptr, nullptr,
        pNewDacl,   // новый DACL
        nullptr
    );

    LocalFree(pSD);
    LocalFree(pNewDacl);

    if (result != ERROR_SUCCESS)
    {
        SetLastError(result);
        PrintLastError("SetNamedSecurityInfo");
        std::cerr << "[result] ACL not updated\n";
        return 1;
    }

    std::cout << "[result] ACL updated\n";
    return 0;
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    if (argc < 3)
    {
        std::cout << "usage:\n"
                  << "  lab12_util.exe create <path>\n"
                  << "  lab12_util.exe read   <path>\n"
                  << "  lab12_util.exe write  <path> <text>\n"
                  << "  lab12_util.exe edit   <path> <user> <allow|deny> <read|write|full>\n";
        return 1;
    }

    std::string cmd  = argv[1];   // команда: create / read / write / edit
    std::string path = argv[2];   // путь к файлу

    if (cmd == "create")
    {
        return CmdCreate(path);
    }
    else if (cmd == "read")
    {
        return CmdRead(path);
    }
    else if (cmd == "write")
    {
        if (argc < 4)
        {
            std::cerr << "[error] write needs <text>\n";
            return 1;
        }
        return CmdWrite(path, argv[3]);
    }
    else if (cmd == "edit")
    {
        if (argc < 6)
        {
            std::cerr << "[error] edit needs: <path> <user> <allow|deny> <read|write|full>\n";
            return 1;
        }
        return CmdEdit(path, argv[3], argv[4], argv[5]);
    }
    else
    {
        std::cerr << "[error] unknown command: " << cmd << "\n";
        return 1;
    }
}