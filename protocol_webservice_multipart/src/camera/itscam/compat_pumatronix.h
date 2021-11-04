#ifndef COMPAT_PUMATRONIX_H
#define COMPAT_PUMATRONIX_H

#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>

#define HANDLE sem_t *
#define INFINITE -1
#define WAIT_TIMEOUT ETIMEDOUT
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#define DWORD uint32_t

typedef struct _SYSTEMTIME {
	unsigned short int wYear;
	unsigned short int wMonth;
	unsigned short int wDayOfWeek;
	unsigned short int wDay;
	unsigned short int wHour;
	unsigned short int wMinute;
	unsigned short int wSecond;
	unsigned short int wMilliseconds;
} SYSTEMTIME;

void GetLocalTime(SYSTEMTIME*);
DWORD GetTickCount();
HANDLE CreateSemaphore(void *a, int b, int c, void *d);
void CloseHandle(HANDLE a);
void ReleaseSemaphore(HANDLE a, int b, void *c);
int WaitForSingleObject(HANDLE a, int b);
void Sleep(int a);
pthread_t *CreateThread(void *a, int b, void *(*c)(void *), void *d, int e, unsigned int *f);

#endif /* COMPAT_PUMATRONIX_H */