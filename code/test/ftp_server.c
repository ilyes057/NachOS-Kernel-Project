#include "syscall.h"

int main() {
    // Lance le serveur en mode user (il gère sa sandbox server_dir)
    FtpStartServer();
    Exit(0);
}