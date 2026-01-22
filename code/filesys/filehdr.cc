// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, we use a single level of indirection).
//	The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    if (fileSize < 0 || fileSize > (int)MaxFileSize)
        return FALSE;
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    int numIndirectBlocks = divRoundUp(numSectors, NumIndirect);

    if (numIndirectBlocks > (int)NumDirect)
        return FALSE;

    if (freeMap->NumClear() < (numSectors + numIndirectBlocks))
	return FALSE;

    int sectorsAllocated = 0;
    for (int i = 0; i < numIndirectBlocks; i++) {
        dataSectors[i] = freeMap->Find();

        int indirect[NumIndirect];
        for (unsigned int j = 0; j < NumIndirect; j++)
            indirect[j] = -1;

        for (unsigned int j = 0; j < NumIndirect && sectorsAllocated < numSectors; j++) {
            indirect[j] = freeMap->Find();
            sectorsAllocated++;
        }
        synchDisk->WriteSector(dataSectors[i], (char *)indirect);
    }

    for (unsigned int i = numIndirectBlocks; i < NumDirect; i++)
        dataSectors[i] = -1;
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    int numIndirectBlocks = divRoundUp(numSectors, NumIndirect);
    int sectorsFreed = 0;

    for (int i = 0; i < numIndirectBlocks; i++) {
        int indirSector = dataSectors[i];
        ASSERT(indirSector >= 0);
        ASSERT(freeMap->Test(indirSector));

        int indirect[NumIndirect];
        synchDisk->ReadSector(indirSector, (char *)indirect);

        for (unsigned int j = 0; j < NumIndirect && sectorsFreed < numSectors; j++) {
            int data = indirect[j];
            ASSERT(data >= 0);
            ASSERT(freeMap->Test(data));
            freeMap->Clear(data);
            sectorsFreed++;
        }

        freeMap->Clear(indirSector);
    }

    ASSERT(sectorsFreed == numSectors);
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int sectorIndex = offset / SectorSize;
    int indirIndex  = sectorIndex / NumIndirect;
    int indirOffset = sectorIndex % NumIndirect;

    ASSERT(indirIndex >= 0 && indirIndex < (int)NumDirect);
    ASSERT(dataSectors[indirIndex] >= 0);

    int indirect[NumIndirect];
    synchDisk->ReadSector(dataSectors[indirIndex], (char *)indirect);

    return indirect[indirOffset];
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", ByteToSector(i * SectorSize));
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	synchDisk->ReadSector(ByteToSector(i * SectorSize), data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

bool
FileHeader::Extend(BitMap *freeMap, int newFileSize)
{
    if (newFileSize < 0 || newFileSize > (int)MaxFileSize)
        return FALSE;

    int oldNumSectors = numSectors;
    int newNumSectors = divRoundUp(newFileSize, SectorSize);

    int oldIndirectBlocks = divRoundUp(oldNumSectors, NumIndirect);
    int newIndirectBlocks = divRoundUp(newNumSectors, NumIndirect);

    if (newIndirectBlocks > (int)NumDirect)
        return FALSE;

    int extraData  = newNumSectors - oldNumSectors;
    int extraIndir = newIndirectBlocks - oldIndirectBlocks;

    if (freeMap->NumClear() < (extraData + extraIndir))
        return FALSE;

    char zeros[SectorSize];
    bzero(zeros, SectorSize);

    for (int i = oldIndirectBlocks; i < newIndirectBlocks; i++) {
        int indirSector = freeMap->Find();
        dataSectors[i] = indirSector;

        int indir[NumIndirect];
        for (unsigned int j = 0; j < NumIndirect; j++)
            indir[j] = -1;

        synchDisk->WriteSector(indirSector, (char *)indir);
    }

    int sectorIndex = oldNumSectors;

    while (sectorIndex < newNumSectors) {
        int indirIdx    = sectorIndex / NumIndirect;
        int indirOffset = sectorIndex % NumIndirect;

        ASSERT(indirIdx >= 0 && indirIdx < (int)NumDirect);
        ASSERT(dataSectors[indirIdx] >= 0);

        int indir[NumIndirect];
        synchDisk->ReadSector(dataSectors[indirIdx], (char *)indir);

        // Fill as many entries as possible in this indirect block
        for (unsigned int j = indirOffset; j < NumIndirect && sectorIndex < newNumSectors; j++) {
            int dataSector = freeMap->Find();
            indir[j] = dataSector;

            // Initialize new data block to zero
            synchDisk->WriteSector(dataSector, zeros);

            sectorIndex++;
        }

        // Persist this indirect block update
        synchDisk->WriteSector(dataSectors[indirIdx], (char *)indir);
    }

    numBytes   = newFileSize;
    numSectors = newNumSectors;
    return TRUE;
}
