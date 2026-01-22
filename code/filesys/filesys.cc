// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "system.h"
#include "systemTable.h"
#include "synch.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        char zero[SectorSize];
        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

    // First, allocate space for FileHeaders for the directory and bitmap
    // (make sure no one else grabs these!)
	freeMap->Mark(FreeMapSector);	    
	freeMap->Mark(DirectorySector);

    // Second, allocate space for the data blocks containing the contents
    // of the directory and bitmap files.  There better be enough space!

	ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
	ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

    // Flush the bitmap and directory FileHeaders back to disk
    // We need to do this before we can "Open" the file, since open
    // reads the file header off of disk (and currently the disk has garbage
    // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
	mapHdr->WriteBack(FreeMapSector);    
	dirHdr->WriteBack(DirectorySector);

    // OK to open the bitmap and directory files now
    // The file system operations assume these two files are left open
    // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        currentDirectoryFile = new OpenFile(DirectorySector); // set current directory to root
        currentDirectorySector = DirectorySector;
    // Once we have the files "open", we can write the initial version
    // of each file back to disk.  The directory at this point is completely
    // empty; but the bitmap has been changed to reflect the fact that
    // sectors on the disk have been allocated for the file headers and
    // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
	freeMap->WriteBack(freeMapFile);	 // flush changes to disk
	directory->WriteBack(directoryFile);

        for (int i = 0; i < SectorSize; i++) {
            zero[i] = '\0';
        }
        for (int s = 0; s < NumSectors; s++) {
            if (!freeMap->Test(s)) {
                synchDisk->WriteSector(s, zero);
            }
        }

	if (DebugIsEnabled('f')) {
	    freeMap->Print();
	    directory->Print();

        delete freeMap; 
	delete directory; 
	delete mapHdr; 
	delete dirHdr;
	}
    } else {
    // if we are not formatting the disk, just open the files representing
    // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        currentDirectoryFile = new OpenFile(DirectorySector); // set current directory to root
        currentDirectorySector = DirectorySector;
    }
    dataLock = new Lock("FS dataLock");

}
FileSystem::~FileSystem() {
    delete dataLock;
}

bool
FileSystem::ResolvePath(const char *path, int *sector, int *isDir)
{
    if (path == NULL || path[0] == '\0') {
        return FALSE;
    }

    int secteurCourant = (path[0] == '/') ? DirectorySector : currentDirectorySector;
    OpenFile *dossierCourant = new OpenFile(secteurCourant);
    Directory *dossier = new Directory(NumDirEntries);

    int i = 0;
    bool aComposant = FALSE;

    while (1) {
        while (path[i] == '/') i++;
        if (path[i] == '\0') {
            if (!aComposant) {
                *sector = DirectorySector;
                *isDir = 1;
            } else {
                *sector = secteurCourant;
                *isDir = 1;
            }
            delete dossier;
            delete dossierCourant;
            return TRUE;
        }

        char nom[FileNameMaxLen + 1];
        int len = 0;
        while (path[i] != '\0' && path[i] != '/') {
            if (len >= FileNameMaxLen) {
                delete dossier;
                delete dossierCourant;
                return FALSE;
            }
            nom[len++] = path[i++];
        }
        nom[len] = '\0';
        aComposant = TRUE;

        while (path[i] == '/') i++;
        bool dernier = (path[i] == '\0');

        if (!strcmp(nom, ".")) {
            if (dernier) {
                *sector = secteurCourant;
                *isDir = 1;
                delete dossier;
                delete dossierCourant;
                return TRUE;
            }
            continue;
        }

        if (!strcmp(nom, "..")) {
            if (secteurCourant != DirectorySector) {
                int secteurParent = -1;
                int parentEstDir = 0;
                dossier->FetchFrom(dossierCourant);
                if (!dossier->Find("..", &secteurParent, &parentEstDir) || !parentEstDir) {
                    delete dossier;
                    delete dossierCourant;
                    return FALSE;
                }
                delete dossierCourant;
                dossierCourant = new OpenFile(secteurParent);
                secteurCourant = secteurParent;
            }
            if (dernier) {
                *sector = secteurCourant;
                *isDir = 1;
                delete dossier;
                delete dossierCourant;
                return TRUE;
            }
            continue;
        }

        dossier->FetchFrom(dossierCourant);
        int secteurEnfant = -1;
        int enfantEstDir = 0;
        if (!dossier->Find(nom, &secteurEnfant, &enfantEstDir)) {
            delete dossier;
            delete dossierCourant;
            return FALSE;
        }

        if (dernier) {
            *sector = secteurEnfant;
            *isDir = enfantEstDir;
            delete dossier;
            delete dossierCourant;
            return TRUE;
        }

        if (!enfantEstDir) {
            delete dossier;
            delete dossierCourant;
            return FALSE;
        }

        delete dossierCourant;
        dossierCourant = new OpenFile(secteurEnfant);
        secteurCourant = secteurEnfant;
    }
}

bool
FileSystem::ResolveParent(const char *path, OpenFile **parentFile, int *parentSector,
                          char *nomFinal)
{
    if (path == NULL || path[0] == '\0') {
        return FALSE;
    }

    int secteurCourant = (path[0] == '/') ? DirectorySector : currentDirectorySector;
    OpenFile *dossierCourant = new OpenFile(secteurCourant);
    Directory *dossier = new Directory(NumDirEntries);

    int i = 0;

    while (1) {
        while (path[i] == '/') i++;
        if (path[i] == '\0') {
            delete dossier;
            delete dossierCourant;
            return FALSE;
        }

        char nom[FileNameMaxLen + 1];
        int len = 0;
        while (path[i] != '\0' && path[i] != '/') {
            if (len >= FileNameMaxLen) {
                delete dossier;
                delete dossierCourant;
                return FALSE;
            }
            nom[len++] = path[i++];
        }
        nom[len] = '\0';

        while (path[i] == '/') i++;
        bool dernier = (path[i] == '\0');

        if (dernier) {
            if (!strcmp(nom, ".") || !strcmp(nom, "..")) {
                delete dossier;
                delete dossierCourant;
                return FALSE;
            }
            strncpy(nomFinal, nom, FileNameMaxLen);
            nomFinal[FileNameMaxLen] = '\0';
            *parentFile = dossierCourant;
            *parentSector = secteurCourant;
            delete dossier;
            return TRUE;
        }

        if (!strcmp(nom, ".")) {
            continue;
        }

        if (!strcmp(nom, "..")) {
            if (secteurCourant != DirectorySector) {
                int secteurParent = -1;
                int parentEstDir = 0;
                dossier->FetchFrom(dossierCourant);
                if (!dossier->Find("..", &secteurParent, &parentEstDir) || !parentEstDir) {
                    delete dossier;
                    delete dossierCourant;
                    return FALSE;
                }
                delete dossierCourant;
                dossierCourant = new OpenFile(secteurParent);
                secteurCourant = secteurParent;
            }
            continue;
        }

        dossier->FetchFrom(dossierCourant);
        int secteurEnfant = -1;
        int enfantEstDir = 0;
        if (!dossier->Find(nom, &secteurEnfant, &enfantEstDir) || !enfantEstDir) {
            delete dossier;
            delete dossierCourant;
            return FALSE;
        }

        delete dossierCourant;
        dossierCourant = new OpenFile(secteurEnfant);
        secteurCourant = secteurEnfant;
    }
}


//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(const char *name, int initialSize)
{
    Directory *dossier;
    BitMap *bitmap;
    FileHeader *entete;
    int secteur;
    bool success;
    OpenFile *dossierParent = NULL;
    int secteurParent = -1;
    char nomFinal[FileNameMaxLen + 1];

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);
    dataLock->Acquire();
    if (!ResolveParent(name, &dossierParent, &secteurParent, nomFinal)) {
        dataLock->Release();
        return FALSE;
    }

    dossier = new Directory(NumDirEntries);
    dossier->FetchFrom(dossierParent);

    if (dossier->Find(nomFinal) != -1)
      success = FALSE;			// file is already in directory
    else {	
        bitmap = new BitMap(NumSectors);
        bitmap->FetchFrom(freeMapFile);
        secteur = bitmap->Find();	// find a sector to hold the file header
    	if (secteur == -1) 		
            success = FALSE;		// no free block for file header 
        else if (!dossier->Add(nomFinal, secteur, 0))
            success = FALSE;	// no space in directory
        else {
                entete = new FileHeader;
            if (!entete->Allocate(bitmap, initialSize))
                    success = FALSE;	// no space on disk for data
            else {
                success = TRUE;
            // everthing worked, flush all changes back to disk
                    entete->WriteBack(secteur); 		
                    dossier->WriteBack(dossierParent);
                    bitmap->WriteBack(freeMapFile);
            }
                delete entete;
        }
        delete bitmap;
    }
    delete dossier;
    delete dossierParent;
    dataLock->Release();
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(const char *name)
{
    OpenFile *openFile = NULL;
    int secteurCible;
    int estDossier = 0;
    dataLock->Acquire();

    DEBUG('f', "Opening file %s\n", name);
    if (ResolvePath(name, &secteurCible, &estDossier) && !estDossier) {
	openFile = new OpenFile(secteurCible);
    }
    dataLock->Release();

    return openFile;				// return NULL if not found
}

//---------------------------------------------------------------------- 
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(const char *name)
{ 
    Directory *dossier;
    BitMap *bitmap;
    FileHeader *entete;
    int secteurCible = -1;
    int estDossier = 0;
    OpenFile *dossierParent = NULL;
    int secteurParent = -1;
    char nomFinal[FileNameMaxLen + 1];

    dossier = new Directory(NumDirEntries);
    dataLock->Acquire();
    if (!ResolveParent(name, &dossierParent, &secteurParent, nomFinal)) {
        delete dossier;
        dataLock->Release();
        return FALSE; // not found
    }
    dossier->FetchFrom(dossierParent);

    if (!dossier->Find(nomFinal, &secteurCible, &estDossier)) {
        delete dossier;
        delete dossierParent;
        dataLock->Release();
        return FALSE; // not found
    }
    if (secteurCible == -1) {
       delete dossier;
       delete dossierParent;
       dataLock->Release();
       return FALSE;			 // file not found 
    }
    if (sysTable->IsOpen(secteurCible)) {
        delete dossier;
        delete dossierParent;
        dataLock->Release();
        return FALSE;
    }
    //un rep ne peut etre supp que sil est vide
    if (estDossier) {
        OpenFile *fichierDir = new OpenFile(secteurCible);
        Directory *dossierEnfant = new Directory(NumDirEntries);
        dossierEnfant->FetchFrom(fichierDir);

        if (!dossierEnfant->IsEmpty()) {
            delete dossierEnfant;
            delete fichierDir;
            delete dossier;
            dataLock->Release();
            return FALSE; //directory not empty
        }

        delete dossierEnfant;
        delete fichierDir;
        //si vide, on continue et on supprime 
    }
    
    entete = new FileHeader;
    entete->FetchFrom(secteurCible);

    bitmap = new BitMap(NumSectors);
    bitmap->FetchFrom(freeMapFile);

    entete->Deallocate(bitmap);  		// remove data blocks
    bitmap->Clear(secteurCible);			// remove header block
    dossier->Remove(nomFinal);

    bitmap->WriteBack(freeMapFile);		// flush to disk
    dossier->WriteBack(dossierParent);        // flush to disk
    delete entete;
    delete dossier;
    delete bitmap;
    delete dossierParent;
    dataLock->Release();

    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
    Directory *dossier = new Directory(NumDirEntries);

    dossier->FetchFrom(currentDirectoryFile);
    dossier->List();
    delete dossier;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *enteteBitmap = new FileHeader;
    FileHeader *enteteDir = new FileHeader;
    BitMap *bitmap = new BitMap(NumSectors);
    Directory *dossier = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    enteteBitmap->FetchFrom(FreeMapSector);
    enteteBitmap->Print();

    printf("Directory file header:\n");
    enteteDir->FetchFrom(DirectorySector);
    enteteDir->Print();

    bitmap->FetchFrom(freeMapFile);
    bitmap->Print();

    dossier->FetchFrom(currentDirectoryFile);
    dossier->Print();

    delete enteteBitmap;
    delete enteteDir;
    delete bitmap;
    delete dossier;
} 

//function to create a subdirectory of the current directory
bool 
FileSystem::MakeDirectory(char *name)
{
    Directory *dossierParent = new Directory(NumDirEntries); 
    Directory *nouveauDir = new Directory(NumDirEntries);
    FileHeader *entete = new FileHeader;
    BitMap *bitmap = new BitMap(NumSectors);
    int secteur;
    bool success = FALSE;
    OpenFile *fichierParent = NULL;
    int secteurParent = -1;
    char nomFinal[FileNameMaxLen + 1];

    printf("Creating directory: %s\n", name);
    dataLock->Acquire();

    if (!ResolveParent(name, &fichierParent, &secteurParent, nomFinal)) {
        dataLock->Release();
        delete dossierParent;
        delete nouveauDir;
        delete entete;
        delete bitmap;
        return FALSE;
    }

    dossierParent->FetchFrom(fichierParent);

    //check the free map for a free sector to store the header of the new directory
    bitmap->FetchFrom(freeMapFile);
    secteur = bitmap->Find(); 

    if (secteur == -1) {
        //no more available secotrs
        success = FALSE; 
        dataLock->Release();
    } else {
        if (!entete->Allocate(bitmap, DirectoryFileSize)) {
            success = FALSE;
            dataLock->Release();
        } else {
            entete->WriteBack(secteur);
            if (!dossierParent->Add(nomFinal, secteur, 1)) {
                success = FALSE;
                dataLock->Release();
            } else {
                //add mandatory entries
                nouveauDir->Add((char *)".", secteur, 1);
                nouveauDir->Add((char *)"..", secteurParent, 1); // Point to parent
                entete->WriteBack(secteur); 
            
                //write the directory table of the new directory (with . .. for now)
                OpenFile *subdirf = new OpenFile(secteur); // Create a temporary file handle
                nouveauDir->WriteBack(subdirf);
                delete subdirf;

                dossierParent->WriteBack(fichierParent);
                bitmap->WriteBack(freeMapFile);

                success = TRUE;
            }
        }
    }

    dataLock->Release();
    // Cleanup memory
    delete dossierParent;
    delete nouveauDir;
    delete entete;
    delete bitmap;
    delete fichierParent;
    
    return success;
}

//function to change directory from the current one to a subdirectory
bool
FileSystem::ChangeDirectory(char *name){
    int secteurCible;
    bool success = FALSE;  
    dataLock->Acquire();

    int estDossier = 0;
    if (ResolvePath(name, &secteurCible, &estDossier) && estDossier) {
        delete currentDirectoryFile;//fermeture de l'ancien rep.
        currentDirectoryFile = new OpenFile(secteurCible);//nouveau  rep
        currentDirectorySector = secteurCible;
        success=TRUE;
    }
    dataLock->Release();
    return success;
}

//function to get te sector of a file in the current directory
int FileSystem::FindSector(const char *name)
{
    dataLock->Acquire();
    int secteurCible = -1;
    int estDossier = 0;
    if (ResolvePath(name, &secteurCible, &estDossier) && estDossier) {
        secteurCible = -1;
    }
    dataLock->Release();
    return secteurCible; 
}

bool
FileSystem::ExtendFile(int hdrSector, int newSize)
{
    if (hdrSector < 0 || newSize < 0) return FALSE;

    dataLock->Acquire();

    BitMap *freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    FileHeader *hdr = new FileHeader;
    hdr->FetchFrom(hdrSector);

    bool ok = TRUE;
    ok = hdr->Extend(freeMap, newSize);   // à faire dans filehdr.(h/.cc)
    if (ok) {
        hdr->WriteBack(hdrSector);
        freeMap->WriteBack(freeMapFile);
    }

    delete hdr;
    delete freeMap;

    dataLock->Release();
    return ok;
}
