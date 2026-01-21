#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "varpost.h"
#include "reliablepost.h"

class FileTransfer {
  public:
    FileTransfer(VarPostOffice *vpo_, ReliablePostOffice *rpo_, const char *baseDir = ".");
    ~FileTransfer();

    bool RequestList(int serverMachine, const char *remotePath);        // ls
    bool RequestGet(int serverMachine, const char *remoteFile, const char *localPath); // get
    bool RequestPut(int serverMachine, const char *localFile, const char *remoteDest); // put
    bool RequestDelete(int serverMachine, const char *remoteFile);      // rm / delete
    bool RequestMkdir(int serverMachine, const char *remotePath);       // mkdir
    bool RequestRmdir(int serverMachine, const char *remotePath);       // rmdir
    
    bool RequestRename(int serverMachine, const char *oldName, const char *newName);

    void ListLocal();                         
    bool LocalMkdir(const char *name);        
    bool LocalRmdir(const char *name);        
    bool LocalDelete(const char *name);       
    
    bool LocalRename(const char *oldName, const char *newName); 
    void PrintWorkingDir();                   

    // --- SERVEUR ---
    void StartServer();

  private:
    VarPostOffice *vpo;
    ReliablePostOffice *rpo;
    char baseDirectory[128]; // Notre Sandbox

    void GetRealLocalPath(const char *filename, char *outBuffer, int maxLen);

    // Helpers Système
    bool ReadFileContent(const char *filename, char **outData, int *outSize);
    bool WriteFileContent(const char *filename, const char *data, int size);
    bool GetDirectoryListing(const char *path, char **outList, int *outLen);
    bool ParseCommand(const char *cmd, char *type, char *arg1, char *arg2, int maxLen);

    static const int MAX_CMD_LEN = 256;
};

#endif