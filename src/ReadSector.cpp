#include "NTFS.h"

static SYSTEMTIME convertFileTimeToDateTime(uint64_t filetime)
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

int getEntry(LPCWSTR drive, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, BYTE entry[1024])
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

    if (!ReadFile(device, entry, 1024, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }
    else
    {
        printf("Success!\n");
    }
}

void getNthEntry(BYTE entry[1024], int entryOffset = 0)
{
    LARGE_INTEGER li;

    uint64_t startSector = START_CLUSTER * SECTOR_PER_CLUSTER * SECTOR_SIZE;
    uint64_t bypassSector = entryOffset * 2 * SECTOR_SIZE;
    uint64_t readSector = startSector + bypassSector;
    li.QuadPart = readSector;

    getEntry(INPUT_DRIVE, li.LowPart, &li.HighPart, entry);
}

void writeEntryToFile(BYTE entry[1024])
{
    FILE *fp = fopen("entry.bin", "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

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
    BYTE MFT[1024];
    getNthEntry(MFT, 1);
    writeEntryToFile(MFT);

    return 0;
}