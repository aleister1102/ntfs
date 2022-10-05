#define UNICODE

#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <windows.h>
using namespace std;

LPCWSTR INPUT_DRIVE = L"\\\\.\\U:";

SYSTEMTIME convertFileTimeToDateTime(uint64_t filetime);
void printFileName(char fileName[]);

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

struct StandardAttributeHeader
{
    uint32_t signature;
    uint32_t length;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attrID;
    uint32_t attrLength;
    uint16_t offsetToAttrData;
    uint8_t indexFlag;
    uint8_t padding;
};

struct StandardInformation
{
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t MFTChanged;
    uint64_t lastAccessTime;
    uint32_t filePermissions;
};

void readStandardInformation()
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, 56, SEEK_SET);

    StandardAttributeHeader sah;
    fread(&sah, sizeof(sah), 1, fp);

    cout << sah.signature << endl;
    cout << sah.length << endl;
    cout << sah.attrLength << endl;

    StandardInformation si;
    fread(&si, sizeof(si), 1, fp);

    convertFileTimeToDateTime(si.fileCreated);
    convertFileTimeToDateTime(si.fileModified);
    convertFileTimeToDateTime(si.MFTChanged);
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

struct FileName
{
    uint64_t fileReferenceToParentDir;
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t MFTChanged;
    uint64_t lastAccessTime;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint32_t fileAttributes;
    uint32_t reparse;
    uint8_t fileNameLength;
    uint8_t fileNameFormat;
    char fileName[14];
};

void readFileNameAttribute()
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, 152, SEEK_SET);

    StandardAttributeHeader sah;
    fread(&sah, sizeof(sah), 1, fp);

    cout << sah.signature << endl;
    cout << sah.length << endl;
    cout << sah.attrLength << endl;

    FileName fn;
    fread(&fn, sizeof(fn), 1, fp);

    convertFileTimeToDateTime(fn.fileCreated);
    convertFileTimeToDateTime(fn.fileModified);
    convertFileTimeToDateTime(fn.MFTChanged);
    convertFileTimeToDateTime(fn.lastAccessTime);
    cout << fn.fileAttributes << endl;
    cout << fn.reparse << endl;
    cout << (int)fn.fileNameLength << endl;
    cout << (int)fn.fileNameFormat << endl;
    printFileName(fn.fileName);

    fclose(fp);
}

void printFileName(char fileName[])
{
    for (int i = 0; i < 14; i += 2)
    {
        cout << fileName[i] << fileName[i + 1];
    }
}

struct DataHeader
{
    uint32_t signature;
    uint32_t length;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attrID;
    uint64_t firstVCN;
    uint64_t lastVCN;
    uint16_t dataRunsOffset;
    uint16_t compressionUnitSize;
    uint32_t padding;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint64_t initializedSize;
};

void readDataAttribute()
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, 256, SEEK_SET);

    StandardAttributeHeader dah;
    fread(&dah, sizeof(dah), 1, fp);

    cout << dah.signature << endl;
    cout << dah.length << endl;
    cout << (int)dah.nonResidentFlag << endl;
}

int main(int argc, char **argv)
{
    long startCluster = 786432;
    long sectorPerCluster = 8;

    // getBootSector();
    // getMFTEntry(startCluster, sectorPerCluster);
    // readStandardInformation();
    // readFileNameAttribute();
    // readDataAttribute();

    return 0;
}