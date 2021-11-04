#include "camera/itscam/compat_pumatronix.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

HANDLE CreateSemaphore(void *, int b, int, void*)
{
    HANDLE t;
    t = (HANDLE) malloc(sizeof(sem_t));
    sem_init(t,0,b);
    return t;
}

void CloseHandle(HANDLE a)
{
    Sleep(3);
    sem_destroy(a);
    free(a);
}

DWORD GetTickCount()
{
    struct timeval detail_time;

    gettimeofday(&detail_time,NULL);

    DWORD t = (DWORD)(1000 * (uint32_t)(detail_time.tv_sec) + (uint32_t)(detail_time.tv_usec)/1000);

    return t;
}

void ReleaseSemaphore(HANDLE a, int, void*)
{
    int v;
    sem_getvalue(a,&v);
    if(v==0) sem_post(a);
}

int WaitForSingleObject(HANDLE a, int b)
{
    struct timespec t;
    if(b==INFINITE) sem_wait(a);
    else{
        clock_gettime(CLOCK_REALTIME,&t);
        t.tv_sec = t.tv_sec + b/1000 + (t.tv_nsec + (b%1000)*1000000)/1000000000;
        t.tv_nsec = (t.tv_nsec + (b%1000)*1000000)%1000000000;
        if(sem_timedwait(a,&t)==-1) return errno;
    }
    return 0;
}

void Sleep(int a)
{
    usleep(a*1000);
}

pthread_t *CreateThread(void *, int, void *(*c)(void *), void *d, int, unsigned int*)
{
    pthread_t *t;
    t = (pthread_t *) malloc(sizeof(pthread_t));
    if(pthread_create(t,NULL,c,d)==0) return t;
    else return 0;
}

void GetLocalTime(SYSTEMTIME *st)
{
    struct timeval tv;
    struct tm t;
    gettimeofday(&tv,NULL);
    localtime_r(&tv.tv_sec,&t);
    st->wYear = t.tm_year + 1900;
    st->wMonth = t.tm_mon + 1;
    st->wDay = t.tm_mday;
    st->wHour = t.tm_hour;
    st->wMinute = t.tm_min;
    st->wSecond = t.tm_sec;
    st->wMilliseconds = (int)(tv.tv_usec/1000);
    st->wDayOfWeek = t.tm_wday;
}
