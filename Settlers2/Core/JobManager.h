#pragma once

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
    bool TryPop(Job& job);

    struct Impl;
    Impl* m_impl;

    static unsigned long __stdcall WorkerProc(void* param);
};
