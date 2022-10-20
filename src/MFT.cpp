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
    auto fp = fopen(ENTRY_FILENAME, "wb");
    fwrite(entry, 1, 1024, fp);
    fclose(fp);
}

void getCurrDir(int currDirID = ROOT_DIR)
{
    for (int i = 0; i < MFT_LIMIT; i++)
    {
        Entry entry;
        getNthEntryAndWriteToFile(i);
        readEntry(entry);

        // Tìm các entry có tên và nằm trong thư mục hiện tại
        if (entry.entryName != "" && entry.parentID == currDirID)
            entries.push_back(entry);
    }
}

void readEntry(Entry &entry)
{
    int offset = 0;
    EntryBuffers buffers;
    uint32_t signature = 0, length = 0;
    uint8_t nonResidentFlag = 0;

    readEntryHeader(buffers, offset);
    parseEntryFlags(entry, buffers);
    do
    {
        readAttributeIdentifiers(signature, length, nonResidentFlag, offset);

        // File name
        if (signature == FILE_NAME)
        {
            readFileNameAttribute(buffers, offset);

            parseParentIDs(entry, buffers);
            parseFileName(entry, buffers);
        }
        // Data
        else if (signature == DATA)
        {
            if (nonResidentFlag)
                readNonResidentData(buffers, entry, offset);
            else
                readResidentData(buffers, entry, offset);
        }

        offset += length;
    } while (signature != END_MARKER && signature != 0x0);
}

void readEntryHeader(EntryBuffers &buffers, int &offset)
{
    auto fp = fopen(ENTRY_FILENAME, "rb");
    fread(&buffers.EH, sizeof(buffers.EH), 1, fp);
    fclose(fp);

    offset += buffers.EH.offsetToFirstAttr;
}

void parseEntryFlags(Entry &entry, EntryBuffers &buffers)
{
    auto flags = buffers.EH.flags;

    // Bit 0: trạng thái sử dụng
    entry.isUsed = flags & 1;
    // Bit 1: loại entry (tập tin hoặc thư mục)
    flags = flags >> 1;
    entry.isDir = flags & 1;
}

void readAttributeIdentifiers(uint32_t &signature, uint32_t &length, uint8_t &nonResidentFlag, int offset)
{
    auto fp = fopen(ENTRY_FILENAME, "rb");
    fseek(fp, offset, SEEK_SET);
    fread(&signature, sizeof(signature), 1, fp);
    fread(&length, sizeof(length), 1, fp);
    fread(&nonResidentFlag, sizeof(nonResidentFlag), 1, fp);
    fclose(fp);
}

void readStandardAttributeHeader(EntryBuffers &buffers, FILE *fp, int offset)
{
    fseek(fp, offset, SEEK_SET);
    fread(&buffers.SAH, sizeof(buffers.SAH), 1, fp);
}

void readFileNameAttribute(EntryBuffers &buffers, int offset)
{
    auto fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);

    fread(&buffers.FNA, sizeof(buffers.FNA), 1, fp);
    readFileName(buffers, fp);
    fclose(fp);
}

void readFileName(EntryBuffers &buffers, FILE *fp)
{
    buffers.fileNameBuffer = new uint16_t[buffers.FNA.fileNameLength];

    for (int i = 0; i < buffers.FNA.fileNameLength; i++)
        fread(&buffers.fileNameBuffer[i], sizeof(uint16_t), 1, fp);
}

void parseParentIDs(Entry &entry, EntryBuffers &buffers)
{
    char *parentID = buffers.FNA.parentID;
    int parentIDLength = 6;

    uint32_t buffer;
    memcpy(&buffer, parentID, parentIDLength);
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
    delete buffers.fileNameBuffer;
    buffers.fileNameBuffer = nullptr;

    entry.entryName = string(buffer);
}

void readNonResidentDataAttributeHeader(EntryBuffers &buffers, FILE *fp, int offset)
{
    fseek(fp, offset, SEEK_SET);
    fread(&buffers.NRDAH, sizeof(buffers.NRDAH), 1, fp);
}

void readNonResidentData(EntryBuffers &buffers, Entry &entry, int offset)
{
    auto fp = fopen(ENTRY_FILENAME, "rb");
    readNonResidentDataAttributeHeader(buffers, fp, offset);
    readDataRuns(entry, fp);
    fclose(fp);
}

void readDataRuns(Entry &entry, FILE *fp)
{
    uint8_t size = 0;
    do
    {
        fread(&size, sizeof(size), 1, fp);

        if (size == 0)
            break;

        int clusterCountSize = size & 0xF;
        int clusterOffsetSize = (size >> 4) & 0xF;

        DataRun DR;
        // Lấy 4 bit thấp, là số lượng cluster dành cho data run
        fread(&DR.clusterCount, clusterCountSize, 1, fp);
        // Lấy 4 bit cao, là cluster bắt đầu của data run
        fread(&DR.clusterOffset, clusterOffsetSize, 1, fp);

        entry.dataRuns.push_back(DR);
    } while (size != 0x0);

    fclose(fp);
}

void readResidentData(EntryBuffers &buffers, Entry &entry, int offset)
{
    auto fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);

    // Xử lý trường hợp attribute có tên
    if (buffers.SAH.nameLength)
        fseek(fp, offset + buffers.SAH.offsetToAttrData, SEEK_SET);
    if (buffers.SAH.attrDataLength)
        readData(buffers, entry, fp);
    fclose(fp);
}

void readData(EntryBuffers &buffers, Entry &entry, FILE *fp)
{
    int dataLength = buffers.SAH.attrDataLength;
    char *data = new char[dataLength]{0};
    int i = 0;

    do
    {
        fread(&data[i], sizeof(char), 1, fp);
    } while (data[i++] != 0x00);

    fclose(fp);
    entry.data += (string)data;
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
        auto args = split(command);
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
        result.push_back(item);

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
            if (directoryStack.back().ID != ROOT_DIR)
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
                readTextFile(entry);
                cout << entry.data << endl;
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
    if (entry.ID == END_MARKER)
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

void readTextFile(Entry &entry)
{
    int offset = 0;
    EntryBuffers buffers;
    uint32_t signature = 0, length = 0;
    uint8_t nonResidentFlag = 0;

    getNthEntryAndWriteToFile(entry.ID);
    readEntryHeader(buffers, offset);
    do
    {
        readAttributeIdentifiers(signature, length, nonResidentFlag, offset);
        if (signature == DATA)
            readTextData(buffers, entry, offset);

        offset += length;
    } while (signature != END_MARKER);
}

void readTextData(EntryBuffers &buffers, Entry &entry, int offset)
{
    auto fp = fopen(ENTRY_FILENAME, "rb");
    readStandardAttributeHeader(buffers, fp, offset);

    char *data = new char[buffers.SAH.attrDataLength];
    fread(data, buffers.SAH.attrDataLength, 1, fp);
    fclose(fp);

    data[buffers.SAH.attrDataLength] = '\0';
    entry.data += (string)data;
}

void init()
{
    // Lấy thông tin của thư mục gốc
    Entry entry;
    getNthEntryAndWriteToFile(ROOT_DIR);
    readEntry(entry);
    directoryStack.push_back(entry);

    // Lấy số lượng entry tối đa của bảng MFT
    getMFTLimit();

    // Lấy các entry của thư mục gốc
    getCurrDir(directoryStack.back().ID);
}

void getMFTLimit()
{
    Entry entry;
    getNthEntryAndWriteToFile($MFT_INDEX);
    readEntry(entry);

    int MFT_CLUSTERS = entry.dataRuns.at(0).clusterCount;
    MFT_LIMIT = MFT_CLUSTERS * SECTOR_PER_CLUSTER / 2;
}

int main(int argc, char **argv)
{
    init();
    menu();
    return 0;
}