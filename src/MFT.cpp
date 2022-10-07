#include "MFT.h"

int CURRENT_DIR_ID = 5;
EntryHeader EH;
StandardAttributeHeader SAH;
FileNameAttribute FNA;
uint16_t *FILE_NAME = nullptr;

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

        int currOffset = STANDARD_INFORMATION_OFFSET;
        unsigned int parentID;

        readEntryHeader();
        readStandardInformation(currOffset);
        readFileNameAttribute(currOffset);
        parentID = readParentID(FNA.parentID);

        cout << "Entry " << i << endl;

        // ID của entry
        cout << "ID: " << EH.ID << endl;

        // parentID của entry
        cout << "Parent ID: " << parentID << endl;

        // Tên của entry
        printFileName(FNA.fileNameLength);

        cout << "=================" << endl;
    }
}

void listCurrDir(unsigned int currDirID = 5)
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

        readEntryHeader();
        tuple<int, int> tp = readEntryFlags(EH.flags);
        readStandardInformation(currentOffset);
        readFileNameAttribute(currentOffset);
        parentID = readParentID(FNA.parentID);

        if (parentID == currDirID && FILE_NAME)
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
            printFileName(FNA.fileNameLength);

            cout << "=================" << endl;
        }

        parentID = -1;
    }
}

void readEntryHeader()
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fread(&EH, sizeof(EH), 1, fp);
    fclose(fp);
}

tuple<int, int> readEntryFlags(uint16_t flags)
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

    if (SAH.attributeType != 48)
        return;

    fread(&FNA, sizeof(FNA), 1, fp);
    readFileName(fp, FNA.fileNameLength);
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
    FILE_NAME = new uint16_t[100];

    for (int i = 0; i < fileNameLength; i++)
        fread(&FILE_NAME[i], 2, 1, fp);
}

void printFileName(int fileNameLength)
{
    cout << "Name: ";
    for (int i = 0; i < fileNameLength; i++)
        cout << (char)FILE_NAME[i];
    cout << endl;

    delete[] FILE_NAME;
    FILE_NAME = nullptr;
}

void readDataAttribute(int currOffset)
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, currOffset, SEEK_SET);

    DataAttributeHeader DH;
    fread(&DH, sizeof(DH), 1, fp);
    fclose(fp);

    currOffset += DH.totalLength;
}

void menu()
{
    do
    {
        string DRIVE = convertWideCharToString(INPUT_DRIVE);
        cout << DRIVE << "\\" << CURRENT_DIR_ID << "\\"
             << ">";

        string COMMAND;
        getline(cin, COMMAND);
        handleCommands(COMMAND);
    } while (1);
}

string convertWideCharToString(const wchar_t *characters)
{
    wstring ws(characters);
    string str(ws.begin(), ws.end());
    return str;
}

void handleCommands(string command)
{
    if (command == "cls")
        system("cls");
    if (command == "ls")
        listCurrDir(CURRENT_DIR_ID);
    else
        cout << "Can not regconize " << command << endl;
}

int main(int argc, char **argv)
{

// Đọc các entry của thư mục dựa trên ID
#if 0
    listCurrDir();
#endif

// Đọc entry thứ n
#if 0
    BYTE entry[1024];
    getNthEntry(entry, 1);
    readEntryHeader();
    int nextAttrOffset = readStandardInformation(STANDARD_INFORMATION_OFFSET);
    nextAttrOffset = readFileNameAttribute(nextAttrOffset);
#endif

    menu();

    return 0;
}