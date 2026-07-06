#pragma once

#include "../Platform/Thread.h"
#include "../Platform/Event.h"
#include "../Platform/Atomic.h"

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
    volatile long m_head;
    volatile long m_tail;
    volatile long m_counter;

    int m_numWorkers;
    struct WorkerThread
    {
        JobManager* owner;
        Platform::Thread* thread;
        Platform::Event* wakeEvent;
        volatile bool running;
        int processor;
    }* m_workers;

    void Push(JobFunction func, void* data);
    bool TryPop(Job& job);

    static unsigned int __stdcall WorkerProc(void* param);
};
