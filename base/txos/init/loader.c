/*++

Free95 20x/TX Kernel

You may only use this code if you agree to the terms of the Free95 Source Code License agreement (GNU GPL v3) (see LICENSE).
If you do not agree to the terms, do not use the code.


Module Name:

    loader.c

Abstract:

    This module implements the code to load PE Executable files and Batch scripts.

--*/

#include "loader.h"

typedef struct _IMAGE_DOS_HEADER {  // DOS .EXE header
    uint16_t e_magic;		// must contain "MZ"
    uint16_t e_cblp;		// number of bytes on the last page of the file
    uint16_t e_cp;		// number of pages in file
    uint16_t e_crlc;		// relocations
    uint16_t e_cparhdr;		// size of the header in paragraphs
    uint16_t e_minalloc;	// minimum and maximum paragraphs to allocate
    uint16_t e_maxalloc;
    uint16_t e_ss;		// initial SS:SP to set by Loader
    uint16_t e_sp;
    uint16_t e_csum;		// checksum
    uint16_t e_ip;		// initial CS:IP
    uint16_t e_cs;
    uint16_t e_lfarlc;		// address of relocation table
    uint16_t e_ovno;		// overlay number
    uint16_t e_res[4];		// resevered
    uint16_t e_oemid;		// OEM id
    uint16_t e_oeminfo;		// OEM info
    uint16_t e_res2[10];	// reserved
    uint32_t   e_lfanew;	// address of new EXE header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    USHORT  Machine;
    USHORT  NumberOfSections;			// Number of sections in section table
    ULONG   TimeDateStamp;			// Date and time of program link
    ULONG   PointerToSymbolTable;		// RVA of symbol table
    ULONG   NumberOfSymbols;			// Number of symbols in table
    USHORT  SizeOfOptionalHeader;		// Size of IMAGE_OPTIONAL_HEADER in bytes
    USHORT  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD VirtualAddress;		// RVA of table
  DWORD Size;			// size of table
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_EXPORT_DIRECTORY {
  uint32_t Characteristics;
  uint32_t TimeDateStamp;
  uint16_t MajorVersion;
  uint16_t MinorVersion;
  uint32_t Name;
  uint32_t Base;
  uint32_t NumberOfFunctions;
  uint32_t NumberOfNames;
  uint32_t AddressOfFunctions;
  uint32_t AddressOfNames;
  uint16_t AddressOfNameOrdinal;
}IMAGE_EXPORT_DIRECTORY,*PIMAGE_EXPORT_DIRECTORY;

struct _IMAGE_OPTIONAL_HEADER {

    USHORT  Magic;				// not-so-magical number
    UCHAR   MajorLinkerVersion;			// linker version
    UCHAR   MinorLinkerVersion;
    ULONG   SizeOfCode;				// size of .text in bytes
    ULONG   SizeOfInitializedData;		// size of .bss (and others) in bytes
    ULONG   SizeOfUninitializedData;		// size of .data,.sdata etc in bytes
    ULONG   AddressOfEntryPoint;		// RVA of entry point
    ULONG   BaseOfCode;				// base of .text
    ULONG   BaseOfData;				// base of .data
    ULONG   ImageBase;				// image base VA
    ULONG   SectionAlignment;			// file section alignment
    ULONG   FileAlignment;			// file alignment
    USHORT  MajorOperatingSystemVersion;	// Windows specific. OS version required to run image
    USHORT  MinorOperatingSystemVersion;
    USHORT  MajorImageVersion;			// version of program
    USHORT  MinorImageVersion;
    USHORT  MajorSubsystemVersion;		// Windows specific. Version of SubSystem
    USHORT  MinorSubsystemVersion;
    ULONG   Reserved1;
    ULONG   SizeOfImage;			// size of image in bytes
    ULONG   SizeOfHeaders;			// size of headers (and stub program) in bytes
    ULONG   CheckSum;				// checksum
    USHORT  Subsystem;				// Windows specific. subsystem type
    USHORT  DllCharacteristics;			// DLL properties
    ULONG   SizeOfStackReserve;			// size of stack, in bytes
    ULONG   SizeOfStackCommit;			// size of stack to commit
    ULONG   SizeOfHeapReserve;			// size of heap, in bytes
    ULONG   SizeOfHeapCommit;			// size of heap to commit
    ULONG   LoaderFlags;			// no longer used
    ULONG   NumberOfRvaAndSizes;		// number of DataDirectory entries
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
  DWORD                 Signature;
  IMAGE_FILE_HEADER     FileHeader;
  struct _IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

typedef struct _IMAGE_SECTION_HEADER {
    BYTE  Name[8];                 // Name of the section (null-terminated or space-padded)
    union {
        DWORD PhysicalAddress;     // Reserved, used in some tools
        DWORD VirtualSize;         // Actual size of the section in memory
    } Misc;
    DWORD VirtualAddress;          // Address of the section in memory relative to the image base
    DWORD SizeOfRawData;           // Size of the section in the file (aligned to FileAlignment)
    DWORD PointerToRawData;        // Offset to the section data in the file
    DWORD PointerToRelocations;    // Offset to the relocations (not used in PE for Windows)
    DWORD PointerToLinenumbers;    // Offset to line numbers (deprecated)
    WORD  NumberOfRelocations;     // Number of relocation entries (not used in PE for Windows)
    WORD  NumberOfLinenumbers;     // Number of line number entries (deprecated)
    DWORD Characteristics;         // Attributes of the section (e.g., executable, writable)
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

typedef struct _PEImageFileProcessed
{
    IMAGE_FILE_HEADER FileHeader;
    struct _IMAGE_OPTIONAL_HEADER OptionalHeader;

    WINBOOL IsDll;
    ULONG ImageBase; // absolute
    DWORD SizeOfImage;
    DWORD AddressOfEntryPointOffset; // relative

    WORD NumOfSections;
    PIMAGE_SECTION_HEADER SectionHeaderFirst; // absolute

    PIMAGE_DATA_DIRECTORY pDataDirectoryExport;
    PIMAGE_DATA_DIRECTORY pDataDirectoryImport;
    PIMAGE_DATA_DIRECTORY pDataDirectoryReloc;
    PIMAGE_DATA_DIRECTORY pDataDirectoryException;
} PEImageFileProcessed, * PPEImageFileProcessed;

#define FIELD_OFFSET(type, field) ((LONG)(ULONG)&(((type *)0)->field))

#define IMAGE_FIRST_SECTION(ntHeaders) \
    ((PIMAGE_SECTION_HEADER)((ULONG)(ntHeaders) + \
    FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + \
    ((ntHeaders)->FileHeader.SizeOfOptionalHeader)))

WINBOOL LdrProcessPe(LPVOID pBufImageFile, PPEImageFileProcessed pPeImageFileProcessed)
{
    DbgPrint("LdrProcessPe() called with params:\npBufImageFile=0x%x\npPeImageFileProcessed=%d\n", pBufImageFile, pPeImageFileProcessed);

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pBufImageFile;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((ULONG)pBufImageFile + (pDosHeader->e_lfanew));

    if (!(pNtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        DbgLog("LdrProcessPe(): pBufImageFile is not a valid PE File", LOG_ERROR);
        return FALSE;
    }

    pPeImageFileProcessed->FileHeader = pNtHeaders->FileHeader;
    pPeImageFileProcessed->OptionalHeader = pNtHeaders->OptionalHeader;

    // Process misc
    pPeImageFileProcessed->IsDll = (pNtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) ? TRUE : FALSE;
    pPeImageFileProcessed->SizeOfImage = pNtHeaders->OptionalHeader.SizeOfImage;
    pPeImageFileProcessed->ImageBase = pNtHeaders->OptionalHeader.ImageBase;
    pPeImageFileProcessed->AddressOfEntryPointOffset = pNtHeaders->OptionalHeader.AddressOfEntryPoint;

    // Process section headers
    pPeImageFileProcessed->NumOfSections = pNtHeaders->FileHeader.NumberOfSections;
    pPeImageFileProcessed->SectionHeaderFirst = IMAGE_FIRST_SECTION(pNtHeaders);

    // Process required sections explicitly
    pPeImageFileProcessed->pDataDirectoryExport = &(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    pPeImageFileProcessed->pDataDirectoryImport = &(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    pPeImageFileProcessed->pDataDirectoryReloc = &(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]);
    pPeImageFileProcessed->pDataDirectoryException = &(pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION]);

    return TRUE;
}

void LdrAllocMemory(DWORD SizeOfImage, LPVOID *pBufInMemPE)
{
    DbgPrint("LdrAllocMemory() called with params:\nSizeOfImage=%d\npBufInMemPE=%d\n", SizeOfImage, pBufInMemPE);

    *pBufInMemPE = kmalloc(SizeOfImage);

    if (*pBufInMemPE == NULL)
    {
        DbgLog("LdrAllocMemory(): Failed to allocate memory", LOG_ERROR);
    }
    else
    {
        DbgLog("LdrAllocMemory(): Allocated memory successfully", LOG_SUCCESS);
    }
}

void LdrCpySections(PPEImageFileProcessed pPeImageFileProcessed, LPVOID pBufImageFile, LPVOID pBufInMemPE)
{
    DbgPrint("LdrCpySections() called with params:\npPeImageFileProcessed=0x%x\npBufImageFile=0x%x\n", pPeImageFileProcessed, pBufImageFile);

    for (int i = 0; i < pPeImageFileProcessed->NumOfSections; i++)
    {
        IMAGE_SECTION_HEADER SectionHeader = pPeImageFileProcessed->SectionHeaderFirst[i];

        memcpy((void*)(DWORD64)pBufInMemPE + SectionHeader.VirtualAddress, (void*)(DWORD64)pBufImageFile + SectionHeader.PointerToRawData, SectionHeader.SizeOfRawData);
    }
}

typedef struct _IMAGE_BASE_RELOCATION_ENTRY
{
    WORD Offset : 12;
    WORD Type: 4;
} IMAGE_BASE_RELOCATION_ENTRY, *PIMAGE_BASE_RELOCATION_ENTRY;

typedef struct _IMAGE_BASE_RELOCATION {
    DWORD VirtualAddress; // RVA of the block of relocations
    DWORD SizeOfBlock;    // Size of the block, including this header
    // WORD TypeOffset[]; // An array of relocation entries
} IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

void LdrRelocMemory(PPEImageFileProcessed pPeImageFileProcessed, LPVOID pBufInMemPE)
{
    DbgPrint("LdrRelocMemory() called\n");

    PIMAGE_BASE_RELOCATION pImageBaseRelocation = (PIMAGE_BASE_RELOCATION)((DWORD64)pBufInMemPE + pPeImageFileProcessed->pDataDirectoryReloc->VirtualAddress);
    DWORD NumImageBaseRelocationEntry = 0;
    PIMAGE_BASE_RELOCATION_ENTRY pImageBaseRelocationEntry = 0;
    DWORD64 relocOffset = (DWORD64)pBufInMemPE - pPeImageFileProcessed->ImageBase;
    DWORD64 relocAt = 0;

    // For each Base Relocation Block
    while (pImageBaseRelocation->VirtualAddress != 0)
    {
        NumImageBaseRelocationEntry = (pImageBaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(IMAGE_BASE_RELOCATION_ENTRY);
        pImageBaseRelocationEntry = (PIMAGE_BASE_RELOCATION_ENTRY)((DWORD64)pImageBaseRelocation + sizeof(IMAGE_BASE_RELOCATION));
        relocAt = 0;

        // For each Base Relocation Block Entry
        for (int i = 0; i < NumImageBaseRelocationEntry; i++)
        {
            relocAt = (DWORD64)ADD_OFFSET_TO_POINTER(pBufInMemPE, pImageBaseRelocation->VirtualAddress + pImageBaseRelocationEntry[i].Offset);

            switch (pImageBaseRelocationEntry[i].Type)
            {
                case IMAGE_REL_BASED_HIGH: // The base relocation adds the high 16 bits of the difference to the 16-bit field at offset. The 16-bit field represents the high value of a 32-bit word.
                    *(PWORD)relocAt += HIWORD(relocOffset);
                    break;
                case IMAGE_REL_BASED_LOW: // The base relocation adds the low 16 bits of the difference to the 16-bit field at offset. The 16-bit field represents the low half of a 32-bit word.
                    *(PWORD)relocAt += LOWORD(relocOffset);
                    break;
                case IMAGE_REL_BASED_HIGHLOW: // The base relocation applies all 32 bits of the difference to the 32-bit field at offset.
                    *(PDWORD)relocAt += (DWORD)relocOffset;
                    break;
                case IMAGE_REL_BASED_DIR32: // The base relocation applies the difference to the 64-bit field at offset.
                    DbgPrint("Yes!\n");
                    *(PDWORD64)relocAt += relocOffset;
                    break;
                case IMAGE_REL_BASED_ABSOLUTE: // The base relocation is skipped. This type can be used to pad a block.
                default:
                    break;
            }
        }

        // Move on to next relocation block
        pImageBaseRelocation = ADD_OFFSET_TO_POINTER(pImageBaseRelocation, pImageBaseRelocation->SizeOfBlock);
    }
}

void join_paths(const char* str1, const char* str2, char* result, size_t result_size)
{
    // Calculate the lengths of the input strings
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);

    // Ensure there's enough space in the result buffer
    if (result_size < len1 + len2 + 2) { // +1 for '/' and +1 for null terminator
        print("Error: Result buffer is too small.\n");
        return;
    }

    // Handle different cases for concatenation
    if (str1[len1 - 1] == '/' && str2[0] == '/') {
        snprintf(result, result_size, "%s%s", str1, str2 + 1); // Skip the extra '/'
    } else if (str1[len1 - 1] != '/' && str2[0] != '/') {
        snprintf(result, result_size, "%s/%s", str1, str2); // Add a '/'
    } else {
        snprintf(result, result_size, "%s%s", str1, str2); // Direct concatenation
    }
}

LPVOID LdrLoadPe(const LPSTR path)
{
    DbgPrint("LdrLoadPe() called with params:\npath=%s\n", path);

    const char *ex = "0:/";
    char file_path[20];

    join_paths(ex, path, file_path, sizeof(file_path));

    DbgPrint("Absolute Path: %s\n", file_path);

    int fd = fopen(file_path, "r");

	if (fd)
	{
		char pBufImageFile[6200];
		fread(pBufImageFile, 6199, 1, fd);

        PPEImageFileProcessed processedFile = NULL;

        if(LdrProcessPe(pBufImageFile, processedFile))
        {
            DbgLog("LdrLoadPe(): Successfully processed PE File\n", LOG_SUCCESS);
        }
        else
        {
            DbgLog("LdrLoadPe(): Could not successfully process PE File\n", LOG_ERROR);
            return NULL;
        }

        if (processedFile->IsDll)
        {
            DbgLog("LdrLoadPe(): File is a Dynamic Link Library", LOG_INFO);
        }
        else
        {
            DbgLog("LdrLoadPe(): File is an Executable", LOG_INFO);
        }

        DWORD64 pBufInMemPE = 0;
        LdrAllocMemory(processedFile->SizeOfImage, (LPVOID)&pBufInMemPE);

        LdrCpySections(processedFile, pBufImageFile, (LPVOID)pBufInMemPE);

        LdrRelocMemory(processedFile, (LPVOID)pBufInMemPE);

        LPVOID pEntry = ADD_OFFSET_TO_POINTER(pBufInMemPE, processedFile->AddressOfEntryPointOffset);

        return pEntry;
    }
    else
    {
        DbgLog("LdrLoadPe(): Failed to open PE File", LOG_FAIL);
        return NULL;
    }
}

NTSTATUS LdrExecBat(const char *path)
{
    DbgPrint("LdrExecBat() called with params:\npath=%s\n", path);

    const char *ex = "0:/";
    char file_path[20];

    join_paths(ex, path, file_path, sizeof(file_path));

    DbgPrint("Absolute Path: %s\n", file_path);

    int fd = fopen(file_path, "r");

	if (fd)
	{
		char data[100];
		fread(data, 99, 1, fd);

        char *line = strtok(data, "\r\n");  // Tokenize by newlines (\r\n or \n)
        while (line != NULL) {
            // Remove any leading or trailing spaces
            while (*line == ' ') line++;
            if (*line != '\0') {  // Skip empty lines
                NsExec(line, 0);
            }
            line = strtok(NULL, "\r\n");  // Get the next line
        }

        return STATUS_SUCCESS;
    }
    else
    {
        DbgLog("LdrExecBat(): Failed to open Batch Script", LOG_FAIL);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
}
