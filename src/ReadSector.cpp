#include "NTFS.h"

SYSTEMTIME convertFileTimeToDateTime(uint64_t filetime)
{

    long long value = filetime;
    FILETIME ft = {0};

    ft.dwHighDateTime = (value & 0xffffffff00000000) >> 32;
    ft.dwLowDateTime = value & 0xffffffff;

    SYSTEMTIME sys = {0};
    FileTimeToSystemTime(&ft, &sys);

    return sys;
}

void printDateTime(SYSTEMTIME datetime, string prefix = "")
{
    string day[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    string month[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

    cout << prefix << ": " << day[datetime.wDayOfWeek] << "," << month[datetime.wMonth - 1] << " " << datetime.wDay << "," << datetime.wYear << " " << datetime.wHour << ":" << datetime.wMinute << ":" << datetime.wSecond << endl;
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
        // printf("Success!\n");
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
    FILE *fp = fopen(ENTRY_FILENAME, "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

void getSystemEntries()
{
    for (int i = 0; i < 16; i++)
    {
        BYTE entry[1024];
        getNthEntry(entry, i);
        writeEntryToFile(entry);

        cout << "entry " << i << ": ";
        int nextAttrOffset = readStandardInformation(STANDARD_INFORMATION_OFFSET);
        nextAttrOffset = readFileNameAttribute(nextAttrOffset);
    }
}

int readStandardInformation(int attrOffset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fseek(fp, attrOffset, SEEK_SET);

    StandardAttributeHeader sah;
    fread(&sah, sizeof(sah), 1, fp);

    fclose(fp);

    int nextAttrOffset = attrOffset + sah.totalLength;
    return nextAttrOffset;
}

int readFileNameAttribute(int attrOffset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fseek(fp, attrOffset, SEEK_SET);

    StandardAttributeHeader sah;
    fread(&sah, sizeof(sah), 1, fp);

    // cout << "Attribute type: " << sah.attributeType << endl;
    // cout << "Attribute total length: " << sah.totalLength << endl;
    // cout << "Attribute data length: " << sah.attrDataLength << endl;

    if (sah.attributeType != 48)
        return attrOffset + sah.totalLength;

    FileName fn;
    fread(&fn, sizeof(fn), 1, fp);

    // printDateTime(convertFileTimeToDateTime(fn.fileCreated), "File created time");
    // printDateTime(convertFileTimeToDateTime(fn.fileModified), "File modified time");
    // printDateTime(convertFileTimeToDateTime(fn.MFTChanged), "MFT changed time");
    // printDateTime(convertFileTimeToDateTime(fn.lastAccess), "Last access time");
    // cout << "File permissions: " << fn.fileAttributes << endl;
    // cout << "File name length: " << (int)fn.fileNameLength << endl;
    // cout << "File name format: " << (int)fn.fileNameFormat << endl;

    uint16_t fileName[100];
    readFileName(fp, fileName, (int)fn.fileNameLength);
    printFileName(fileName, (int)fn.fileNameLength);

    fclose(fp);

    int nextAttrOffset = attrOffset + sah.totalLength;
    return nextAttrOffset;
}

void readFileName(FILE *fp, uint16_t fileName[], int fileNameLength)
{
    for (int i = 0; i < fileNameLength; i++)
        fread(&fileName[i], 2, 1, fp);
}

void printFileName(uint16_t fileName[], int fileNameLength)
{
    for (int i = 0; i < fileNameLength; i++)
        cout << (char)fileName[i];

    cout << endl;
}

void readDataAttribute(int attrOffset)
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, attrOffset, SEEK_SET);

    DataHeader dah;
    fread(&dah, sizeof(dah), 1, fp);

    fclose(fp);
}

int main(int argc, char **argv)
{

// Đọc nhiều entry
#if 1
    getSystemEntries();
#endif

// Đọc entry thứ n
#if 0
    BYTE entry[1024];
    getNthEntry(entry, 12);
    writeEntryToFile(entry);
    int nextAttrOffset = readStandardInformation(STANDARD_INFORMATION_OFFSET);
    nextAttrOffset = readFileNameAttribute(nextAttrOffset);
#endif

    return 0;
}