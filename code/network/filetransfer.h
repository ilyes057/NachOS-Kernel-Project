#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "varpost.h"
#include "reliablepost.h"

class FileTransfer {
  public:
    FileTransfer(VarPostOffice *vpo_, ReliablePostOffice *rpo_);
    ~FileTransfer();

    bool RequestList(int serverMachine, const char *remotePath);        // ls
    bool RequestGet(int serverMachine, const char *remoteFile, const char *localPath); // get
    bool RequestPut(int serverMachine, const char *localFile, const char *remoteDest); // put
    bool RequestDelete(int serverMachine, const char *remoteFile);      // rm / delete
    bool RequestMkdir(int serverMachine, const char *remotePath);       // mkdir
    bool RequestRmdir(int serverMachine, const char *remotePath);       // rmdir
    
    bool RequestRename(int serverMachine, const char *oldName, const char *newName);               
    // --- SERVEUR ---
    void StartServer();

  private:
    VarPostOffice *vpo;
    ReliablePostOffice *rpo;
    // Helpers Système
    bool ReadFileContent(const char *filename, char **outData, int *outSize);
    bool WriteFileContent(const char *filename, const char *data, int size);
    bool GetDirectoryListing(const char *path, char **outList, int *outLen);
    bool ParseCommand(const char *cmd, char *type, char *arg1, char *arg2, int maxLen);

    static const int MAX_CMD_LEN = 256;
};

#endif