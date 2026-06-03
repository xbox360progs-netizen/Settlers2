#pragma once
#include <xtl.h>

typedef void (*JobFunction)(void*);

struct Job
{
    JobFunction func;
    void* data;
};

class JobManager
{
public:
    JobManager();
    ~JobManager();

    void Initialize(int numWorkers, const int* processorForWorker = NULL);
    void Shutdown();

    void Submit(JobFunction func, void* data);
    void WaitAll();

private:
    static const int QUEUE_SIZE = 256;

    Job m_jobs[QUEUE_SIZE];
    volatile LONG m_head;
    volatile LONG m_tail;
    volatile LONG m_counter;

    int m_numWorkers;
    struct WorkerThread
    {
        JobManager* owner;
        HANDLE handle;
        HANDLE wakeEvent;
        volatile bool running;
        int processor;
    }* m_workers;

    void Push(JobFunction func, void* data);
    bool TryPop(Job& job);

    static DWORD WINAPI WorkerProc(LPVOID param);
};
