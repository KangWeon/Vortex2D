//
//  Instance.cpp
//  Vortex2D
//

#include "Instance.h"

#include <iostream>

namespace Vortex2D
{
namespace Renderer
{
void RequestAdapterCallback(WGPUAdapterId received, void* userdata)
{
  auto promise = reinterpret_cast<std::promise<WGPUAdapterId>*>(userdata);
  promise->set_value(received);
}

Instance::Instance() : mAdapter(0)
{
  std::promise<WGPUAdapterId> adapterPromise;

  wgpu_request_adapter_async(nullptr, 2 | 4 | 8, RequestAdapterCallback, (void*)&adapterPromise);

  mAdapter = adapterPromise.get_future().get();
}

Instance::~Instance()
{
  if (mAdapter != 0)
  {
    wgpu_adapter_destroy(mAdapter);
  }
}

WGPUAdapterId Instance::GetAdapter() const
{
  return mAdapter;
}

}  // namespace Renderer
}  // namespace Vortex2D
