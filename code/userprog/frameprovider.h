#ifndef FRAMEPROVIDER_H
#define FRAMEPROVIDER_H

#include "bitmap.h"
#include "synch.h"   

class FrameProvider {
public:
    FrameProvider(int numPhysPages);
    ~FrameProvider();

    int GetEmptyFrame();

    void ReleaseFrame(int frame);

    int NumAvailFrame();

private:
    BitMap *frames;   
    Lock *lock;       
    int numFrames;    
};

#endif // FRAMEPROVIDER_H
