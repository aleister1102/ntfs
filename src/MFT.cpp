#include "MFT.h"

vector<int> directoryStack = {5};
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

void readEntryHeader(EntryBuffers &buffers, int &offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    fread(&buffers.EH, sizeof(buffers.EH), 1, fp);
    fclose(fp);

    offset += buffers.EH.offsetToFirstAttr;
}

void readStandardInformation(EntryBuffers &buffers, int &offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);
    fclose(fp);

    offset += buffers.SAH.totalLength;
}

void readFileNameAttribute(EntryBuffers &buffers, int &offset)
{
    FILE *fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);

    if (buffers.SAH.attributeType != 48)
        return;

    fread(&buffers.FNA, sizeof(buffers.FNA), 1, fp);
    readFileName(buffers, fp);
    fclose(fp);

    offset += buffers.SAH.totalLength;
}

void readStandardAttributeHeader(EntryBuffers &buffers, FILE *fp, int &offset)
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

void parseFileName(Entry &entry, EntryBuffers buffers)
{
    int fileNameLength = buffers.FNA.fileNameLength;

    char *buffer = new char[fileNameLength + 1];
    for (int i = 0; i < fileNameLength; i++)
        buffer[i] = (char)buffers.fileNameBuffer[i];
    buffer[fileNameLength] = '\0';

    entry.entryName = string(buffer);
    delete[] buffers.fileNameBuffer;
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

void readDataAttribute(int currOffset)
{
    FILE *fp = fopen("MFT.bin", "rb");
    fseek(fp, currOffset, SEEK_SET);

    DataAttributeHeader DH;
    fread(&DH, sizeof(DH), 1, fp);
    fclose(fp);

    currOffset += DH.totalLength;
}

void readEntry(Entry &entry)
{
    int offset = 0;
    EntryBuffers buffers;

    readEntryHeader(buffers, offset);
    readStandardInformation(buffers, offset);
    readFileNameAttribute(buffers, offset);

    if (buffers.fileNameBuffer != nullptr)
    {
        parseFileName(entry, buffers);
    }
    parseEntryFlags(entry, buffers);
    parseParentIDs(entry, buffers);
}

void printCurrDir()
{
    cout << "-----------------------------------------------------------------------------" << endl;
    cout << "Type\tStatus\t\tID\tName" << endl;

    for (auto entry : entries)
        printEntry(entry);

    cout << "-----------------------------------------------------------------------------" << endl;
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
    for (int i = 0; i < directoryStack.size(); i++)
    {
        int dirID = directoryStack.at(i);
        for (int i = 0; i < entries.size(); i++)
        {
            if (entries.at(i).ID == dirID)
            {
                if (entries.at(i).entryName != ".")
                {
                    cout << entries.at(i).entryName << "\\";
                    break;
                }
            }
        }
    }

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
        getCurrDir(directoryStack.back());
    else if (args[0] == "cd")
    {
        if (args.size() < 2)
            cout << "Missing input" << endl;
        else if (args[1] == "..")
        {
            if (directoryStack.back() != 5)
                directoryStack.pop_back();
            return 1;
        }
        else
        {
            int entryID;
            if (validateInputDirectory(args[1], directoryStack.back(), entryID))
                directoryStack.push_back(entryID);
        }
    }
    else if (args[0] == "cat")
    {
        if (args.size() < 2)
            cout << "Missing input" << endl;
        else
        {
            string input = args[1];
            // readFileContent(input);
        }
    }
    else if (args[0] == "exit")
        return 0;
    else
        cout << "Can not regconize '" << args[0] << "' command" << endl;

    return 1;
}

bool validateInputDirectory(string input, int parentID, int &ID)
{
    Entry entryInfos = findEntryInfos(input, directoryStack.back());
    bool valid = true;

    if (entryInfos.ID < 0)
    {
        cout << "Can not find " << input << endl;
        valid = false;
    }
    else if (!entryInfos.isDir)
    {
        cout << input << " is not directory" << endl;
        valid = false;
    }
    else
        ID = entryInfos.ID;

    return valid;
}

Entry findEntryInfos(string dirName, int parentID)
{
    for (auto entry : entries)
    {
        if (entry.parentID == parentID && entry.entryName == dirName)
            return entry;
    }

    return Entry();
}

// void readFileContent(string input)
// {
//     Entry entryInfos = findEntryInfos(input, directoryStack.back());

//     if (entryInfos.ID < 0)
//     {
//         cout << "Can not find " << input << endl;
//         return;
//     }

//     BYTE entry[1024];
//     FILE *fp = fopen(ENTRY_FILENAME, "rb");
//     int offset = STANDARD_INFORMATION_OFFSET;

//     getNthEntryAndWriteToFile(entryInfos.ID);
//     readEntryHeader();

//     do
//     {
//         if (SAH.attributeType == 0x80)
//             printFileContent(fp, offset);

//         readStandardAttributeHeader(fp, offset);
//         // offset += SAH.totalLength;

//     } while (SAH.attributeType != 0xFFFFFFFF);
// }

// void printFileContent(FILE *fp, int currentOffset)
// {
//     int dataOffset = currentOffset - SAH.totalLength + sizeof(StandardAttributeHeader);
//     fseek(fp, dataOffset, SEEK_SET);

//     char buffer;
//     for (int i = 0; i < SAH.attrDataLength; i++)
//     {
//         fread(&buffer, 1, 1, fp);
//         cout << buffer;
//     }

//     cout << endl;
// }

int main(int argc, char **argv)
{
    getCurrDir(5);
    printCurrDir();

    return 0;
}