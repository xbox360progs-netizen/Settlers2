#pragma once

namespace World {

class ILogisticsPublisher {
public:
    virtual void PublishRequests() = 0;
    virtual void PublishSupply() = 0;
    virtual ~ILogisticsPublisher() {}
};

} // namespace World
