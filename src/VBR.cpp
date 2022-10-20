#include <iomanip>
#include <iostream>
#include <stack>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <windows.h>
using namespace std;

struct Information
{
    BYTE *sector = NULL;
    int length = 0;
    LONG base10_value = -1; // if information needed integer value, then store, -1 for default
};

struct File_Name_Attribute
{
    // Define header of attribute
    int type;
    int length;
    string name;   // From type -> name
    bool resilent; // 0 for non-resilent, 1 for resilent
    int offset_to_data;
    // Data
    int file_attribute;
    string file_name;
};

struct Data_Attribute
{
    // Define header of attribute
    int type;
    int length;
    string name;   // From type -> name
    bool resilent; // 0 for non-resilent, 1 for resilent
    int offset_to_data;
    // Data
    int length_of_file;
    string content;
};

struct MFT_Entry
{
    Information signature;
    Information offset_first_attribute;
    Information flag; // is folder or file
    File_Name_Attribute file_name_attribute;
    Data_Attribute data_attribute;
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

int ReadSector(LPCWSTR drive, int64_t readPoint, BYTE sector[512])
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
        return 1;
    }

    if (readPoint <= LONG_MAX)
    {
        SetFilePointer(device, readPoint, NULL, FILE_BEGIN); // Set a Point to Read when Distance is smaller than LONG_MAX
    }
    else
    {
        LARGE_INTEGER li;
        li.QuadPart = readPoint;
        SetFilePointer(device, li.LowPart, &li.HighPart, FILE_BEGIN); // Set a Point to Read when Distance is bigger than LONG_MAX
        // cout << li.LowPart << "-" << li.HighPart << endl;
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
string convertBase10ToBase16(short number)
{
    string result;
    stack<short> s;
    short remainder = -1;
    do
    {
        remainder = number % 16;
        number = number / 16;
        s.push(remainder);
    } while (number > 0);
    size_t size = s.size();
    if (size < 2)
    {
        s.push(0);
        size = s.size();
    }
    for (size_t i = 0; i < size; i++)
    {
        short value = s.top();
        if (value >= 0 && value <= 9)
            result.push_back((char)(value + 48));
        else
            result.push_back((char)(value + 65 - 10));
        s.pop();
    }
    return result;
}
void convertInformationToInt(Information &info)
{
    string s = "0x";
    for (int i = info.length - 1; i >= 0; i--)
    {
        s.append(convertBase10ToBase16(info.sector[i]));
    }
    // cout << s;
    int value = stoi(s, nullptr, 0);
    info.base10_value = value;
}
//////////////////////////////////////////

Information extract(BYTE *sector, size_t start, size_t length)
{
    Information res;
    res.length = length;
    res.sector = new BYTE[length];
    for (size_t i = start; i < start + length; i++)
    {
        res.sector[i - start] = sector[i];
    }
    return res;
}

void printInfomation(Information info)
{
    if (info.base10_value == -1)
    {
        for (size_t i = 0; i < info.length; i++)
        {
            cout << info.sector[i];
        }
    }
    else
    {
        cout << info.base10_value;
    }
    cout << endl;
}

Information read_oem_id(BYTE *sector)
{
    Information oem_id = extract(sector, stoi("0x03", nullptr, 0), vLONGLONG);

    return oem_id;
}

Information read_bytes_per_sector(BYTE *sector)
{
    Information bps = extract(sector, stoi("0x0B", nullptr, 0), vWORD);
    convertInformationToInt(bps);

    return bps;
}

Information read_sectors_per_cluster(BYTE *sector)
{
    Information spc = extract(sector, stoi("0x0D", nullptr, 0), vBYTE);
    convertInformationToInt(spc);

    return spc;
}

Information read_sector_per_track(BYTE *sector)
{
    Information spt = extract(sector, stoi("0x18", nullptr, 0), vWORD);
    convertInformationToInt(spt);

    return spt;
}

Information read_logical_cluster_number_mft(BYTE *sector)
{
    Information lcn_mft = extract(sector, stoi("0x30", nullptr, 0), vLONGLONG);
    convertInformationToInt(lcn_mft);

    return lcn_mft;
}

Information read_cluster_per_file_record(BYTE *sector)
{
    Information cpfr = extract(sector, stoi("0x40", nullptr, 0), vDWORD);
    convertInformationToInt(cpfr);

    return cpfr;
}

Information read_total_sector(BYTE *sector)
{
    Information tsec = extract(sector, stoi("0x28", nullptr, 0), vLONGLONG);
    convertInformationToInt(tsec);

    return tsec;
}

Information read_logical_cluster_number_mftmirr(BYTE *sector)
{
    Information lcn_mftmirr = extract(sector, stoi("0x38", nullptr, 0), vLONGLONG);
    convertInformationToInt(lcn_mftmirr);

    return lcn_mftmirr;
}

NTFS_Partition_Boot_Sector read(BYTE *sector)
{
    NTFS_Partition_Boot_Sector ntfs;
    ntfs.oem_id = read_oem_id(sector);
    ntfs.bytes_per_sector = read_bytes_per_sector(sector);
    ntfs.sectors_per_cluster = read_sectors_per_cluster(sector);
    ntfs.sectors_per_track = read_sector_per_track(sector);
    ntfs.logical_cluster_number_mft = read_logical_cluster_number_mft(sector);
    ntfs.cluster_per_file_record = read_cluster_per_file_record(sector);
    ntfs.total_sector = read_total_sector(sector);
    ntfs.logical_cluster_number_mftmirr = read_logical_cluster_number_mftmirr(sector);
    return ntfs;
}

void print(NTFS_Partition_Boot_Sector ntfs)
{
    cout << "OEM ID: ";
    printInfomation(ntfs.oem_id);
    cout << "Bytes per sector: ";
    printInfomation(ntfs.bytes_per_sector);
    cout << "Sectors per cluster: ";
    printInfomation(ntfs.sectors_per_cluster);
    cout << "Sectors per track: ";
    printInfomation(ntfs.sectors_per_track);
    cout << "Total sectors: ";
    printInfomation(ntfs.total_sector);
    cout << "Logical cluster number for MFT: ";
    printInfomation(ntfs.logical_cluster_number_mft);
    cout << "Logical cluster number for MFTMirr: ";
    printInfomation(ntfs.logical_cluster_number_mftmirr);
    cout << "Cluster per file record: ";
    printInfomation(ntfs.cluster_per_file_record);
}

/////////////////////////////////////////////////////////////////////
LONG get_cluster_per_file_record(NTFS_Partition_Boot_Sector ntfs)
{
    return ntfs.cluster_per_file_record.base10_value;
}

int64_t convertClusterToBytes(LONG cluster_starter, NTFS_Partition_Boot_Sector ntfs)
{
    return (int64_t)cluster_starter * ntfs.sectors_per_cluster.base10_value * ntfs.bytes_per_sector.base10_value;
}

//////////////////////////////////////////////////////////////////

void printSector(BYTE *sector, size_t start, size_t length)
{
    for (size_t i = start; i < start + length; i++)
    {
        cout << convertBase10ToBase16(sector[i]) << " ";
    }
}
//----------//
string convertBase10ToBase2(int64_t num)
{
    string res{};
    while (num > 0)
    {
        res.insert(res.begin(), num % 2 == 0 ? '0' : '1');
        num /= 2;
    }
    return res;
}

LONG convertBase2ToBase10(string s, bool part)
{ // part is low -> 0 or high -> 1
    LONG result = 0;
    string substr;
    if (part == 0)
    {
        substr = s.substr(s.length() - 32, 32);
    }
    else if (part == 1)
    {
        substr = s.substr(0, 32);
    }
    result = stoi(substr, nullptr, 2);

    return result;
}
//-------------------//

//#########################//
// FILE READER for one record
Information read_signature(BYTE *sector)
{
    Information sig = extract(sector, stoi("0x00", nullptr, 0), vDWORD);

    return sig;
}

Information read_offset_first_attribute(BYTE *sector)
{
    Information ofa = extract(sector, stoi("0x14", nullptr, 0), vWORD);
    convertInformationToInt(ofa);

    return ofa;
}

MFT_Entry readMFT(BYTE *sector)
{
    MFT_Entry entry;
    entry.signature = read_signature(sector);
    entry.offset_first_attribute = read_offset_first_attribute(sector);

    return entry;
}

void getAttribute(MFT_Entry &entry, BYTE *sector)
{
    LONG first_attribute = entry.offset_first_attribute.base10_value;
    // Extract: Type of attribute
    Information type = extract(sector, first_attribute, vDWORD);
    convertInformationToInt(type);
    if (type.base10_value == 48)
    { // File Name
    }
    else if (type.base10_value == 128)
    { // Data
    }
}

void printMFT(MFT_Entry entry)
{
    cout << "Signature: ";
    printInfomation(entry.signature);
    cout << "Offset First Attribute: ";
    printInfomation(entry.offset_first_attribute);
}
//##########################//

int main(int argc, char **argv)
{
    /*
    // Cau 1
    int64_t readPoint = 0;
    BYTE sector[512];
    ReadSector(L"\\\\.\\E:", 0, sector);
    NTFS_Partition_Boot_Sector ntfs = read(sector);


    // Cau 2
    BYTE mft_sector[1024];
    LONG start_mft = ntfs.logical_cluster_number_mft.base10_value;
    readPoint = convertClusterToBytes(start_mft, ntfs);
    //cout << "Start MFT: " << start_mft << "\nReadPoint: " << readPoint;
    ReadSector(L"\\\\.\\E:", readPoint, mft_sector);

    MFT_Entry entry = readMFT(mft_sector);
    printMFT(entry);



    int k = 0;
    for (short i : mft_sector) {
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