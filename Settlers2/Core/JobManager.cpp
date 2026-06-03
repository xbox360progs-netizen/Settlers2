#include "stdafx.h"
#include "JobManager.h"

JobManager::JobManager()
    : m_head(0)
    , m_tail(0)
    , m_counter(0)
    , m_numWorkers(0)
    , m_workers(NULL)
{
}

JobManager::~JobManager()
{
    Shutdown();
}

void JobManager::Initialize(int numWorkers, const int* processorForWorker)
{
    if (m_workers) return;

    m_numWorkers = numWorkers;
    m_workers = new WorkerThread[numWorkers];

    for (int i = 0; i < numWorkers; ++i)
    {
        m_workers[i].owner = this;
        m_workers[i].running = true;
        m_workers[i].processor = processorForWorker ? processorForWorker[i] : -1;
        m_workers[i].wakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_workers[i].handle = CreateThread(NULL, 0, WorkerProc, &m_workers[i], 0, NULL);
    }
}

void JobManager::Shutdown()
{
    if (!m_workers) return;

    for (int i = 0; i < m_numWorkers; ++i)
        m_workers[i].running = false;

    // Wake all workers so they exit
    for (int i = 0; i < m_numWorkers; ++i)
        SetEvent(m_workers[i].wakeEvent);

    for (int i = 0; i < m_numWorkers; ++i)
    {
        WaitForSingleObject(m_workers[i].handle, INFINITE);
        CloseHandle(m_workers[i].handle);
        CloseHandle(m_workers[i].wakeEvent);
    }

    delete[] m_workers;
    m_workers = NULL;
    m_numWorkers = 0;
}

void JobManager::Submit(JobFunction func, void* data)
{
    Push(func, data);
}

void JobManager::WaitAll()
{
    // Signal all workers that jobs are waiting
    for (int i = 0; i < m_numWorkers; ++i)
        SetEvent(m_workers[i].wakeEvent);

    // Spin until all jobs complete
    while (m_counter > 0)
        Sleep(0);
}

void JobManager::Push(JobFunction func, void* data)
{
    LONG idx = m_head;
    LONG slot = idx & (QUEUE_SIZE - 1);

    m_jobs[slot].func = func;
    m_jobs[slot].data = data;

    MemoryBarrier();

    m_head = idx + 1;
    InterlockedIncrement(&m_counter);
}

bool JobManager::TryPop(Job& job)
{
    LONG currentTail = m_tail;
    LONG currentHead = m_head;

    if (currentTail >= currentHead)
        return false;

    LONG nextTail = currentTail + 1;
    if (InterlockedCompareExchange(&m_tail, nextTail, currentTail) == currentTail)
    {
        LONG slot = currentTail & (QUEUE_SIZE - 1);
        job = m_jobs[slot];
        return true;
    }

    return false;
}

DWORD WINAPI JobManager::WorkerProc(LPVOID param)
{
    WorkerThread* w = (WorkerThread*)param;
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
            InterlockedDecrement(&self->m_counter);
        }

        if (!gotJob)
            WaitForSingleObject(w->wakeEvent, INFINITE);
    }

    return 0;
}
