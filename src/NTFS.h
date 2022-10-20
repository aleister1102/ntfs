// Ổ đĩa cần đọc
extern const wchar_t *CURRENT_DRIVE;

// Các thông số của phân vùng
extern long long START_CLUSTER;
extern long SECTOR_PER_CLUSTER;
extern long SECTOR_SIZE;

// Các thông số cơ bản của MFT
extern int ROOT_DIR;
extern int $MFT_INDEX;
extern int MFT_LIMIT;

// Signature của các attribute
extern unsigned int FILE_NAME;
extern unsigned int DATA;
extern unsigned int END_MARKER;

// Tên tập tin lưu entry
extern const char *ENTRY_FILENAME;
