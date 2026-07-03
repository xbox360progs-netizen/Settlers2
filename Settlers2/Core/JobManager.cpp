#include "stdafx.h"
#include "JobManager.h"

struct JobManager::Impl {
    static const int QUEUE_SIZE = 256;

    Job m_jobs[QUEUE_SIZE];
    volatile long m_head;
    volatile long m_tail;
    volatile long m_counter;

    int m_numWorkers;
    struct WorkerThread {
        JobManager* owner;
        HANDLE handle;
        HANDLE wakeEvent;
        volatile bool running;
        int processor;
    }* m_workers;

    Impl() : m_head(0), m_tail(0), m_counter(0), m_numWorkers(0), m_workers(NULL) {}
};

JobManager::JobManager() : m_impl(new Impl)
{
}

JobManager::~JobManager()
{
    Shutdown();
    delete m_impl;
}

void JobManager::Initialize(int numWorkers, const int* processorForWorker)
{
    if (m_impl->m_workers) return;

    m_impl->m_numWorkers = numWorkers;
    m_impl->m_workers = new Impl::WorkerThread[numWorkers];

    for (int i = 0; i < numWorkers; ++i)
    {
        m_impl->m_workers[i].owner = this;
        m_impl->m_workers[i].running = true;
        m_impl->m_workers[i].processor = processorForWorker ? processorForWorker[i] : -1;
        m_impl->m_workers[i].wakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_impl->m_workers[i].handle = CreateThread(NULL, 0, WorkerProc, &m_impl->m_workers[i], 0, NULL);
    }
}

void JobManager::Shutdown()
{
    if (!m_impl->m_workers) return;

    for (int i = 0; i < m_impl->m_numWorkers; ++i)
        m_impl->m_workers[i].running = false;

    for (int i = 0; i < m_impl->m_numWorkers; ++i)
        SetEvent(m_impl->m_workers[i].wakeEvent);

    for (int i = 0; i < m_impl->m_numWorkers; ++i)
    {
        WaitForSingleObject(m_impl->m_workers[i].handle, INFINITE);
        CloseHandle(m_impl->m_workers[i].handle);
        CloseHandle(m_impl->m_workers[i].wakeEvent);
    }

    delete[] m_impl->m_workers;
    m_impl->m_workers = NULL;
    m_impl->m_numWorkers = 0;
}

void JobManager::Submit(JobFunction func, void* data)
{
    long idx = m_impl->m_head;
    long slot = idx & (Impl::QUEUE_SIZE - 1);

    m_impl->m_jobs[slot].func = func;
    m_impl->m_jobs[slot].data = data;

    MemoryBarrier();

    m_impl->m_head = idx + 1;
    InterlockedIncrement(&m_impl->m_counter);
}

void JobManager::WaitAll()
{
    for (int i = 0; i < m_impl->m_numWorkers; ++i)
        SetEvent(m_impl->m_workers[i].wakeEvent);

    while (m_impl->m_counter > 0)
        Sleep(0);
}

bool JobManager::TryPop(Job& job)
{
    long currentTail = m_impl->m_tail;
    long currentHead = m_impl->m_head;

    if (currentTail >= currentHead)
        return false;

    long nextTail = currentTail + 1;
    if (InterlockedCompareExchange(&m_impl->m_tail, nextTail, currentTail) == currentTail)
    {
        long slot = currentTail & (Impl::QUEUE_SIZE - 1);
        job = m_impl->m_jobs[slot];
        return true;
    }

    return false;
}

unsigned long __stdcall JobManager::WorkerProc(void* param)
{
    Impl::WorkerThread* w = (Impl::WorkerThread*)param;
    JobManager* self = w->owner;

#ifdef _XBOX
    if (w->processor >= 0)
        XSetThreadProcessor(GetCurrentThread(), w->processor);
#endif

    while (w->running)
    {
        Job job;
        bool gotJob = false;

        while (self->TryPop(job))
        {
            gotJob = true;
            job.func(job.data);
            InterlockedDecrement(&self->m_impl->m_counter);
        }

        if (!gotJob)
            WaitForSingleObject(w->wakeEvent, INFINITE);
    }

    return 0;
}
