#include "stdafx.h"
#include "TransportJobManager.h"

namespace World {

    TransportJobManager::TransportJobManager()
        : m_flagManager(NULL), m_roadManager(NULL), m_carrierManager(NULL), m_warehouse(NULL)
    {
    }

    TransportJobManager::~TransportJobManager()
    {
    }
}
