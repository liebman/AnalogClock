//============================================================================
// Name        : NTPTest.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "NTP.h"

#include <sys/time.h>
#include <unistd.h>
#include <math.h>

class NTPTest : public NTP
{
    public:
    NTPTest(NTPPersist * persist) : NTP(persist)
    {
    }
};

uint32_t last_time;
double current_offset = 0.0;
double fake_drift_ppm = 600.0;

int getTime(uint32_t *result)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    // add fake drift to offset
    if (last_time)
    {
        double drift = fake_drift_ppm * (((double)tp.tv_sec - (double)last_time) / 1000000.);
        printf("this drift: %lf\n", drift);
        current_offset += drift;
    }

    // apply the current offset
    double x = (double)tp.tv_sec + (double)tp.tv_usec / 1000000.;
    x += current_offset;
    tp.tv_sec  = (int)x;
    tp.tv_usec = (uint32_t)((x-(double)tp.tv_sec) * 1000000.);

    // sync to the next second boundary
    if (tp.tv_usec != 0)
    {
        usleep(1000000-tp.tv_usec);
    }
    *result = tp.tv_sec+1;
    last_time = *result;
    return 0;
}

int main(int argc, char**argv)
{
    const char *server = "192.168.0.31";
    printf("argc: %d\n", argc);
    if (argc > 1)
    {
        server = argv[1];
    }
    NTPPersist persist;
    memset(&persist, 0, sizeof(persist));
    NTPTest test(&persist);
    test.begin();
    double offset;
    for (int i = 0; i < 1000; ++i)
    {
        int interval = 30;
        double drift = test.getDrift();
        printf("fake_drift: %fppm drift: %fppm\n", fake_drift_ppm, drift);
        double drift_compensation = (double)interval * drift / 1000000;
        current_offset += drift_compensation;
        printf("applying drift compensation: %f current_offset: %f\n", drift_compensation, current_offset);
        if (drift == 0.0 || ((i%5)==0))
        {
            int err = test.getOffset(server, &offset, &getTime);
            if (!err)
            {
                current_offset += offset;
                printf("******OFFSET: %f current_offset: %f\n", offset, current_offset);
            }
        }
        printf("sleeping %d seconds\n", interval);
        sleep(interval);
    }

    printf("Done!\n");
	return 0;
}
