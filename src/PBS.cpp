#include "PBS.h"
#include "NTFS.h"

bool readSector(LPCWSTR drive, int64_t readPoint, BYTE sector[512])
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
        printf("Error (CreateFile): %u\n", GetLastError());
        return 0;
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
        printf("Error (ReadFile): %u\n", GetLastError());
        return 0;
    }
    else
    {
        printf("Success!\n");
        return 1;
    }
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

void convertInformationToInt(Information &info)
{
    string s = "0x";

    for (int i = info.length - 1; i >= 0; i--)
    {
        s.append(convertBase10ToBase16(info.sector[i]));
    }

    int value = stoi(s, nullptr, 0);
    info.base10_value = value;
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

void print(NTFS_Partition_Boot_Sector ntfs)
{
    cout << "-----------------------------------------------------------------------------" << endl;

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

    cout << "-----------------------------------------------------------------------------" << endl;
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

NTFS_Partition_Boot_Sector getNTFSPartitionBootSector()
{
    int64_t readPoint = 0;
    BYTE sector[512];
    NTFS_Partition_Boot_Sector NTFS_PBS;

    if (readSector(CURRENT_DRIVE, readPoint, sector))
    {
        NTFS_PBS = read(sector);
        
        START_CLUSTER = NTFS_PBS.logical_cluster_number_mft.base10_value;
        SECTOR_PER_CLUSTER = NTFS_PBS.sectors_per_cluster.base10_value;
        SECTOR_SIZE = NTFS_PBS.bytes_per_sector.base10_value;
    }

    return NTFS_PBS;
}