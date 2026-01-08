#include "frameprovider.h"

#include "system.h"  
#include <strings.h>  

FrameProvider::FrameProvider(int numPhysPages) {
    ASSERT(numPhysPages > 0);
    numFrames = numPhysPages;
    frames = new BitMap(numPhysPages);
    lock = new Lock("FrameProvider");
}

FrameProvider::~FrameProvider() {
    delete lock;
    delete frames;
}

int FrameProvider::GetEmptyFrame() {
    lock->Acquire();
    int frame = frames->Find();
    if (frame < 0) {
        DEBUG('a', "No free frame available!\n");
        lock->Release();
        return -1;
    }
    bzero(&machine->mainMemory[frame * PageSize], PageSize);
    lock->Release();

    return frame;
}

void FrameProvider::ReleaseFrame(int frame) {
    lock->Acquire();
    ASSERT(frame >= 0 && frame < numFrames);
    frames->Clear(frame);
    lock->Release();
}

int FrameProvider::NumAvailFrame() {
    lock->Acquire();
    int num = frames->NumClear();
    lock->Release();
    return num;
}