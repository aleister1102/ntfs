#define UNICODE

#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <windows.h>
//-----------------//
#include <stack>
#include <string>
using namespace std;
int ReadSector(LPCWSTR drive, int readPoint, BYTE sector[512])
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive,                              // Drive to open
                        GENERIC_READ,                       // Access mode
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // Share Mode
                        NULL,                               // Security Descriptor
                        OPEN_EXISTING,                      // How to create
                        0,                                  // File attributes
                        NULL);                              // Handle to template

    if (device == INVALID_HANDLE_VALUE) // Open Error
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    SetFilePointer(device, readPoint, NULL, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, 512, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }
    else
    {
        printf("Success!\n");
    }
}
string convertBase10ToBase16(short number)
{
    string result;
    stack<short> s;
    short remainder = -1;
    do
    {
        remainder = number % 16;
        number = number / 16;
        s.push(remainder);
    } while (number > 0);
    size_t size = s.size();
    if (size < 2)
    {
        s.push(0);
        size = s.size();
    }
    for (size_t i = 0; i < size; i++)
    {
        short value = s.top();
        if (value >= 0 && value <= 9)
            result.push_back((char)(value + 48));
        else
            result.push_back((char)(value + 65 - 10));
        s.pop();
    }
    return result;
}

int main(int argc, char **argv)
{
    BYTE sector[512];
    ReadSector(L"\\\\.\\E:", 0, sector);
    int k = 0;
    for (short i : sector)
    {
        string base_16 = convertBase10ToBase16(i);
        cout << setw(3) << base_16;
        k++;
        if (k > 16)
        {
            k = 0;
            cout << endl;
        }
    }
    return 0;
}