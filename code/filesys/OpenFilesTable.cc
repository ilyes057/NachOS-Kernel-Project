#include "OpenFilesTable.h"

OpenFilesTable::OpenFilesTable() {
    for (int i = 0; i < MAX_OPEN_FILES; i++)
        table[i] = NULL;
}

OpenFilesTable::~OpenFilesTable() {
    CloseAll();
}

int OpenFilesTable::Add(OpenFile *f) {
    if (f == NULL) return -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (table[i] == NULL) {
            table[i] = f;
            return i;
        }
    }
    return -1; // table full
}

OpenFile* OpenFilesTable::Get(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return NULL;
    return table[fd];
}

bool OpenFilesTable::Remove(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return false;
    if (table[fd] == NULL) return false;
    //plus tard faire close?
    delete table[fd];

    table[fd] = NULL;
    return true;
}

void OpenFilesTable::CloseAll() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (table[i] != NULL) {
            delete table[i];
            table[i] = NULL;
        }
    }
}
