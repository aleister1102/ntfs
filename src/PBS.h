#define UNICODE
#include <cstdint>
#include <cwchar>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <stack>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <tuple>
#include <vector>
#include <windows.h>
using namespace std;

struct Information
{
    BYTE *sector = NULL;
    int length = 0;
    LONG base10_value = -1; // if information needed integer value, then store, -1 for default
};

const size_t vWORD = 2;
const size_t vLONGLONG = 8;
const size_t vBYTE = 1; // value byte
const size_t vDWORD = 4;

struct NTFS_Partition_Boot_Sector
{
    Information oem_id;
    Information bytes_per_sector;
    Information sectors_per_cluster;
    Information sectors_per_track;
    Information logical_cluster_number_mft;
    Information cluster_per_file_record;
    Information total_sector;
    Information logical_cluster_number_mftmirr;
};

bool readSector(LPCWSTR drive, int64_t readPoint, BYTE sector[512]);

// Đọc boot sector
NTFS_Partition_Boot_Sector read(BYTE *sector);
Information read_oem_id(BYTE *sector);
Information read_bytes_per_sector(BYTE *sector);
Information read_sectors_per_cluster(BYTE *sector);
Information read_sector_per_track(BYTE *sector);
Information read_logical_cluster_number_mft(BYTE *sector);
Information read_cluster_per_file_record(BYTE *sector);
Information read_total_sector(BYTE *sector);
Information read_logical_cluster_number_mftmirr(BYTE *sector);

Information extract(BYTE *sector, size_t start, size_t length);
void convertInformationToInt(Information &info);
string convertBase10ToBase16(short number);

void print(NTFS_Partition_Boot_Sector ntfs);
void printInfomation(Information info);

NTFS_Partition_Boot_Sector getNTFSPartitionBootSector();
