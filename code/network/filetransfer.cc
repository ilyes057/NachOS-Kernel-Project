#include "filetransfer.h"
#include "system.h"
#include <stdio.h>
#include <string.h>
FileTransfer::FileTransfer(VarPostOffice *vpo_, ReliablePostOffice *rpo_)
    : vpo(vpo_), rpo(rpo_) {
}

FileTransfer::~FileTransfer() {}

// --- PARSEUR DE COMMANDE ---
bool FileTransfer::ParseCommand(const char *cmd, char *type, char *arg1, char *arg2, int maxLen) {
    char buffer[MAX_CMD_LEN];
    strncpy(buffer, cmd, MAX_CMD_LEN); 
    buffer[MAX_CMD_LEN-1] = '\0';

    char *token = strtok(buffer, " ");
    if (!token) return false;
    strncpy(type, token, 10);

    token = strtok(NULL, " "); 
    if (token) {
        strncpy(arg1, token, maxLen);
        size_t len = strlen(arg1);
        while (len > 0 && (arg1[len-1] == '\n' || arg1[len-1] == '\r')) arg1[--len] = '\0';
    } else {
        arg1[0] = '\0';
    }

    token = strtok(NULL, ""); 
    if (token) {
        while(*token == ' ') token++;
        strncpy(arg2, token, maxLen);
        size_t len = strlen(arg2);
        while (len > 0 && (arg2[len-1] == '\n' || arg2[len-1] == '\r')) arg2[--len] = '\0';
    } else {
        arg2[0] = '\0';
    }
    return true;
}


bool FileTransfer::ReadFileContent(const char *filename, char **outData, int *outSize) {
    OpenFile *openFile = fileSystem->Open(filename);
    if (openFile == NULL) return false;

    int size = openFile->Length();
    char *data = new char[size + 1];
    
    // On lit directement depuis le disque simulé
    int totalRead = openFile->Read(data, size);
    data[totalRead] = '\0';
    
    delete openFile; 
    
    if (totalRead != size) { delete[] data; return false; }
    *outData = data;
    *outSize = size;
    return true;
}

bool FileTransfer::WriteFileContent(const char *filename, const char *data, int size) {
    if (!fileSystem->Create(filename, size)) return false;
    OpenFile *openFile = fileSystem->Open(filename);
    if (openFile == NULL) return false;

    int written = openFile->Write(data, size);
    delete openFile;
    return (written == size);
}


bool FileTransfer::GetDirectoryListing(const char *path, char **outList, int *outLen) {
    printf("[SERVER] Le client demande la liste des fichiers du DISK.\n");
    fileSystem->List(); 

    const char *msg = "--- Liste DISK (Consultez la console du Serveur) ---";
    *outLen = strlen(msg) + 1;
    *outList = new char[*outLen];
    strcpy(*outList, msg);
    return true;
}
// --- CLIENT : REQUÊTES DISTANTES ---
bool SendSimpleCmd(VarPostOffice *_vpo, ReliablePostOffice *_rpo, int s, const char *cmd) {
    if (!_vpo->SendLong(s, cmd, strlen(cmd) + 1)) return false;
    char r[64]; int f=-1;
    _rpo->ReceiveReliable(&f, r, 64);
    if (strncmp(r, "ACK", 3) == 0) return true;
    printf("Erreur Serveur: %s\n", r);
    return false;
}

bool FileTransfer::RequestDelete(int s, const char *f) { 
    char c[256]; snprintf(c,256,"DEL %s",f); return SendSimpleCmd(vpo,rpo,s,c); 
}
bool FileTransfer::RequestMkdir(int s, const char *f) { 
    char c[256]; snprintf(c,256,"MKDIR %s",f); return SendSimpleCmd(vpo,rpo,s,c); 
}
bool FileTransfer::RequestRmdir(int s, const char *f) { 
    char c[256]; snprintf(c,256,"RMDIR %s",f); return SendSimpleCmd(vpo,rpo,s,c); 
}
bool FileTransfer::RequestList(int s, const char *path) {
    char c[256]; snprintf(c,256,"LIST %s", (strlen(path)>0)?path:".");
    if (!vpo->SendLong(s, c, strlen(c)+1)) return false;

    char *l=NULL; int f=-1;
    int len = vpo->ReceiveLongAlloc(&f, &l);
    if (len > 0) {
        printf("\n--- Contenu Distant (%s) ---\n%s----------------------------\n", path, l);
        delete[] l; return true;
    }
    printf("Erreur reception listing.\n"); return false;
}

bool FileTransfer::RequestGet(int s, const char *remote, const char *local) {
    char c[256]; snprintf(c,256,"GET %s", remote);
    if (!vpo->SendLong(s, c, strlen(c)+1)) return false;

    char *d=NULL; int f=-1;
    int len = vpo->ReceiveLongAlloc(&f, &d);

    if (len < 0) return false;
    if (strncmp(d, "ERROR", 5) == 0) {
        printf("Erreur Serveur: %s\n", d);
        delete[] d; return false;
    }

    bool ok = WriteFileContent(local, d, len);
    delete[] d;
    if (ok) rpo->SendReliable(s, "ACK", 4);
    return ok;
}

bool FileTransfer::RequestPut(int s, const char *local, const char *remote) {
    char *d=NULL; int sz=0;
    if (!ReadFileContent(local, &d, &sz)) {
        printf("Erreur: Impossible de lire %s (Local)\n", local);
        return false;
    }

    char c[256]; snprintf(c,256,"PUT %s", remote);
    if (!vpo->SendLong(s, c, strlen(c)+1)) { delete[] d; return false; }

    char r[64]; int f=-1;
    rpo->ReceiveReliable(&f, r, 64);
    if (strncmp(r, "READY", 5) != 0) {
        printf("Erreur Serveur: %s\n", r);
        delete[] d; return false;
    }

    vpo->SendLong(s, d, sz);
    delete[] d;

    rpo->ReceiveReliable(&f, r, 64);
    return (strncmp(r, "ACK", 3) == 0);
}

// --- SERVEUR ---

void FileTransfer::StartServer() {
    printf("[SERVER] FTP démarré sur DISK simulé.\n");

    while (true) {
        char *cb = NULL; int f = -1;
        int len = vpo->ReceiveLongAlloc(&f, &cb);
        if (len <= 0) { if(cb) delete[] cb; continue; }

        char type[10], arg1[128], arg2[128];
        if (!ParseCommand(cb, type, arg1, arg2, 128)) { delete[] cb; continue; }
        delete[] cb;

        if (strcmp(type, "GET") == 0) {
            char *d = NULL; int s = 0;
            if (ReadFileContent(arg1, &d, &s)) {
                vpo->SendLong(f, d, s); 
                delete[] d; 
                char a[10]; rpo->ReceiveReliable(&f, a, 10);
            } else {
                vpo->SendLong(f, "ERROR", 6);
            }
        } else if (strcmp(type, "PUT") == 0) {
            rpo->SendReliable(f, "READY", 6);
            char *fc = NULL; int s = vpo->ReceiveLongAlloc(&f, &fc);
            if (s >= 0) {
                if (WriteFileContent(arg1, fc, s)) rpo->SendReliable(f, "ACK", 4);
                else rpo->SendReliable(f, "ERROR", 6);
                delete[] fc;
            }
        } else if (strcmp(type, "DEL") == 0) {
            if (fileSystem->Remove(arg1)) rpo->SendReliable(f, "ACK", 4); //
            else rpo->SendReliable(f, "ERROR", 6);
        } else if (strcmp(type, "MKDIR") == 0) {
            if (fileSystem->MakeDirectory(arg1)) rpo->SendReliable(f, "ACK", 4); //
            else rpo->SendReliable(f, "ERROR", 6);
        } else if (strcmp(type, "LIST") == 0) {
            char *l = NULL; int ll = 0;
            if (GetDirectoryListing(arg1, &l, &ll)) {
                vpo->SendLong(f, l, ll);
                delete[] l;
            }
        }
    }
}