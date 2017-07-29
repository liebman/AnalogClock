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

uint32_t start_time   = 0;
uint32_t last_time    = 0;
double current_offset = 0.0;
double fake_drift_ppm = 1.0*SPEEDUP_FACTOR;
uint32_t sleep_left   = 0;

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

void adjustOffsetByDrift()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    if (!start_time)
    {
        start_time = (uint32_t)tp.tv_sec;
    }

    // add fake drift to offset
    if (last_time)
    {
        double drift = fake_drift_ppm * (((double)tp.tv_sec - (double)last_time) / 1000000.);
        current_offset += drift;
        double hours = (double)(tp.tv_sec - start_time) / (3600.0 / SPEEDUP_FACTOR);
        printf("HOURS: %f applying fake drift: %lfms for %ld seconds current_offset: %f\n", hours, drift, tp.tv_sec - last_time, current_offset);
    }
    last_time =  (uint32_t)tp.tv_sec;
}

int getTime(uint32_t *result)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    // apply the current offset
    double x = (double)tp.tv_sec + (double)tp.tv_usec / 1000000.;
    x += current_offset;
    tp.tv_sec  = (long)x;
    tp.tv_usec = (uint32_t)((x-(double)tp.tv_sec) * 1000000.);

    // sync to the next second boundary
    if (tp.tv_usec != 0)
    {
        usleep(1000000-tp.tv_usec);
    }
    *result = tp.tv_sec+1;
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
    NTP test(&runtime, &persist, &savePersist, SPEEDUP_FACTOR);
    test.begin();
    for (int i = 0; i < 1000; ++i)
    {
        adjustOffsetByDrift();

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
            sleep_left = test.getPollInterval();
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
        fflush(stdout);
        sleep(interval);
    }

    printf("Done!\n");
	return 0;
}
