#include "MFT.h"

EntryHeader EH;
StandardAttributeHeader SAH;
FileNameAttribute FNA;
uint16_t *FILE_NAME = nullptr;

const wchar_t *CURRENT_DRIVE = L"\\\\.\\D:";
vector<int> DIR_STACK = {5};

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

    getEntry(CURRENT_DRIVE, li.LowPart, &li.HighPart, entry);
    writeEntryToFile(entry);
}

void writeEntryToFile(BYTE entry[1024])
{
    FILE *fp = fopen(ENTRY_FILENAME, "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

void listCurrDir(unsigned int currDirID = 5)
{
    cout << "-----------------------------------------------------------------------------" << endl;
    cout << "Type\tStatus\t\tID\tName" << endl;

    // Khi nào thì MFT kết thúc?
    for (int i = 0; i < 100; i++)
    {
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
            string isDir = get<0>(tp) ? "dir" : "file";
            cout << isDir << "\t";
            string isUsed = get<1>(tp) ? "being used" : "";
            cout << isUsed << "\t";

            // ID của entry
            cout << EH.ID << "\t";

            // Tên của entry
            printFileName(FNA.fileNameLength);

            cout << endl;
        }
        parentID = -1;
    }
    cout << "-----------------------------------------------------------------------------" << endl;
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

    return make_tuple(isDir, isUsed);
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
    for (int i = 0; i < fileNameLength; i++)
        cout << (char)FILE_NAME[i];

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
    int running = true;

    do
    {
        wcout << CURRENT_DRIVE[4] << " > ";
        printCurrDir();

        string command;
        getline(cin, command);
        vector<string> args = split(command);
        running = handleCommands(args);
    } while (running);
}

void printCurrDir()
{
    for (int i = 0; i < DIR_STACK.size(); i++)
        cout << DIR_STACK.at(i) << "\\";
}

vector<string> split(const string &s, char delim)
{
    vector<string> result;
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

int handleCommands(vector<string> args)
{
    if (args[0] == "cls")
        system("cls");
    else if (args[0] == "dir")
        listCurrDir(DIR_STACK.back());
    else if (args[0] == "cd")
    {
        if (args[1] == "..")
        {
            if (DIR_STACK.back() != 5)
                DIR_STACK.pop_back();
            return 1;
        }

        if (stoi(args[1]))
            DIR_STACK.push_back(stoi(args[1])); // cần kiểm soát lỗi
    }
    else if (args[0] == "exit")
        return 0;
    else
        cout << "Can not regconize " << args[0] << endl;

    return 1;
}

int main(int argc, char **argv)
{
    menu();

    return 0;
}