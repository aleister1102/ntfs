#define UNICODE

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
int FILE_NAME_OFFSET = 152;

struct StandardAttributeHeader
{
    uint32_t signature;
    uint32_t length;
    uint8_t nonResidentFlag;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;
    uint16_t attrID;
    uint32_t attrLength;
    uint16_t offsetToAttrData;
    uint8_t indexFlag;
    uint8_t padding;
};

struct StandardInformation
{
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t MFTChanged;
    uint64_t lastAccessTime;
    uint32_t filePermissions;
};

struct FileName
{
    uint64_t fileReferenceToParentDir;
    uint64_t fileCreated;
    uint64_t fileModified;
    uint64_t MFTChanged;
    uint64_t lastAccessTime;
    uint64_t allocatedSize;
    uint64_t realSize;
    uint32_t fileAttributes;
    uint32_t reparse;
    uint8_t fileNameLength;
    uint8_t fileNameFormat;
};

struct DataHeader
{
    uint32_t signature;
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
void readFileName(FILE *fp, uint16_t fileName[], int fileNameLength);
