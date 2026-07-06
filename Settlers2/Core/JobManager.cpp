#include "stdafx.h"
#include "JobManager.h"
#include "../Platform/Affinity.h"

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
        m_workers[i].thread = new Platform::Thread();
        m_workers[i].wakeEvent = new Platform::Event(false);
        m_workers[i].thread->Start(WorkerProc, &m_workers[i]);
    }
}

void JobManager::Shutdown()
{
    if (!m_workers) return;

    for (int i = 0; i < m_numWorkers; ++i)
        m_workers[i].running = false;

    for (int i = 0; i < m_numWorkers; ++i)
        m_workers[i].wakeEvent->Signal();

    for (int i = 0; i < m_numWorkers; ++i)
    {
        m_workers[i].thread->Join();
        delete m_workers[i].thread;
        delete m_workers[i].wakeEvent;
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
    for (int i = 0; i < m_numWorkers; ++i)
        m_workers[i].wakeEvent->Signal();

    while (m_counter > 0)
        Platform::Sleep(0);
}

void JobManager::Push(JobFunction func, void* data)
{
    long idx = m_head;
    long slot = idx & (QUEUE_SIZE - 1);

    m_jobs[slot].func = func;
    m_jobs[slot].data = data;

    Platform::MemoryFence();

    m_head = idx + 1;
    Platform::AtomicIncrement(&m_counter);
}

bool JobManager::TryPop(Job& job)
{
    long currentTail = m_tail;
    long currentHead = m_head;

    if (currentTail >= currentHead)
        return false;

    long nextTail = currentTail + 1;
    if (Platform::AtomicCompareExchange(&m_tail, nextTail, currentTail) == currentTail)
    {
        long slot = currentTail & (QUEUE_SIZE - 1);
        job = m_jobs[slot];
        return true;
    }

    return false;
}

unsigned int __stdcall JobManager::WorkerProc(void* param)
{
    WorkerThread* w = (WorkerThread*)param;
    JobManager* self = w->owner;

    if (w->processor >= 0)
        Platform::SetThreadAffinity(w->thread->GetNativeHandle(), w->processor);

    while (w->running)
    {
        Job job;
        bool gotJob = false;

        while (self->TryPop(job))
        {
            gotJob = true;
            job.func(job.data);
            Platform::AtomicDecrement(&self->m_counter);
        }

        if (!gotJob)
            w->wakeEvent->Wait(INFINITE);
    }

    return 0;
}
