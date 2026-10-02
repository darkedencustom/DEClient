// urarlib.h - interface of the UniquE RAR File Library reader in urarlib.cpp
// (verbatim copy of the reader inside RarLib/RarFile.cpp). The typedefs and
// structs below are the ones RarLib/RarFile.h declares for that code.
#pragma once
#include <windows.h>

typedef unsigned char    UBYTE;
typedef unsigned short   UWORD;
typedef unsigned long    UDWORD;

struct RAR20_archive_entry
{
    char*  Name;
    UWORD  NameSize;
    UDWORD PackSize;
    UDWORD UnpSize;
    UBYTE  HostOS;
    UDWORD FileCRC;
    UDWORD FileTime;
    UBYTE  UnpVer;
    UBYTE  Method;
    UDWORD FileAttr;
};

struct ArchiveList_struct
{
    struct RAR20_archive_entry  item;
    struct ArchiveList_struct*  next;
};

// Extract one file (by name, case-insensitive) into a malloc'd buffer.
bool urarlib_get(char** output, int* size, const char* filename, const char* libfilename, const char* libpassword);
// List the files of an archive; the caller frees every node and its Name.
int  urarlib_list(const char* libfilename, struct ArchiveList_struct** list);
