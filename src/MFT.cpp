#include "MFT.h"

vector<Entry> directoryStack;
vector<Entry> entries;

void getNthEntryAndWriteToFile(int entryOffset = 0)
{
    BYTE buffer[1024];
    LARGE_INTEGER li;

    uint64_t startSector = START_CLUSTER * SECTOR_PER_CLUSTER * SECTOR_SIZE;
    uint64_t bypassSector = entryOffset * 2 * SECTOR_SIZE;
    uint64_t readSector = startSector + bypassSector;
    li.QuadPart = readSector;

    getEntry(CURRENT_DRIVE, li.LowPart, &li.HighPart, buffer);
    writeEntryToFile(buffer);
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

void writeEntryToFile(BYTE entry[1024])
{
    FILE *fp = fopen(ENTRY_FILENAME, "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

void getCurrDir(int currDirID = 5)
{
    // Khi nào thì MFT kết thúc?
    for (int i = 0; i < 100; i++)
    {
        Entry entry;

        getNthEntryAndWriteToFile(i);
        readEntry(entry);

        // Tìm các entry có tên và nằm trong thư mục hiện tại
        if (entry.parentID == currDirID && entry.entryName != "")
        {
            // Bỏ qua các entry không phải là file hoặc thư mục
            if (!entry.ID && i > 0)
                continue;
            else
                entries.push_back(entry);
        }
    }
}

void readEntry(Entry &entry)
{
    int offset = 0;
    EntryBuffers buffers;
    uint32_t signature = 0, length = 0;

    readEntryHeader(buffers, offset);
    do
    {
        readAttributeSignatureAndLength(signature, length, offset);
        // Standard Information
        if (signature == 0x10)
        {
            readStandardInformation(buffers, offset);
        }
        // File name
        else if (signature == 0x30)
        {
            readFileNameAttribute(buffers, offset);

            parseParentIDs(entry, buffers);
            parseEntryFlags(entry, buffers);
            parseFileName(entry, buffers);
        }

        offset += length;
    } while (signature != 0xFFFFFFFF && signature != 0x0);
}

void readEntryHeader(EntryBuffers &buffers, int &offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fread(&buffers.EH, sizeof(buffers.EH), 1, fp);
    fclose(fp);

    offset += buffers.EH.offsetToFirstAttr;
}

void readAttributeSignatureAndLength(uint32_t &signature, uint32_t &length, int offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fseek(fp, offset, SEEK_SET);
    fread(&signature, sizeof(signature), 1, fp);
    fread(&length, sizeof(length), 1, fp);
    fclose(fp);
}

void readStandardInformation(EntryBuffers &buffers, int offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);
    fclose(fp);
}

void readFileNameAttribute(EntryBuffers &buffers, int offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);

    fread(&buffers.FNA, sizeof(buffers.FNA), 1, fp);
    readFileName(buffers, fp);
    fclose(fp);
}

void readStandardAttributeHeader(EntryBuffers &buffers, FILE *fp, int offset)
{
    fseek(fp, offset, SEEK_SET);
    fread(&buffers.SAH, sizeof(buffers.SAH), 1, fp);
}

void readFileName(EntryBuffers &buffers, FILE *fp)
{
    buffers.fileNameBuffer = new uint16_t[100];

    for (int i = 0; i < buffers.FNA.fileNameLength; i++)
        fread(&buffers.fileNameBuffer[i], 2, 1, fp);
}

void parseEntryFlags(Entry &entry, EntryBuffers &buffers)
{
    uint16_t flags = buffers.EH.flags;

    // Bit 0: trạng thái sử dụng
    entry.isUsed = flags & 1;
    // Bit 1: loại entry (tập tin hoặc thư mục)
    flags = flags >> 1;
    entry.isDir = flags & 1;
}

void parseParentIDs(Entry &entry, EntryBuffers &buffers)
{
    char *parentID = buffers.FNA.parentID;

    uint32_t buffer;
    memcpy(&buffer, parentID, 6);
    entry.parentID = buffer;
    entry.ID = buffers.EH.ID;
}

void parseFileName(Entry &entry, EntryBuffers &buffers)
{
    int fileNameLength = buffers.FNA.fileNameLength;

    // Chuyển tên file thành mảng char
    char *buffer = new char[fileNameLength + 1];
    for (int i = 0; i < fileNameLength; i++)
        buffer[i] = (char)buffers.fileNameBuffer[i];
    buffer[fileNameLength] = '\0';

    // Giải phóng bộ nhớ cũ
    buffers.fileNameBuffer = nullptr;
    delete[] buffers.fileNameBuffer;

    entry.entryName = string(buffer);
}

void printCurrDir()
{
    cout << "-----------------------------------------------------------------------------" << endl;
    cout << "Type\tStatus\t\tID\tName" << endl;

    for (auto entry : entries)
        printEntry(entry);

    cout << "-----------------------------------------------------------------------------" << endl;
}

void printEntry(Entry entry)
{
    // Loại và trạng thái của entry
    string isDir = entry.isDir ? "dir" : "file";
    cout << isDir << "\t";
    string isUsed = entry.isUsed ? "being used" : "";
    cout << isUsed << "\t";

    // ID của entry
    cout << entry.ID << "\t";

    // Tên của entry
    cout << entry.entryName << endl;
}

void menu()
{
    int running = true;

    do
    {
        wcout << CURRENT_DRIVE[4] << "\\";
        printDirStack();

        string command;
        getline(cin, command);
        vector<string> args = split(command);
        running = handleCommands(args);
    } while (running);
}

void printDirStack()
{
    for (auto dir : directoryStack)
        cout << dir.entryName << "\\";
    cout << " > ";
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
        printCurrDir();
    else if (args[0] == "cd")
    {
        if (args.size() < 2)
            cout << "Missing input" << endl;
        else if (args[1] == "..")
        {
            if (directoryStack.back().ID != 5)
            {
                directoryStack.pop_back();
                entries.clear();
                getCurrDir(directoryStack.back().ID);
            }
        }
        else
        {
            Entry entry;
            string input = args[1];
            if (validateInputDirectory(entry, input))
            {
                directoryStack.push_back(entry);
                entries.clear();
                getCurrDir(directoryStack.back().ID);
            }
        }
    }
    else if (args[0] == "cat")
    {
        if (args.size() < 2)
            cout << "Missing input" << endl;
        else
        {
            Entry entry;
            string input = args[1];
            if (validateTextFile(entry, input))
            {
                printTextFileContent(entry);
            }
        }
    }
    else if (args[0] == "exit")
        return 0;
    else
        cout << "Can not regconize '" << args[0] << "' command" << endl;

    return 1;
}

bool validateInputDirectory(Entry &entry, string input)
{
    entry = findEntry(input);
    bool valid = checkExistence(entry, input) && checkDirectory(entry);
    return valid;
}

Entry findEntry(string dirName)
{
    int parentID = directoryStack.back().ID;
    for (auto entry : entries)
    {
        if (entry.entryName == dirName && entry.parentID == parentID)
            return entry;
    }

    return Entry();
}

bool checkExistence(Entry entry, string input)
{
    if (entry.ID == 0xFFFFFFFF)
    {
        cout << "Can not find " << input << endl;
        return false;
    }
    return true;
}

bool checkDirectory(Entry entry)
{
    if (!entry.isDir)
    {
        cout << entry.entryName << " is not directory" << endl;
        return false;
    }
    return true;
}

bool validateTextFile(Entry &entry, string input)
{
    entry = findEntry(input);
    bool valid = checkExistence(entry, input) && checkTextFile(entry);
    return valid;
}

bool checkTextFile(Entry entry)
{
    if (entry.entryName.substr(entry.entryName.length() - 4) != ".txt")
    {
        cout << entry.entryName << " is not a text file" << endl;
        return false;
    }
    return true;
}

void printTextFileContent(Entry entry)
{
    
}

void init()
{
    Entry entry;
    getNthEntryAndWriteToFile(5);
    readEntry(entry);

    directoryStack.push_back(entry);
    getCurrDir(directoryStack.back().ID);
}

int main(int argc, char **argv)
{
    init();
    menu();

    return 0;
}