#ifndef OPENFILESTABLE_H
#define OPENFILESTABLE_H

#include "openfile.h"

#define MAX_OPEN_FILES 10

class OpenFilesTable {
public:
    OpenFilesTable();
    ~OpenFilesTable();

    int Add(OpenFile *f);      // returns fd [0..9] or -1
    OpenFile* Get(int fd);     // NULL if invalid / closed
    bool Remove(int fd);       // closes slot (does NOT delete f unless you choose)
    void CloseAll();           // useful at process exit

private:
    OpenFile* table[MAX_OPEN_FILES];
};

#endif
