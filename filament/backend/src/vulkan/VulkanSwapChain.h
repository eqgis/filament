/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TNT_FILAMENT_BACKEND_VULKANSWAPCHAIN_H
#define TNT_FILAMENT_BACKEND_VULKANSWAPCHAIN_H

#include "VulkanContext.h"
#include "VulkanDriver.h"

#include <backend/platforms/VulkanPlatform.h>

#include <bluevk/BlueVK.h>
#include <utils/FixedCapacityVector.h>

#include <memory>

using namespace bluevk;

namespace filament::backend {

struct VulkanHeadlessSwapChain;
struct VulkanSurfaceSwapChain;

// A wrapper around the platform implementation of swapchain.
struct VulkanSwapChain : public HwSwapChain {
    VulkanSwapChain(VulkanPlatform* platform, VulkanContext const& context, VmaAllocator allocator,
            std::shared_ptr<VulkanCommands> commands, VulkanStagePool& stagePool,
            void* nativeWindow, uint64_t flags, VkExtent2D extent = {0, 0});

    ~VulkanSwapChain();

    void present();

    void acquire(bool& reized);

    inline std::shared_ptr<VulkanTexture> getCurrentColor() const noexcept {
        return mColors[mCurrentSwapIndex];
    }

    inline std::shared_ptr<VulkanTexture> getDepth() const noexcept {
        return mDepth;
    }

    inline bool isFirstRenderPass() const noexcept {
        return mIsFirstRenderPass;
    }

    inline void markFirstRenderPass() noexcept {
        mIsFirstRenderPass = false;
    }

    inline VkExtent2D getExtent() noexcept {
        return mExtent;
    }

private:
    void update();

    VulkanPlatform* mPlatform;
    std::shared_ptr<VulkanCommands> mCommands;
    VmaAllocator mAllocator;
    VulkanStagePool& mStagePool;
    bool const mHeadless;

    // We create VulkanTextures based on VkImages. VulkanTexture has facilities for doing layout
    // transitions, which are useful here. We use std::shared_ptr because they will be shared with
    // VulkanRenderTarget.
    utils::FixedCapacityVector<std::shared_ptr<VulkanTexture>> mColors;
    std::shared_ptr<VulkanTexture> mDepth;
    VkExtent2D mExtent;
    VkSemaphore mImageReady;
    uint32_t mCurrentSwapIndex;
    bool mAcquired;
    bool mIsFirstRenderPass;
};


}// namespace filament::backend

#endif// TNT_FILAMENT_BACKEND_VULKANSWAPCHAIN_H
