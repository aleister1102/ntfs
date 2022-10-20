#define UNICODE
#pragma pack(1) // Yêu cầu compiler cấp phát bộ nhớ đúng với kdl (không padding)

#include <cstdint>
#include <cwchar>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <tuple>
#include <vector>
#include <windows.h>
using namespace std;

const wchar_t *CURRENT_DRIVE = L"\\\\.\\U:";

// Các thông số cơ bản của MFT
int ROOT_DIR = 5;
int $MFT_INDEX = 0;
int MFT_LIMIT = 0;

// Signature của các attribute
unsigned int FILE_NAME = 0x30;
unsigned int DATA = 0x80;
unsigned int END_MARKER = 0xFFFFFFFF;

// Tên tập tin lưu entry
const char *ENTRY_FILENAME = "entry.bin";

// Ba hằng số dưới đây là lấy từ phần VBR
long long START_CLUSTER = 786432;
long SECTOR_PER_CLUSTER = 8;
long SECTOR_SIZE = 512;

struct EntryHeader
{
    uint32_t signature;
    uint16_t offsetToUpdateSeq;
    uint16_t updateSeqSize;
    uint64_t logFileSeqNum;
    uint16_t seqNum;
    uint16_t hardLinkCount;
    uint16_t offsetToFirstAttr;
    uint16_t flags;
    uint32_t realSizeOfFileRecord;
    uint32_t allocatedSizeOfFileRecord;
    uint64_t baseFileRecord;
    uint16_t nextAttrID;
    uint16_t unused;
    uint32_t ID;
    uint16_t updateSeqNumber;
    uint32_t updateSeqArray;
};

struct StandardAttributeHeader
{
    uint32_t attributeType;
    uint32_t totalLength;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attrID;
    uint32_t attrDataLength;
    uint16_t offsetToAttrData;
    uint8_t indexFlag;
    uint8_t padding;
};

struct FileNameAttribute
{
    char parentID[6];
    char parentSeqNum[2];
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t MFTChanged;
    uint64_t lastAccess;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint32_t fileAttributes;
    uint32_t reparse;
    uint8_t fileNameLength;
    uint8_t fileNameFormat;
};

struct NonResidentDataAttributeHeader
{
    uint32_t attributeType;
    uint32_t totalLength;
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

struct DataRun
{
    uint64_t clusterCount = 0;
    uint64_t clusterOffset = 0;
};

struct EntryBuffers
{
    EntryHeader EH;
    StandardAttributeHeader SAH;
    NonResidentDataAttributeHeader NRDAH;

    FileNameAttribute FNA;
    uint16_t *fileNameBuffer = nullptr;
    uint16_t *attrNameBuffer = nullptr;
};

struct Entry
{
    string entryName;
    unsigned int ID = -1;
    unsigned int parentID;
    int isDir;
    int isUsed;
    string data;
    vector<DataRun> dataRuns;
};

void getEntry(LPCWSTR drive, uint64_t readPoint, BYTE entry[1024]);
void writeEntryToFile(BYTE entry[1024]);

// Entry header
void readEntry(Entry &entry);
void readEntryHeader(EntryBuffers &buffers, int &offset);
void parseEntryFlags(Entry &entry, EntryBuffers &buffers);

// Attribute header
void readAttributeIdentifiers(uint32_t &signature, uint32_t &length, uint8_t &nonResidentFlag, int offset);
void readStandardAttributeHeader(EntryBuffers &buffers, FILE *fp, int offset);

// File name
void readFileNameAttribute(EntryBuffers &buffers, int offset);
void readFileName(EntryBuffers &buffers, FILE *fp);
void parseParentIDs(Entry &entry, EntryBuffers &buffers);
void parseFileName(Entry &entry, EntryBuffers &buffers);

// Data
void readNonResidentDataAttributeHeader(EntryBuffers &buffers, FILE *fp, int offset);
void readNonResidentData(EntryBuffers &buffers, Entry &entry, int offset);
void readDataRuns(Entry &entry, FILE *fp);
void readResidentData(EntryBuffers &buffers, Entry &entry, int offset);
void readData(EntryBuffers &buffers, Entry &entry, FILE *fp);

// Xuất ra thông tin của entry
void printEntry(Entry entry);

// Kiểm soát các câu lệnh
void printDirStack();
vector<string> split(const string &s, char delim = ' ');
int handleCommands(vector<string> args);
Entry findEntry(string dirName);

// Validate input
bool validateInputDirectory(Entry &entry, string input);
bool checkExistence(Entry entry, string input);
bool checkDirectory(Entry entry);

// Lấy số entry tối đa
void getMFTLimit();