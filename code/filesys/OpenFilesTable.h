#ifndef OPENFILESTABLE_H
#define OPENFILESTABLE_H

#include "openfile.h"

#define MAX_OPEN_FILES 10

class Lock;

class OpenFilesTable {
public:
    OpenFilesTable();
    ~OpenFilesTable();

    int Add(OpenFile *f, int hdrSector);      // returns fd [0..9] or -1
    OpenFile* Get(int fd);     // NULL if invalid / closed
    int Close(int fd);                  // returns hdrSector, or -1 if invalid
    void CloseAll();           // useful at process exit
    int GetHdrSector(int fd); 
    void Print();              // debug: print table content
    bool IsUsed(int fd);
private:
    struct FdEntry {
        bool used;
        OpenFile* of;
        int hdrSector;   // identifiant du fichier (pour systemTable)
        };
    FdEntry table[MAX_OPEN_FILES];
    Lock *fdtableLock;


};

#endif
