#pragma once

#ifndef _WIN32
#include <cstdint> 

typedef union _LARGE_INTEGER {
    struct {
        uint32_t LowPart;
        int32_t  HighPart;
    } DUMMYSTRUCTNAME;
    struct {
        uint32_t LowPart;
        int32_t  HighPart;
    } u;
    int64_t QuadPart;
} LARGE_INTEGER; 

#endif

class Timer
{
    private:
        double cc,fq, st, dt1,st1;  int iFR;
        LARGE_INTEGER CC;

    public:
        double t, dt, FR, iv, iv1;
        // delta time, FrameRate, interval, intervalFR

        Timer();
        bool update(bool updFR = false);
};

