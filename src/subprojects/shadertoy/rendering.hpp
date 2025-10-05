#pragma once

#include "bootstrap.hpp"

namespace rendering {

void recordCommandBuffer(
  vk::raii::CommandBuffer & cmd,
  core::SwapchainBundle &   swapchainBundle,
  uint32_t                  imageIndex );

} // namespace rendering
