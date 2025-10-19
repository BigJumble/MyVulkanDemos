#include "sync.hpp"

namespace offload
{

  FrameSyncObjects createFrameSyncObjects( vk::raii::Device const & device, size_t maxFramesInFlight )
  {
    FrameSyncObjects syncObjects;

    syncObjects.imageAvailableSemaphores.reserve( maxFramesInFlight );
    syncObjects.renderFinishedSemaphores.reserve( maxFramesInFlight );
    syncObjects.presentFences.reserve( maxFramesInFlight );

    for ( size_t i = 0; i < maxFramesInFlight; ++i )
    {
      syncObjects.imageAvailableSemaphores.emplace_back( device, vk::SemaphoreCreateInfo{} );
      syncObjects.renderFinishedSemaphores.emplace_back( device, vk::SemaphoreCreateInfo{} );
      syncObjects.presentFences.emplace_back( device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled } );
    }

    return syncObjects;
  }

}  // namespace offload
