#define UNICODE

#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <windows.h>
using namespace std;

LPCWSTR INPUT_DRIVE = L"\\\\.\\U:";

uint16_t swap_uint16(uint16_t val);
uint32_t swap_uint32(uint32_t val);
uint64_t swap_uint64(uint64_t val);
SYSTEMTIME convertFileTimeToDateTime(uint64_t filetime);

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

void getMFTEntry(int64_t startCluster, int64_t sectorPerCluster)
{
    BYTE sector[512];
    LARGE_INTEGER li;

    int64_t startSector = 512 * startCluster * sectorPerCluster;
    li.QuadPart = startSector;

    ReadSector(INPUT_DRIVE, li.LowPart, &li.HighPart, sector);
    writeSectorToFile(sector, "MFT.bin");
}

struct StandardInformationHeader
{
    uint32_t signature;
    uint32_t length;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
};

struct StandardInformation
{
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t recordChanged;
    uint64_t lastAccessTime;
    uint32_t filePermissions;
};

void readStandardInformation()
{
    FILE *fp = fopen("MFT.bin", "rb");

    StandardInformationHeader sih;
    fseek(fp, 56, SEEK_SET);
    fread(&sih, sizeof(sih), 1, fp);

    swap_uint32(sih.signature);
    swap_uint32(sih.length);
    swap_uint16(sih.nameOffset);
    cout << sih.signature << endl;
    cout << sih.length << endl;
    cout << sih.nonResidentFlag << endl;
    cout << sih.nameLength << endl;
    cout << sih.nameOffset << endl;

    StandardInformation si;
    fseek(fp, 80, SEEK_SET);
    fread(&si, sizeof(si), 1, fp);

    swap_uint64(si.fileCreated);
    swap_uint64(si.fileModified);
    swap_uint64(si.recordChanged);
    swap_uint64(si.lastAccessTime);
    swap_uint32(si.filePermissions);
    convertFileTimeToDateTime(si.fileCreated);
    convertFileTimeToDateTime(si.fileModified);
    convertFileTimeToDateTime(si.recordChanged);
    convertFileTimeToDateTime(si.lastAccessTime);
    cout << si.filePermissions << endl;

    fclose(fp);
}

SYSTEMTIME convertFileTimeToDateTime(uint64_t filetime)
{
    const char *day[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    const char *month[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

    long long value = filetime;
    FILETIME ft = {0};

    ft.dwHighDateTime = (value & 0xffffffff00000000) >> 32;
    ft.dwLowDateTime = value & 0xffffffff;

    SYSTEMTIME sys = {0};
    FileTimeToSystemTime(&ft, &sys);

    cout << day[sys.wDayOfWeek] << "," << month[sys.wMonth - 1] << " " << sys.wDay << "," << sys.wYear << " " << sys.wHour << ":" << sys.wMinute << ":" << sys.wSecond << endl;

    return sys;
}

uint16_t swap_uint16(uint16_t val)
{
    return (val << 8) | (val >> 8);
}

uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

uint64_t swap_uint64(uint64_t val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

int main(int argc, char **argv)
{
    long startCluster = 786432;
    long sectorPerCluster = 8;

    // getBootSector();
    // getMFTEntry(startCluster, sectorPerCluster);
    // readStandardInformation();


    return 0;
}