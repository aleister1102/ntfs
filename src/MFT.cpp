#include "MFT.h"

EntryHeader EH;
StandardAttributeHeader SAH;
FileNameAttribute FN;
uint16_t *fileName = nullptr;

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

void getEntry(LPCWSTR drive, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, BYTE entry[1024])
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
        return;
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
    writeEntryToFile(entry);
}

void writeEntryToFile(BYTE entry[1024])
{
    FILE *fp = fopen(ENTRY_FILENAME, "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

void printEntries(int quantity)
{
    for (int i = 0; i < quantity; i++)
    {
        BYTE entry[1024];
        getNthEntry(entry, i);

        cout << "entry " << i << ": ";
        int currOffset = STANDARD_INFORMATION_OFFSET;
        readEntryHeader();
        readStandardInformation(currOffset);
        readFileNameAttribute(currOffset);
        cout << endl;
    }
}

void getCurrDirEntries(unsigned int currDirID)
{
    // Khi nào thì MFT kết thúc?
    for (int i = 0; i < 100; i++)
    {
        int isUsed;
        int isDir;
        unsigned int parentID;

        BYTE entry[1024];
        getNthEntry(entry, i);

        int currentOffset = STANDARD_INFORMATION_OFFSET;
        tuple<int, int> tp = readEntryHeader();
        readStandardInformation(currentOffset);
        readFileNameAttribute(currentOffset);
        parentID = readParentID(FN.parentID);

        if (parentID == currDirID && fileName)
        {
            if (!EH.ID && i > 0)
                continue;

            // Loại và trạng thái của entry
            string result = get<1>(tp) ? "Directory " : "File ";
            result += get<0>(tp) ? "is being used" : "";
            cout << result << endl;

            // ID của entry
            cout << "ID: " << EH.ID << endl;

            // parentID của entry
            cout << "Parent ID: " << parentID << endl;

            // Tên của entry
            printFileName(FN.fileNameLength);

            cout << "=================" << endl;
        }

        parentID = -1;
    }
}

tuple<int, int> readEntryHeader()
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fread(&EH, sizeof(EH), 1, fp);

    tuple<int, int> tp = getEntryFlags(EH.flags);

    fclose(fp);
    return tp;
}

tuple<int, int> getEntryFlags(uint16_t flags)
{
    // Bit 0: trạng thái sử dụng
    int isUsed = flags & 1;
    // Bit 1: loại entry (tập tin hoặc thư mục)
    flags = flags >> 1;
    int isDir = flags & 1;

    return make_tuple(isUsed, isDir);
}

void readStandardInformation(int &currentOffset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fseek(fp, currentOffset, SEEK_SET);
    fread(&SAH, sizeof(SAH), 1, fp);
    fclose(fp);

    currentOffset += SAH.totalLength;
}

void readFileNameAttribute(int &currentOffset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fseek(fp, currentOffset, SEEK_SET);
    fread(&SAH, sizeof(SAH), 1, fp);

    // cout << "Attribute type: " << SAH.attributeType << endl;
    // cout << "Attribute total length: " << SAH.totalLength << endl;
    // cout << "Attribute data length: " << SAH.attrDataLength << endl;

    if (SAH.attributeType != 48)
        return;

    fread(&FN, sizeof(FN), 1, fp);

    // printDateTime(convertFileTimeToDateTime(FN.fileCreated), "File created time");
    // printDateTime(convertFileTimeToDateTime(FN.fileModified), "File modified time");
    // printDateTime(convertFileTimeToDateTime(FN.MFTChanged), "MFT changed time");
    // printDateTime(convertFileTimeToDateTime(FN.lastAccess), "Last access time");
    // cout << "File permissions: " << FN.fileAttributes << endl;
    // cout << "File name length: " << (int)FN.fileNameLength << endl;
    // cout << "File name format: " << (int)FN.fileNameFormat << endl;

    readFileName(fp, FN.fileNameLength);

    fclose(fp);
    currentOffset += SAH.totalLength;
}

uint32_t readParentID(char parentID[6])
{
    uint32_t buffer;
    memcpy(&buffer, parentID, 6);
    return buffer;
}

void readFileName(FILE *fp, int fileNameLength)
{
    fileName = new uint16_t[100];
    for (int i = 0; i < fileNameLength; i++)
        fread(&fileName[i], 2, 1, fp);
}

void printFileName(int fileNameLength)
{
    cout << "Name: ";
    for (int i = 0; i < fileNameLength; i++)
        cout << (char)fileName[i];
    cout << endl;

    delete[] fileName;
    fileName = nullptr;
}

void readDataAttribute(int currOffset)
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, currOffset, SEEK_SET);

    DataHeader DAH;
    fread(&DAH, sizeof(DAH), 1, fp);

    fclose(fp);
}

int main(int argc, char **argv)
{

// Đọc các entry của thư mục dựa trên ID
#if 1
    getCurrDirEntries(5);
#endif

// Đọc entry thứ n
#if 0
    BYTE entry[1024];
    getNthEntry(entry, 1);
    readEntryHeader();
    int nextAttrOffset = readStandardInformation(STANDARD_INFORMATION_OFFSET);
    nextAttrOffset = readFileNameAttribute(nextAttrOffset);
#endif

    return 0;
}