#include "filetransfer.h"
#include "system.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

FileTransfer::FileTransfer(VarPostOffice *vpo_, ReliablePostOffice *rpo_, const char *baseDir)
    : vpo(vpo_), rpo(rpo_) {
    // Initialisation du dossier racine (Sandbox)
    if (baseDir) {
        strncpy(baseDirectory, baseDir, 127);
        baseDirectory[127] = '\0';
    } else {
        strcpy(baseDirectory, ".");
    }

    // Création physique du dossier racine (Permission 0777)
    mkdir(baseDirectory, 0777); 
}

FileTransfer::~FileTransfer() {}
void FileTransfer::GetRealLocalPath(const char *filename, char *outBuffer, int maxLen) {
    if (!filename || strlen(filename) == 0 || strcmp(filename, ".") == 0) {
        strncpy(outBuffer, baseDirectory, maxLen);
    } else {
        snprintf(outBuffer, maxLen, "%s/%s", baseDirectory, filename);
    }
}

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
    char realPath[512];
    GetRealLocalPath(filename, realPath, 512);

    struct stat s;
    if (stat(realPath, &s) == 0) {
        if (S_ISDIR(s.st_mode)) {
            printf("[ERREUR] '%s' est un dossier. Lecture interdite.\n", filename);
            return false;
        }
    }

    int fd = open(realPath, O_RDONLY);
    if (fd < 0) return false;

    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (size < 0) { close(fd); return false; }

    char *data = new char[size + 1];
    int totalRead = 0;
    while (totalRead < size) {
        int r = read(fd, data + totalRead, size - totalRead);
        if (r <= 0) break;
        totalRead += r;
    }
    close(fd);
    
    if (totalRead != size) { delete[] data; return false; }

    *outData = data;
    *outSize = (int)size;
    return true;
}

bool FileTransfer::WriteFileContent(const char *filename, const char *data, int size) {
    char realPath[512];
    GetRealLocalPath(filename, realPath, 512);

    int fd = open(realPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    int totalWritten = 0;
    while (totalWritten < size) {
        int w = write(fd, data + totalWritten, size - totalWritten);
        if (w <= 0) { close(fd); return false; }
        totalWritten += w;
    }
    close(fd);
    return (totalWritten == size);
}


bool FileTransfer::GetDirectoryListing(const char *path, char **outList, int *outLen) {
    char realPath[512];
    GetRealLocalPath(path, realPath, 512);

    DIR *d = opendir(realPath);
    if (!d) return false;

    char bigBuffer[4096]; bigBuffer[0] = '\0';
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
        strcat(bigBuffer, (dir->d_type == DT_DIR) ? "[DIR ] " : "[FILE] ");
        strcat(bigBuffer, dir->d_name);
        strcat(bigBuffer, "\n");
        if (strlen(bigBuffer) > 4000) break;
    }
    closedir(d);

    int len = strlen(bigBuffer);
    char *res = new char[len + 1];
    strcpy(res, bigBuffer);
    *outList = res; *outLen = len;
    return true;
}

void FileTransfer::ListLocal() {
    char *list = NULL; int len = 0;
    if (GetDirectoryListing(".", &list, &len)) {
        printf("\n=== Fichiers Locaux (%s) ===\n%s------------------------------\n", baseDirectory, list);
        delete[] list;
    } else {
        printf("Erreur lecture dossier local.\n");
    }
}

void FileTransfer::PrintWorkingDir() {
    printf("Dossier Racine (Sandbox): %s\n", baseDirectory);
}

bool FileTransfer::LocalMkdir(const char *name) {
    char p[512]; GetRealLocalPath(name, p, 512);
    return (mkdir(p, 0777) == 0);
}
bool FileTransfer::LocalRmdir(const char *name) {
    char p[512]; GetRealLocalPath(name, p, 512);
    return (rmdir(p) == 0);
}
bool FileTransfer::LocalDelete(const char *name) {
    char p[512]; GetRealLocalPath(name, p, 512);
    return (unlink(p) == 0);
}
bool FileTransfer::LocalRename(const char *oldName, const char *newName) {
    char pOld[512], pNew[512];
    GetRealLocalPath(oldName, pOld, 512);
    GetRealLocalPath(newName, pNew, 512);
    return (rename(pOld, pNew) == 0);
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
bool FileTransfer::RequestRename(int s, const char *oldN, const char *newN) {
    char c[256]; snprintf(c, 256, "REN %s %s", oldN, newN); return SendSimpleCmd(vpo, rpo, s, c);
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
    printf("[SERVER] Racine: '%s'. En attente...\n", baseDirectory);

    while (true) {
        char *cb=NULL; int f=-1;
        int len = vpo->ReceiveLongAlloc(&f, &cb);
        if (len <= 0) { if(cb) delete[] cb; continue; }

        char buffer[MAX_CMD_LEN];
        strncpy(buffer, cb, MAX_CMD_LEN-1); buffer[MAX_CMD_LEN-1] = '\0';
        delete[] cb;

        char type[10], arg1[128], arg2[128];
        if (!ParseCommand(buffer, type, arg1, arg2, 128)) continue;

        printf("[SERVER] CMD '%s' de %d (Args: %s, %s)\n", type, f, arg1, arg2);

        char real1[512]; GetRealLocalPath(arg1, real1, 512);

        if (strcmp(type, "GET") == 0) {
            char *d=NULL; int s=0;
            if (ReadFileContent(arg1, &d, &s)) {
                vpo->SendLong(f, d, s); 
                delete[] d; 
                char a[10]; rpo->ReceiveReliable(&f, a, 10);
            } else {
                vpo->SendLong(f, "ERROR File Not Found or Invalid", 30);
            }

        } else if (strcmp(type, "PUT") == 0) {
            rpo->SendReliable(f, "READY", 6);
            char *fc=NULL; int s = vpo->ReceiveLongAlloc(&f, &fc);
            if (s >= 0) {
                if (WriteFileContent(arg1, fc, s)) rpo->SendReliable(f, "ACK", 4);
                else rpo->SendReliable(f, "ERROR Write Failed", 19);
                delete[] fc;
            }

        } else if (strcmp(type, "DEL") == 0) {
            if (unlink(real1) == 0) rpo->SendReliable(f, "ACK", 4);
            else rpo->SendReliable(f, "ERROR Delete Failed", 20);

        } else if (strcmp(type, "MKDIR") == 0) {
            if (mkdir(real1, 0777) == 0) rpo->SendReliable(f, "ACK", 4);
            else rpo->SendReliable(f, "ERROR Mkdir Failed", 19);

        } else if (strcmp(type, "RMDIR") == 0) {
            if (rmdir(real1) == 0) rpo->SendReliable(f, "ACK", 4);
            else rpo->SendReliable(f, "ERROR Rmdir Failed", 19);

        } else if (strcmp(type, "REN") == 0) {
            char real2[512]; GetRealLocalPath(arg2, real2, 512);
            if (rename(real1, real2) == 0) rpo->SendReliable(f, "ACK", 4);
            else rpo->SendReliable(f, "ERROR Rename Failed", 20);

        } else if (strcmp(type, "LIST") == 0) {
            char *l=NULL; int ll=0;
            if (GetDirectoryListing(arg1, &l, &ll)) {
                vpo->SendLong(f, l, ll);
                delete[] l;
            } else {
                vpo->SendLong(f, "ERROR List Failed", 18);
            }
        }
    }
}