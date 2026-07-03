#pragma once

namespace World {

    enum ConstructionState {
        CS_Pending,
        CS_WaitingForResources,
        CS_Building,
        CS_Completed
    };

}
