#pragma once
#include <vector>
#include "../Shared/RenderFrame.h"
#include "../Presentation/Migration/ICarrierSource.h"

namespace Scene {

class CarrierPresentation {
public:
    CarrierPresentation();

    void SetCarrierSource(ICarrierSource* source);

    void BuildRenderFrame(RenderFrame& frame);

private:
    void CollectCarriers(std::vector<RenderWorker>& out);

    ICarrierSource* m_source;
};

} // namespace Scene
