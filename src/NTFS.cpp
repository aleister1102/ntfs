#include "MFT.h"
#include "PBS.h"

// Ổ đĩa cần đọc
const wchar_t *CURRENT_DRIVE = L"\\\\.\\U:";

// Các thông số của phân vùng
long long START_CLUSTER = 0;
long SECTOR_PER_CLUSTER = 0;
long SECTOR_SIZE = 0;

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

int main(int argc, char **argv)
{
    // Đọc Partition Boot Sector
    auto NTFS_PBS = getNTFSPartitionBootSector();
    print(NTFS_PBS);

    // Hiển thị cây thư mục và đọc tập tin
    initMFT();
    runCommandLines();

    return 0;
}