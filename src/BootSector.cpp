#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <iomanip>
#include <stack>
#include <string>
using namespace std;

struct Infomation {
    BYTE* sector = NULL;
    int length = 0;
    int base10_value = -1; //if infomation needed integer value, then store, -1 for default
};

const size_t vWORD = 2;
const size_t vLONGLONG = 8;
const size_t vBYTE = 1; //value byte
const size_t vDWORD = 4;

struct NTFS_Partition_Boot_Sector {
    Infomation oem_id;
    Infomation bytes_per_sector;
    Infomation sectors_per_cluster;
    Infomation sectors_per_track;
    Infomation logical_cluster_number_mft;
    Infomation cluster_per_file_record;
};

int ReadSector(LPCWSTR  drive, int64_t readPoint, BYTE sector[512])
{
    int retCode = 0;
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile(drive,    // Drive to open
        GENERIC_READ,           // Access mode
        FILE_SHARE_READ | FILE_SHARE_WRITE,        // Share Mode
        NULL,                   // Security Descriptor
        OPEN_EXISTING,          // How to create
        0,                      // File attributes
        NULL);                  // Handle to template

    if (device == INVALID_HANDLE_VALUE) // Open Error
    {
        printf("CreateFile: %u\n", GetLastError());
        return 1;
    }

    if (readPoint <= LONG_MAX) {
        SetFilePointer(device, readPoint, NULL, FILE_BEGIN);//Set a Point to Read when Distance is smaller than LONG_MAX
    }
    else {
        LARGE_INTEGER li;
        li.QuadPart = readPoint;
        SetFilePointer(device, li.LowPart, &li.HighPart, FILE_BEGIN);//Set a Point to Read when Distance is bigger than LONG_MAX
        //cout << li.LowPart << "-" << li.HighPart << endl;
    }

    if (!ReadFile(device, sector, 512, &bytesRead, NULL))
    {
        printf("ReadFile: %u\n", GetLastError());
    }
    else
    {
        printf("Success!\n");
    }
}
string convertBase10ToBase16(short number) {
    string result;
    stack<short> s;
    short remainder = -1;
    do {
        remainder = number % 16;
        number = number / 16;
        s.push(remainder);
    } while (number > 0);
    size_t size = s.size();
    if (size < 2) {
        s.push(0);
        size = s.size();
    }
    for (size_t i = 0; i < size; i++) {
        short value = s.top();
        if (value >= 0 && value <= 9) result.push_back((char)(value + 48));
        else result.push_back((char)(value + 65 - 10));
        s.pop();
    }
    return result;
}
void convertInfomationToInt(Infomation& info) {
    string s = "0x";
    for (int i = info.length - 1; i >= 0; i--) {
        s.append(convertBase10ToBase16(info.sector[i]));
    }
    //cout << s;
    int value = stoi(s, nullptr, 0);
    info.base10_value = value;
}
//////////////////////////////////////////

Infomation extract(BYTE* sector, size_t start, size_t length) {
    Infomation res;
    res.length = length;
    res.sector = new BYTE[length];
    for (size_t i = start; i < start + length; i++) {
        res.sector[i - start] = sector[i];
    }
    return res;
}

void printInfomation(Infomation info) {
    if (info.base10_value == -1) {
        for (size_t i = 0; i < info.length; i++) {
            cout << info.sector[i];
        }
    }
    else {
        cout << info.base10_value;
    }
    cout << endl;
}

Infomation read_oem_id(BYTE* sector) {
    Infomation oem_id = extract(sector, stoi("0x03", nullptr, 0), vLONGLONG);

    return oem_id;
}

Infomation read_bytes_per_sector(BYTE* sector) {
    Infomation bps = extract(sector, stoi("0x0B", nullptr, 0), vWORD);
    convertInfomationToInt(bps);

    return bps;
}

Infomation read_sectors_per_cluster(BYTE* sector) {
    Infomation spc = extract(sector, stoi("0x0D", nullptr, 0), vBYTE);
    convertInfomationToInt(spc);

    return spc;
}

Infomation read_sector_per_track(BYTE* sector) {
    Infomation spt = extract(sector, stoi("0x18", nullptr, 0), vWORD);
    convertInfomationToInt(spt);

    return spt;
}

Infomation read_logical_cluster_number_mft(BYTE* sector) {
    Infomation lcn_mft = extract(sector, stoi("0x30", nullptr, 0), vLONGLONG);
    convertInfomationToInt(lcn_mft);

    return lcn_mft;
}

Infomation read_cluster_per_file_record(BYTE* sector) {
    Infomation cpfr = extract(sector, stoi("0x40", nullptr, 0), vDWORD);
    convertInfomationToInt(cpfr);

    return cpfr;
}

NTFS_Partition_Boot_Sector read(BYTE* sector) {
    NTFS_Partition_Boot_Sector ntfs;
    ntfs.oem_id = read_oem_id(sector);
    ntfs.bytes_per_sector = read_bytes_per_sector(sector);
    ntfs.sectors_per_cluster = read_sectors_per_cluster(sector);
    ntfs.sectors_per_track = read_sector_per_track(sector);
    ntfs.logical_cluster_number_mft = read_logical_cluster_number_mft(sector);
    ntfs.cluster_per_file_record = read_cluster_per_file_record(sector);
    return ntfs;
}

void print(NTFS_Partition_Boot_Sector ntfs) {
    cout << "OEM ID: "; printInfomation(ntfs.oem_id);
    cout << "Bytes per sector: "; printInfomation(ntfs.bytes_per_sector);
    cout << "Sectors per cluster: "; printInfomation(ntfs.sectors_per_cluster);
    cout << "Sectors per track: "; printInfomation(ntfs.sectors_per_track);
    cout << "Logical cluster number for MFT: "; printInfomation(ntfs.logical_cluster_number_mft);
    cout << "Cluster per file record: "; printInfomation(ntfs.cluster_per_file_record);
}

void printSector(BYTE* sector, size_t start, size_t length) {
    for (size_t i = start; i < start + length; i++) {
        cout << convertBase10ToBase16(sector[i]) << " ";
    }
}

int main(int argc, char** argv)
{

    BYTE sector[512];
    ReadSector(L"\\\\.\\E:", 0, sector);
    NTFS_Partition_Boot_Sector ntfs = read(sector);
    print(ntfs);


    /*
    int k = 0;
    for (short i : sector) {
        string base_16 = convertBase10ToBase16(i);
        cout << setw(3) << base_16;
        k++;
        if (k > 16) {
            k = 0;
            cout << endl;
        }
    }
    */
    return 0;
}



/*
TODO:
1. Implement the rest of Partition Boot Sector
2. Read Sector: add an input from user for Disk_Name
*/