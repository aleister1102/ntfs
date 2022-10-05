#define UNICODE

#include <iostream>
#include <stdio.h>
#include <windows.h>
using namespace std;
LPCWSTR INPUT_DRIVE = L"\\\\.\\U:";

int ReadSector(LPCWSTR drive, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, BYTE sector[512])
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

    SetFilePointer(device, lDistanceToMove, lpDistanceToMoveHigh, FILE_BEGIN); // Set a Point to Read

    if (!ReadFile(device, sector, 512, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }
    else
    {
        printf("Success!\n");
    }
}

void writeSectorToFile(BYTE sector[512], const char *sectorName)
{
    FILE *fp;
    fp = fopen(sectorName, "wb");
    fwrite(sector, 1, 512, fp);
    fclose(fp);
}

void getBootSector()
{
    BYTE sector[512];

    ReadSector(INPUT_DRIVE, 0, 0, sector);
    writeSectorToFile(sector, "boot_sector.bin");
}

void getMFTEntry(__int64 startCluster, __int64 sectorPerCluster)
{
    BYTE sector[512];
    LARGE_INTEGER li;

    __int64 startSector = 512 * startCluster * sectorPerCluster;
    li.QuadPart = startSector;

    ReadSector(INPUT_DRIVE, li.LowPart, &li.HighPart, sector);
    writeSectorToFile(sector, "MFT.bin");
}

void readMFTEntry()
{
    FILE *fp = fopen("MFT.bin", "rb");
    char attributeSign[5];

    fseek(fp, 42, SEEK_SET);
    fread(&attributeSign, sizeof(attributeSign), 1, fp);
    fclose(fp);

    cout << attributeSign;
}

int main(int argc, char **argv)
{
    int startCluster = 786432;
    int sectorPerCluster = 8;

    // getBootSector();
    // getMFTEntry(startCluster, sectorPerCluster);
    // readMFTEntry();

    return 0;
}