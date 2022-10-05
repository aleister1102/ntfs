#define UNICODE

#include <stdio.h>
#include <windows.h>
LPCWSTR INPUT_DRIVE = L"\\\\.\\U:";

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
    ReadSector(INPUT_DRIVE, 0, sector);
    writeSectorToFile(sector, "boot_sector.bin");
}

void getMFTEntry(int sectorPerCluster, long startCluster)
{
    BYTE sector[512];
    ReadSector(INPUT_DRIVE, (startCluster * sectorPerCluster - 1) * 512, sector);
    writeSectorToFile(sector, "MFT.bin");
}

int main(int argc, char **argv)
{
    // getBootSector();
    getMFTEntry(8, 768432);

    return 0;
}