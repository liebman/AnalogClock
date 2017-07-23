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

#define SPEEDUP_FACTOR 100
#define MAX_SLEEP      (3600 / SPEEDUP_FACTOR)
#define PERSIST_FILE   "/tmp/ntp_persist.data"

uint32_t last_time;
double current_offset = 0.0;
double fake_drift_ppm = 600.0;
uint32_t sleep_left;
NTPPersist persist;

void loadPersist()
{
    printf("loadPersist()\n");
    FILE *fp = fopen(PERSIST_FILE, "r");
    if (fp == NULL)
    {
        printf("loadPersist: failed to open '%s'!\n", PERSIST_FILE);
        return;
    }
    if (fread(&persist, sizeof(persist), 1, fp) != 1)
    {
        printf("loadPersist: fread() failed!!!\n");
        memset(&persist, 0, sizeof(persist));
    }
    fclose(fp);
}

void savePersist()
{
    printf("savePersist()\n");
    FILE *fp = fopen(PERSIST_FILE, "w");
    if (fp == NULL)
    {
        printf("savePersist: failed to open '%s'!\n", PERSIST_FILE);
        return;
    }
    if (fwrite(&persist, sizeof(persist), 1, fp) != 1)
    {
        printf("savePersist: fwrite() failed!!!\n");
    }
    fclose(fp);
}

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
    NTPRunTime runtime;
    memset(&persist, 0, sizeof(persist));
    memset(&runtime, 0, sizeof(runtime));
    loadPersist();
    NTP test(&runtime, &persist, &savePersist);
    test.begin();
    for (int i = 0; i < 1000; ++i)
    {
        double offset = 0.0;
        int err = test.getOffsetUsingDrift(&offset, &getTime);
        if (!err)
        {
            current_offset += offset;
            printf("****** DRIFT:  %f current_offset: %f\n", offset, current_offset);
        }

        if (sleep_left == 0)
        {
            int err = test.getOffset(server, &offset, &getTime);
            if (!err)
            {
                current_offset += offset;
                printf("****** OFFSET: %f current_offset: %f\n", offset, current_offset);
            }
            sleep_left = test.getPollInterval() / SPEEDUP_FACTOR;
        }
        int interval = MAX_SLEEP;
        if (sleep_left > MAX_SLEEP)
        {
            sleep_left -= MAX_SLEEP;
        }
        else
        {
            interval = sleep_left;
            sleep_left = 0;
        }
        printf("sleeping %d seconds (sleep_left: %u)\n", interval, sleep_left);
        sleep(interval);
    }

    printf("Done!\n");
	return 0;
}
