#define UNICODE
// Yêu cầu compiler cấp phát bộ nhớ đúng với kdl (không padding)
#pragma pack(1)

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <windows.h>
using namespace std;

LPCWSTR INPUT_DRIVE = L"\\\\.\\U:";
const char *ENTRY_FILENAME = "entry.bin";
long long START_CLUSTER = 786432;
long SECTOR_PER_CLUSTER = 8;
long SECTOR_SIZE = 512;
int STANDARD_INFORMATION_OFFSET = 56;
int FILE_NAME_OFFSET = 128;

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

struct FileName
{
    uint64_t fileReferenceToParentDir;
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

struct DataHeader
{
    uint32_t attributeType;
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
int readEntryHeader();
void checkEntryFlags(uint16_t flags);
int readStandardInformation(int attrOffset);
int readFileNameAttribute(int attrOffset);
void readFileName(FILE *fp, uint16_t fileName[], int fileNameLength);
void printFileName(uint16_t fileName[], int fileNameLength);