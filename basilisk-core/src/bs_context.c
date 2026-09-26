
 /**
  MIT License

  Copyright (c) 2026 switch360hardflip <switch360hardflip@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  */

#include <stdlib.h>

#include "basilisk-core.gen.h"

#ifdef _WIN32
    #include <windows.h>
    #include <windowsx.h>
    #include <winuser.h>
    #include <dwmapi.h>
    #include <uxtheme.h>
    #include <vsstyle.h>
    #include <vssym32.h>
    #include <uiribbon.h>

    #include <dxgi1_6.h>
    #include <d3d12.h>

    #ifndef NDEBUG
        #include <dxgidebug.h>
        #include <d3d12sdklayers.h>
    #endif

    #define VK_USE_PLATFORM_WIN32_KHR
    #include <vulkan.h>

#elif defined(__linux__)
    #include <wayland-client.h>
    #include <wayland-cursor.h>
    #include <xdg-shell.h>
    #include <xdg-decoration-unstable-v1.h>
    #include <viewporter.h>
    #include <single-pixel-buffer-v1.h>
    #include <unistd.h>

    #define VK_USE_PLATFORM_WAYLAND_KHR
    #include <vulkan.h>
#endif

#include <time.h>
#include <assert.h>

#include <basilisk-core.h>
#include <bs_internal.h>

static void _bs_createSurface() {
    VkResult result = VK_SUCCESS;

    #ifdef _WIN32
    if (_bs_instance_->extensions.surface_type == BS_SURFACE_TYPE_WIN32) {
        const VkWin32SurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(0),
            .hwnd = _bs_scope_.context->hwnd,
        };

        result = vkCreateWin32SurfaceKHR(_bs_instance_->instance, &ci, NULL, &_bs_scope_.context->surface);
    }
    #elif defined(__linux__)
    if (_bs_instance_->extensions.surface_type == BS_SURFACE_TYPE_WAYLAND) {
        const VkWaylandSurfaceCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
            .display = _bs_scope_.context->display,
            .surface = _bs_scope_.context->_wl_surface,
        };

        if (!_bs_scope_.context->display) {
            BS_WARN("Cannot create Wayland surface for context \"%s\", display is NULL", _bs_scope_.context->title);
            return;
        }

        result = vkCreateWaylandSurfaceKHR(_bs_instance_->instance, &ci, NULL, &_bs_scope_.context->surface);
    }
    #else
    if (0) { }
    #endif
    else if (_bs_instance_->extensions.surface_type == BS_SURFACE_TYPE_HEADLESS) {
        const VkHeadlessSurfaceCreateInfoEXT ci = {
            .sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT,
        };

        result = vkCreateHeadlessSurfaceEXT(_bs_instance_->instance, &ci, NULL, &_bs_scope_.context->surface);
    }
    else {
        _bs_warnF("Surface type %d is not supported", _bs_instance_->extensions.surface_type);
    }

    if (result != VK_SUCCESS) {
        _bs_warnF("Failed to create surface for window \"%s\" (Vulkan result = %d)", _bs_scope_.context->title, result);
    }
}

BSAPI void _bs_queryProcedures(bs_Procedure* procedures, int count, void* dll_handle, void* dst) {
    unsigned char* destination = dst;
#define BS_STRING_GEN_2(TYPE, FUNC, ...) { .size = sizeof(TYPE), .func = #FUNC, __VA_OPT__(.is_required = __VA_ARGS__) },
#ifdef _WIN32
    for (int i = 0; i < count; i++) {
        void* data = dll_handle == 0 ?
            (void*)vkGetDeviceProcAddr(_bs_instance_->device, procedures[i].func) :
            (void*)GetProcAddress(dll_handle, procedures[i].func);

        if (data)
            memcpy(destination, &data, procedures[i].size);
        else if (procedures[i].is_required)
            _bs_warnF("Failed to query procedure \"%s\"", procedures[i].func);

        destination += procedures[i].size;
    }
#else
    bs_log("_bs_queryProcedures not implemented");
#endif
}



  /*==============================================================================
   * Swapchain
   *============================================================================*/

BSAPI bs_Image* _bs_swapchainImage() {
    return _bs_scope_.context->swapchain_image->image;
}

void _bs_destroySwapchain() {
    if (!_bs_scope_.context->swapchain_image)
        return;
    bs_Image* swapchain_image = _bs_scope_.context->swapchain_image->image;

    for (int i = 0; i < _bs_scope_.context->head.swaps_count; i++) {
        if (swapchain_image->_[i].vk_image_view)
            vkDestroyImageView(_bs_instance_->device, swapchain_image->_[i].vk_image_view, NULL);
        swapchain_image->_[i].vk_image_view = VK_NULL_HANDLE;
    }

    if (_bs_scope_.context->swapchain)
        vkDestroySwapchainKHR(_bs_instance_->device, _bs_scope_.context->swapchain, NULL);
    _bs_scope_.context->swapchain = VK_NULL_HANDLE;
}


#ifdef _WIN32

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    bs_ivec2 resolution;
    bs_U32 images_count;
} bs_SwapchainProperties;

static inline bs_SwapchainProperties _bs_swapchainProperties() {
    bs_Context* context = _bs_scope_.context;
    bs_SwapchainProperties props = { 0 };

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_bs_instance_->physical_device->vk_device, context->surface, &props.capabilities);

    props.resolution = BS_IV2(
        bs_clamp(context->dimensions.x, props.capabilities.minImageExtent.width, props.capabilities.maxImageExtent.width),
        bs_clamp(context->dimensions.y, props.capabilities.minImageExtent.height, props.capabilities.maxImageExtent.height)
    );

    props.resolution = BS_IV2(
        context->dimensions.x,
        context->dimensions.y
    );
    const int max_images_count = 3;
    if (props.capabilities.maxImageCount > max_images_count)
        props.capabilities.maxImageCount = max_images_count;

    props.images_count = props.capabilities.minImageCount + 1;
    if (props.capabilities.maxImageCount > 0 && props.images_count > props.capabilities.maxImageCount)
        props.images_count = props.capabilities.maxImageCount;

    return props;
}


  /*==============================================================================
   * DXGI Swapchain
   *============================================================================*/

static void _bs_awaitDxCommands() {
    ID3D12CommandQueue* queue = _bs_scope_.context->dx_command_queue;

    ID3D12Fence* fence = _bs_scope_.context->dx_fence;

    UINT64 value = ++_bs_scope_.context->dx_fence_value;

    HRESULT hr = queue->lpVtbl->Signal(queue, fence, value);
    if (FAILED(hr)) {
        BS_WARN_HRESULT("Signal", hr);
        return;
    }

    if (fence->lpVtbl->GetCompletedValue(fence) < value) {
        hr = fence->lpVtbl->SetEventOnCompletion(fence, value, _bs_scope_.context->dx_fence_event);

        if (FAILED(hr)) {
            BS_WARN_HRESULT("SetEventOnCompletion", hr);
            return;
        }

        WaitForSingleObject(_bs_scope_.context->dx_fence_event, INFINITE);
    }
}

static bs_Result _bs_dxgiSwapchain(bs_Context* context) {
    HRESULT hresult;
    VkResult vk_result;
    bs_Result result;
    bs_SwapchainProperties props = _bs_swapchainProperties();

    if (context->dxgi_swapchain) {
        _bs_awaitDxCommands();

        for (int i = 0; i < context->swapchain_image->head->swaps_count; i++) {
            bs_ImageSwaps* swap = context->swapchain_image->image->_ + i;
            vkDestroyImageView(_bs_instance_->device, swap->vk_image_view, NULL);
            vkDestroyImage(_bs_instance_->device, swap->vk_image, NULL);
            vkFreeMemory(_bs_instance_->device, swap->vk_memory, NULL);

            swap->vk_image_view = NULL;
            swap->vk_image = NULL;
            swap->vk_memory = NULL;
        }

        for (int i = 0; i < context->swapchain_image->head->swaps_count; i++) {
            bs_DxSwapchainImage* image = context->dx_swapchain_images + i;

            image->dx_image->lpVtbl->Release(image->dx_image);
            CloseHandle(image->dx_shared_handle);

            image->dx_image = NULL;
            image->dx_shared_handle = NULL;
        }

      //  _bs_destroySwapchain();

        hresult = context->dxgi_swapchain->lpVtbl->ResizeBuffers(
            context->dxgi_swapchain,
            context->swapchain_image->head->swaps_count,
            props.resolution.x,
            props.resolution.y,
            DXGI_FORMAT_R8G8B8A8_UNORM, // TODO
            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
        );

        if (FAILED(hresult)) {
            BS_WARN_HRESULT("ResizeBuffers", hresult);
            if (hresult == 0x887a0005) {
                hresult = _bs_instance_->dx_device->lpVtbl->GetDeviceRemovedReason(_bs_instance_->dx_device);
                BS_WARN_HRESULT("GetDeviceRemovedReason", hresult);
            }

           // return BS_RESULT_OK; // TODO
          //  return bs_convertHResult(hresult);
        }
    }

    _bs_instance_->max_swapchain_images_count = BS_MAX(_bs_instance_->max_swapchain_images_count, props.images_count);

    if (!context->dxgi_swapchain) {
        context->present_queue = BS_QUEUE(-1, -1, BS_OBJECT_SWAPCHAIN_IMAGE_BIT);
        result = bs_queue(context->present_queue, 0, BS_QUEUE_GRAPHICS_BIT);
        if (result != BS_RESULT_OK)
            return result;


       /**
        Command Queue
        */
        const auto node_count = _bs_instance_->dx_device->lpVtbl->GetNodeCount(_bs_instance_->dx_device);
        const UINT node_mask = node_count <= 1 ? 0 : _bs_props_.device_node_mask;
    
        const D3D12_COMMAND_QUEUE_DESC command_queue_desc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = node_mask,
        };

        hresult = _bs_instance_->dx_device->lpVtbl->CreateCommandQueue(
            _bs_instance_->dx_device, 
            &command_queue_desc,
            &IID_ID3D12CommandQueue,
            &context->dx_command_queue
        );
    
        if (FAILED(hresult)) {
            BS_WARN_HRESULT("CreateCommandQueue", hresult);
            return bs_convertHResult(hresult);
        }

       /**
        Fence
        */
        hresult = _bs_instance_->dx_device->lpVtbl->CreateFence(
            _bs_instance_->dx_device,
            0,
            D3D12_FENCE_FLAG_NONE,
            &IID_ID3D12Fence,
            (void**)&context->dx_fence
        );

        if (FAILED(hresult)) {
            BS_WARN_HRESULT("CreateFence", hresult);
            return bs_convertHResult(hresult);
        }

        context->dx_fence_value = 0;

        context->dx_fence_event = CreateEvent(
            NULL,
            FALSE, // auto-reset
            FALSE,
            NULL
        );

        if (!context->dx_fence_event) {
            hresult = HRESULT_FROM_WIN32(GetLastError());
            BS_WARN_HRESULT("CreateEvent", hresult);
            return bs_convertHResult(hresult);
        }

       /**
        Swapchain
        */
        const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
            .Width = props.resolution.x,
            .Height = props.resolution.y,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Stereo = FALSE,
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = props.images_count,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
        };

        IDXGISwapChain1* swapchain1;
        hresult = _bs_instance_->dxgi_factory->lpVtbl->CreateSwapChainForHwnd(
            _bs_instance_->dxgi_factory,
            context->dx_command_queue,
            context->hwnd, 
            &swapchain_desc,
            NULL,
            NULL, 
            &swapchain1
        );

        if (FAILED(hresult)) {
            BS_WARN_HRESULT("CreateSwapChainForHwnd", hresult);
            return bs_convertHResult(hresult);
        }

        hresult = swapchain1->lpVtbl->QueryInterface(
            swapchain1,
            &IID_IDXGISwapChain4,
            &context->dxgi_swapchain
        );

        if (FAILED(hresult)) {
            BS_WARN_HRESULT("QueryInterface", hresult);
            return bs_convertHResult(hresult);
        }

       /**
        Associate Window
        */
        hresult = _bs_instance_->dxgi_factory->lpVtbl->MakeWindowAssociation(
            _bs_instance_->dxgi_factory,
            context->hwnd, 
            DXGI_MWA_NO_ALT_ENTER
        );

        if (FAILED(hresult)) {
            BS_WARN_HRESULT("MakeWindowAssociation", hresult);
            return bs_convertHResult(hresult);
        }
    }
    
   /**
    Swapchain Images
    */
    if (context->swapchain_image == NULL) {
        context->swapchain_image = BS_OBJECT(bs_Image, -1, 0, props.images_count, BS_OBJECT_SWAPCHAIN_IMAGE_BIT, BS_OBJECT_IMAGE);
        context->dx_swapchain_images = bs_calloc(props.images_count, sizeof(bs_DxSwapchainImage));
    }

    *context->swapchain_image->image = (bs_Image){
        .head = context->swapchain_image->image->head,
        .flags = BS_IMAGE_SWAPCHAIN_IMAGE_BIT,
        .format = _bs_instance_->physical_device->surface_format.format,
        .dim = props.resolution,
    };

    for (int i = 0; i < props.images_count; i++) {
        hresult = context->dxgi_swapchain->lpVtbl->GetBuffer(
            context->dxgi_swapchain, 
            i, 
            &IID_ID3D12Resource, 
            &context->dx_swapchain_images[i].dx_image
        );
        if (FAILED(hresult)) {
            BS_WARN_HRESULT("GetBuffer", hresult);
            return bs_convertHResult(hresult);
        }
    }

    int len = snprintf(NULL, 0, "Local\\%s0_", context->title) + 36;
    char* shared_handle_name = bs_alloca(len + 1);
    LPCWSTR* wide_shared_handle_name = bs_alloca((len + 1) * sizeof(wchar_t));

    for (int i = 0; i < props.images_count; i++) {
        bs_ImageSwaps* swap = context->swapchain_image->image->_ + i;
        bs_DxSwapchainImage* dx_swap = context->dx_swapchain_images + i;

        D3D12_RESOURCE_DESC dx_image_desc;
        dx_swap->dx_image->lpVtbl->GetDesc(dx_swap->dx_image, &dx_image_desc);

        D3D12_HEAP_PROPERTIES dx_image_heap;
        D3D12_HEAP_FLAGS dx_image_heap_flags;
        hresult = dx_swap->dx_image->lpVtbl->GetHeapProperties(dx_swap->dx_image, &dx_image_heap, &dx_image_heap_flags);
        if (FAILED(hresult)) {
            BS_WARN_HRESULT("GetHeapProperties", hresult);
            return bs_convertHResult(hresult);
        }

        if (dx_image_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
            bs_warnF("Unexpected DXGI image dimensionality: (D3D12_RESOURCE_DIMENSION)%d\n", dx_image_desc.Dimension);
            return BS_RESULT_INVALID_TYPE;
        }

        if (dx_image_desc.DepthOrArraySize != 1) {
            bs_warnF("Unexpected DXGI image array count: %d\n", dx_image_desc.DepthOrArraySize);
            return BS_RESULT_INVALID_TYPE;
        }

        if (dx_image_desc.MipLevels != 1) {
            bs_warnF("Unexpected DXGI image mip level count: %d\n", dx_image_desc.MipLevels);
            return BS_RESULT_INVALID_TYPE;
        }

        if (dx_image_desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM) { // TODO
            bs_warnF("Unexpected DXGI image format: %d\n", dx_image_desc.Format);
            return BS_RESULT_INVALID_TYPE;
        }

        if (dx_image_desc.SampleDesc.Count != 1) {
            bs_warnF("Unexpected DXGI image sample count: %d\n", dx_image_desc.SampleDesc.Count);
            return BS_RESULT_INVALID_TYPE;
        }

        VkExternalMemoryImageCreateInfoKHR external_memory_image_ci = {
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHR,
        };

        // TODO("We shouldn't depend on this specific format")
        VkFormat ext_img_format = VK_FORMAT_R8G8B8A8_UNORM;

        assert(dx_image_desc.Width <= UINT32_MAX);

        const VkImageCreateInfo image_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &external_memory_image_ci,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ext_img_format,
            .extent = {
                .width = dx_image_desc.Width,
                .height = dx_image_desc.Height,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        vk_result = vkCreateImage(
            _bs_instance_->device,
            &image_ci,
            NULL,
            &swap->vk_image
        );
        if (vk_result != VK_SUCCESS) {
            _bs_warnF("Failed to create swapchain image for window \"%s\" (%d)", context->title, vk_result);
            return bs_convertVulkanResult(vk_result);
        }

       // bsi_nameHandle((bs_U64)context->swapchain_image->image->_[i].vk_image, VK_OBJECT_TYPE_IMAGE, context->title);

       /**
        Shared Handle
        */
        bs_GUID guid = bs_guid();
        char guid_string[37];
        bs_guidToString(&guid, guid_string);

        sprintf(shared_handle_name, "Local\\%s%d_%s", context->title, i, guid_string);
        bs_widen(shared_handle_name, wide_shared_handle_name, len + 1);

        //TODO("I am pretty sure DX Swapchain is not considered SHARED, but it seems to work anyway.")
        hresult = _bs_instance_->dx_device->lpVtbl->CreateSharedHandle(
            _bs_instance_->dx_device,
            dx_swap->dx_image,
            NULL,
            GENERIC_ALL,
            wide_shared_handle_name,
            &dx_swap->dx_shared_handle
        );
        if (FAILED(hresult)) {
            BS_WARN_HRESULT("CreateSharedHandle", hresult);
            return bs_convertHResult(hresult);
        }

        VkMemoryWin32HandlePropertiesKHR win32_mem_props = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR,
            .memoryTypeBits = 0xcdcdcdcd
        };
        vk_result = _bs_procs_.vkGetMemoryWin32HandlePropertiesKHR(
            _bs_instance_->device, 
            VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, 
            dx_swap->dx_shared_handle,
            &win32_mem_props
        );
        if (vk_result != VK_SUCCESS) {
            BS_WARN_HRESULT("vkGetMemoryWin32HandlePropertiesKHR", hresult);
            return bs_convertVulkanResult(vk_result);
        }

        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(_bs_instance_->device, swap->vk_image, &mem_req);

        // TODO("Workaround for AMD driver.");
        if (win32_mem_props.memoryTypeBits == 0xcdcdcdcd)
            win32_mem_props.memoryTypeBits = mem_req.memoryTypeBits;
        else
            win32_mem_props.memoryTypeBits &= mem_req.memoryTypeBits; // assumably must satisfy both

        VkPhysicalDeviceMemoryProperties mem_props;
        vkGetPhysicalDeviceMemoryProperties(_bs_instance_->physical_device->vk_device, &mem_props);

        int mem_type_index = -1;
        for (uint32_t im = 0; im < mem_props.memoryTypeCount; im++) {
            const uint32_t current_bit = 0x1 << im;
            if (win32_mem_props.memoryTypeBits & current_bit) {
                if (mem_props.memoryTypes[im].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    mem_type_index = im;
                    break;
                }
            }
        }

        if (mem_type_index < 0) {
            bs_warnF("Failed to query device local import memory");
            return BS_RESULT_FAILED_TO_QUERY;
        }

        // DX12 Resource has to be dedicated per Vk spec
        const VkMemoryDedicatedAllocateInfoKHR memory_dedicated_alloc_i = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
            .image = swap->vk_image,
        };

        const VkImportMemoryWin32HandleInfoKHR import_memory_win32_handle = {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
            .pNext = &memory_dedicated_alloc_i,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHR,
            .handle = dx_swap->dx_shared_handle,
        };

        const VkMemoryAllocateInfo mem_alloc_i = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &import_memory_win32_handle,
            .allocationSize = mem_req.size,
            .memoryTypeIndex = mem_type_index,
        };

        vk_result = vkAllocateMemory(
            _bs_instance_->device, 
            &mem_alloc_i,
            NULL, 
            &swap->vk_memory
        ); 
        if (vk_result != VK_SUCCESS) {
            BS_WARN_VULKAN_ERROR("vkAllocateMemory", vk_result, "");
            return bs_convertVulkanResult(vk_result);
        }

        vk_result = vkBindImageMemory(
            _bs_instance_->device, 
            swap->vk_image,
            swap->vk_memory,
            0
        ); 
        if (vk_result != VK_SUCCESS) {
            BS_WARN_VULKAN_ERROR("vkBindImageMemory", vk_result, "");
            return bs_convertVulkanResult(vk_result);
        }

        VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swap->vk_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = (VkFormat)_bs_instance_->physical_device->surface_format.format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };

        vk_result = vkCreateImageView(
            _bs_instance_->device, 
            &image_view_ci, 
            NULL, 
            &swap->vk_image_view
        );
        if (vk_result != VK_SUCCESS) {
            _bs_warnF("Failed to create swapchain image view for window \"%s\" (%d)", context->title, vk_result);
            return bs_convertVulkanResult(vk_result);
        }

        //bsi_nameHandle(context->swapchain_image->image->_[i].vk_image_view, VK_OBJECT_TYPE_IMAGE_VIEW, context->title);
    }

    //context->win32.waitable_object = context->dxgi_swapchain->lpVtbl->GetFrameLatencyWaitableObject(context->dxgi_swapchain);

   // hresult = context->dxgi_swapchain->lpVtbl->SetMaximumFrameLatency(context->dxgi_swapchain, 1);
   // if (FAILED(hresult)) {
   //     BS_WARN_HRESULT("SetMaximumFrameLatency", hresult);
   // }

    return BS_RESULT_OK;
}

#endif



  /*==============================================================================
   * Vulkan Swapchain
   *============================================================================*/

BSAPI bs_Result _bs_swapchain(bs_Context* context) {
    bs_Context* last_context = _bs_scope_.context;
    _bs_scope_.context = context;

#ifdef _WIN32
   /**
    DXGI Swapchain
    */
    if (!_bs_args_.force_vulkan_swapchain) {
        if (context->dxgi_swapchain) {
            bs_Result dxgi_result = _bs_dxgiSwapchain(context);
            _bs_scope_.context = last_context;
            return dxgi_result;
        }
        else if (!context->swapchain) {
            if (_bs_dxgiSwapchain(context) == BS_RESULT_OK) {
                _bs_scope_.context = last_context;
                goto end;
            }

            bs_logF("Failed to create DXGI swapchain, defaulting to Vulkan...");
        }
    }
#endif

   /**
    Vulkan Swapchain
    */
    VkResult result;

    const bool same_family = true; // TODO: this shouldn't always be true

    bs_SwapchainProperties props = _bs_swapchainProperties();

    printf("swapchain %d, %d\n", props.capabilities.minImageExtent.width, props.capabilities.maxImageExtent.width);

    bs_Image image = {
        .head = {
            .type = BS_OBJECT_IMAGE
        },
        .flags = BS_IMAGE_SWAPCHAIN_IMAGE_BIT,
        .format = _bs_instance_->physical_device->surface_format.format,
        .dim = props.resolution,
    };

    //context->frames_in_flight = _bs_instance_->max_frames_in_flight;

    _bs_instance_->max_swapchain_images_count = BS_MAX(_bs_instance_->max_swapchain_images_count, props.images_count);

    VkSwapchainCreateInfoKHR swapchain_ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = context->surface,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .minImageCount = props.images_count,
        .imageExtent = { props.resolution.x, props.resolution.y },
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = same_family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
    //    .queueFamilyIndexCount = same_family ? 0 : 2, // TODO: why is this 2
    //    .pQueueFamilyIndices = same_family ? NULL : NULL,
        .oldSwapchain = context->swapchain ? context->swapchain : 0,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .clipped = VK_TRUE,
        .preTransform = props.capabilities.currentTransform,
        .presentMode = (VkPresentModeKHR)_bs_instance_->physical_device->present_mode,
        .imageFormat = (VkFormat)_bs_instance_->physical_device->surface_format.format,
        .imageColorSpace = (VkColorSpaceKHR)_bs_instance_->physical_device->surface_format.color_space,
    };

    VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(_bs_instance_->device, &swapchain_ci, NULL, &new_swapchain);
    if (result != VK_SUCCESS) {
        _bs_warnF("Failed to create swapchain for window \"%s\"", context->title);
        _bs_scope_.context = last_context;
        return bs_convertVulkanResult(result);
    }

    _bs_destroySwapchain();
    context->swapchain = new_swapchain;

    bsi_nameHandle((bs_U64)context->swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, context->title);

   /**
    Swapchain images
    */
    VkImage images[3];
    vkGetSwapchainImagesKHR(_bs_instance_->device, context->swapchain, &props.images_count, images);
   // _bs_infoF("Swapchain\n  Format: %d\n  Mode: %d\n  Images: %d", swapchain_ci.imageFormat, swapchain_ci.presentMode, images_count);

    if (context->swapchain_image == NULL)
        context->swapchain_image = BS_OBJECT(bs_Image, -1, 0, props.images_count, BS_OBJECT_SWAPCHAIN_IMAGE_BIT, BS_OBJECT_IMAGE);

    bs_Header head = context->swapchain_image->image->head;
    memcpy(context->swapchain_image->image, &image, sizeof(image));
    context->swapchain_image->image->head = head;

   /**
    Swapchain image views
    */
    for (int i = 0; i < props.images_count; i++) {
        context->swapchain_image->image->_[i].vk_image = images[i];
        bsi_nameHandle(images[i], VK_OBJECT_TYPE_IMAGE, context->title);

        VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = context->swapchain_image->image->_[i].vk_image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = (VkFormat)_bs_instance_->physical_device->surface_format.format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1,
        };

        result = vkCreateImageView(_bs_instance_->device, &image_view_ci, NULL, &context->swapchain_image->image->_[i].vk_image_view);
        if (result != VK_SUCCESS) {
            _bs_warnF("Failed to create swapchain image view for window \"%s\"", context->title);
            _bs_scope_.context = last_context;
            return bs_convertVulkanResult(result);
        }

        bsi_nameHandle(context->swapchain_image->image->_[i].vk_image_view, VK_OBJECT_TYPE_IMAGE_VIEW, context->title);
    }

    end:
   /**
    Swapchain semaphores
    */
    VkSemaphoreCreateInfo semaphore_ci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    for (int i = 0; i < context->head.swaps_count; i++) {
        if (!context->_[i].semaphore) {
            result = vkCreateSemaphore(_bs_instance_->device, &semaphore_ci, NULL, &context->_[i].semaphore);
            if (result != VK_SUCCESS) {
                _bs_warnF("Failed to create swapchain semaphore for window \"%s\"", context->title);
                _bs_scope_.context = last_context;
                return bs_convertVulkanResult(result);
            }
        }
    }

    _bs_scope_.context = last_context;
    return BS_RESULT_OK;
}



  /*==============================================================================
   * Window
   *============================================================================*/

BSAPI void _bs_titleWindowN(bs_Context* context, char* name, int name_length) {
    context->title = name; // todo
}

BSAPI void _bs_setCursor(bs_CursorIcon icon) {
	_bs_warnF("_bs_setCursor has not been implemented yet");
	/*
	if (_bs_scope_.context->cursor_icons[icon].handle == NULL)
		_bs_scope_.context->cursor_icons[icon].handle = LoadCursor(NULL, _bs_wnd.cursor_icons[icon].id);

	if (_bs_wnd.cursor_icon == icon)
		return;

	_bs_wnd.cursor_icon = icon;
	*/
}

BSAPI void _bs_resizeWindow(bs_Context* context, bs_U32 width, bs_U32 height) {
#ifdef _WIN32
    RECT rect;
    GetWindowRect(context->hwnd, &rect);

    SetWindowPos(
        context->hwnd,
        NULL,
        rect.left,
        rect.top,
        width,
        height,
        SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER
    );

   _bs_resizeContext(context, width, height);
#else
    _bs_warnF("_bs_resizeWindow has not been implemented for this OS yet");
#endif
}

BSAPI void _bs_maximizeWindow(bs_Context* context) {
#ifdef _WIN32
	ShowWindow(context->hwnd, SW_SHOWMAXIMIZED);
#else
	_bs_warnF("_bs_maximizeWindow has not been implemented for this OS yet");
#endif
}

BSAPI void _bs_minimizeWindow(bs_Context* context) {
#ifdef _WIN32
	ShowWindow(context->hwnd, SW_SHOWMINIMIZED);
#else
	_bs_warnF("_bs_minimizeWindow has not been implemented for this OS yet");
#endif
}

BSAPI void _bs_hideWindow(bs_Context* context) {
#ifdef _WIN32
    if (context->hidden)
        return;

    context->hidden = true;
    ShowWindow(context->hwnd, SW_HIDE);
    bs_logF("Hiding window \"%s\"", context->title);
#else
    _bs_warnF("_bs_hideWindow has not been implemented for this OS yet");
#endif
}

BSAPI void _bs_showWindow(bs_Context* context) {
#ifdef _WIN32
    if (!context->hidden)
        return;

    context->hidden = false;
    if (context->window_type == BS_WINDOW_POPUP)
        ShowWindow(context->hwnd, SW_SHOWNOACTIVATE);
    else
        ShowWindow(context->hwnd, SW_SHOW);

    bs_logF("Showing window \"%s\"", context->title);
#else
    _bs_warnF("_bs_showWindow has not been implemented for this OS yet");
#endif
}

BSAPI void _bs_exit() {
    _bs_instance_->alive = false;
}

BSAPI void _bs_pause() {
    _bs_instance_->paused = !_bs_instance_->paused;
}

BSAPI void _val_bs_advance() {
    BS_VALIDATE(_bs_instance_->paused == true,,);
    _bs_advance();
}

BSAPI void _bs_advance() {
    _bs_instance_->advance = true;
}

BSAPI double _bs_deltaTime() {
	return _bs_instance_->delta_time;
}

BSAPI double _bs_elapsedTime() {
    return _bs_instance_->time;
}

BSAPI bs_ivec2 _val_bs_resolution(bs_Context* context) {
    BS_VALIDATE(context->swapchain_image != NULL, BS_IV2(0, 0),);

    return _bs_resolution(context);
}

BSAPI bs_ivec2 _bs_resolution(bs_Context* context) {
    return context->swapchain_image->image->dim;
}

BSAPI bs_vec2 _bs_windowCursorPosition(bs_Context* context) {
    bs_vec2 p = context->cursor;
    p.y = context->swapchain_image->image->dim.y - p.y;

    return p;

    bs_vec2 dim = { context->swapchain_image->image->dim.x, context->swapchain_image->image->dim.y };
    bs_vec2 pos;

    bs_v2Div(&context->cursor, &dim, &pos);
	return BS_V2(pos.x, 1.0 - pos.y);
}

BSAPI bs_ivec2 _bs_windowPosition(bs_Context* context) {
#ifdef _WIN32
	RECT rectangle = { 0 };
	GetWindowRect(context->hwnd, &rectangle);

	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	return (bs_ivec2) {
		rectangle.left,
		rectangle.bottom
	};

#elif defined(__linux__)
    return (bs_ivec2) {
        .x = 0,
        .y = 0
    };

#elif defined(__APPLE__)
    _bs_warnF("_bs_windowPosition has not been implemented for macOS yet");
    return (bs_ivec2) { 0, 0 };
#else
    return (bs_ivec2) { 0, 0 };
#endif
}



  /*==============================================================================
   * Inputs
   *============================================================================*/

// temp
#ifdef _WIN32
const bool separate_message_thread = false;


#else
const bool separate_message_thread = false;
#endif

#ifdef _WIN32
static inline void bs_setBit(long A[], unsigned int k) {
    InterlockedOr(&A[k / 32U], 1L << (k % 32U));
}

static inline void bs_clearBit(long A[], unsigned int k) {
    InterlockedAnd(&A[k / 32U], ~(1L << (k % 32U)));
}

static inline int bs_getBit(long A[], unsigned int k) {
    long value = InterlockedCompareExchange(&A[k / 32U], 0, 0);
    return (value & (1L << (k % 32U))) != 0;
}

static inline int bs_testBit(long A[], unsigned int k) {
    long value = InterlockedCompareExchange(&A[k / 32U], 0, 0);
    return (value & (1L << (k % 32U))) != 0;
}

#define bs_atomicExchange(object, desired) InterlockedExchange(object, desired)

#else
#include <stdatomic.h>
static inline void bs_setBit(_Atomic bs_U32 A[], unsigned int k) {
    atomic_fetch_or(&A[k / 32U], 1L << (k % 32U));
}

static inline void bs_clearBit(_Atomic bs_U32 A[], unsigned int k) {
    atomic_fetch_and(&A[k / 32U], ~(1L << (k % 32U)));
}

static inline int bs_getBit(bs_U32 A[], unsigned int k) {
    bs_U32 value = A[k / 32U];
    return (value & (1L << (k % 32U))) != 0;
}

static inline int bs_testBit(bs_U32 A[], unsigned int k) {
    bs_U32 value = A[k / 32U];
    return (value & (1L << (k % 32U))) != 0;
}

#define bs_atomicExchange(object, desired) atomic_exchange(object, desired)

#endif

/*
static inline void bs_setBit(bs_I16 A[], unsigned int k) {
    InterlockedOr16(&A[k / 16U], (bs_I16)(1U << (k % 16U)));
}

static inline void bs_clearBit(bs_I16 A[], unsigned int k) {
    InterlockedAnd16(&A[k / 16U], (bs_I16)~(1U << (k % 16U)));
}

static inline int bs_getBit(bs_I16 A[], unsigned int k) {
    bs_I16 value = InterlockedCompareExchange16(&A[k / 16U], 0, 0);

    return (value & (bs_I16)(1U << (k % 16U))) != 0;
}

static inline int bs_testBit(bs_I16 A[], unsigned int k) {
    bs_I16 value = InterlockedCompareExchange16(&A[k / 16U], 0, 0);

    return (value & (bs_I16)(1U << (k % 16U))) != 0;
}
*/


BSAPI bs_vec2 _bs_screenCursorPosition() {
	return _bs_instance_->screen_cursor;
}

BSAPI bool _bs_contextKeyHeld(bs_Context* context, bs_U32 code) {
    return false;
}

BSAPI bool _bs_contextInputDown(bs_Context* context, bs_U32 code) {
    return bs_getBit(context->io.inputs_down, code);
}

BSAPI bool _bs_contextInputDownOnce(bs_Context* context, bs_U32 code) {
    return bs_getBit(context->io.inputs_down_once, code);
}

BSAPI bool _bs_contextInputUpOnce(bs_Context* context, bs_U32 code) {
    return bs_getBit(context->io.inputs_up_once, code);
}

BSAPI bool _bs_contextCharDown(bs_Context* context, unsigned char code) {
    return false;
}

BSAPI bool _bs_contextCharDownOnce(bs_Context* context, unsigned char code) {
    return false;
}

BSAPI bool _bs_contextCharUpOnce(bs_Context* context, unsigned char code) {
    return false;
}

BSAPI bool _bs_inputHeld(bs_U32 code) { return _bs_contextKeyHeld(_bs_scope_.context, code); }
BSAPI bool _bs_inputDown(bs_U32 code) { return _bs_contextInputDown(_bs_scope_.context, code); }
BSAPI bool _bs_inputDownOnce(bs_U32 code) { return _bs_contextInputDownOnce(_bs_scope_.context, code); }
BSAPI bool _bs_inputUpOnce(bs_U32 code) { return _bs_contextInputUpOnce(_bs_scope_.context, code); }

BSAPI bool _bs_charDown(unsigned char code) { return _bs_contextCharDown(_bs_scope_.context, code); }
BSAPI bool _bs_charDownOnce(unsigned char code) { return _bs_contextCharDownOnce(_bs_scope_.context, code); }
BSAPI bool _bs_charUpOnce(unsigned char code) { return _bs_contextCharUpOnce(_bs_scope_.context, code); }

BSAPI int _bs_scroll() {
	return _bs_scope_.context->io.scroll;
}

BSAPI bool _bs_inFixedTick() {
    return _bs_instance_->in_fixed;
}

BSAPI void _bs_setTargetFramerate(int fps) {
    _bs_instance_->target_frame_time = 1.0 / (double)fps;
}

void _bs_tickContext(bs_Context* context) {
   // if (bs_getBit(context->io.keys, BS_KEY_ALT) && bs_getBit(context->io.keys, BS_KEY_F4))
   //     _bs_exit();

    #ifdef _WIN32
    context->active = context->hwnd == GetForegroundWindow();

    POINT p = { _bs_instance_->screen_cursor.x, _bs_instance_->screen_cursor.y };
    if (ScreenToClient(context->hwnd, &p))
        context->cursor = BS_V2(p.x, p.y);
    #endif

    if (separate_message_thread) {
        for (int i = 0; i < BS_KEY_BYTES_COUNT; i++)
            context->io.inputs_up_once[i] = bs_atomicExchange(&context->io.input_up_events[i], 0);

        for (int i = 0; i < BS_KEY_BYTES_COUNT; i++) {
            context->io.inputs_down_last[i] = context->io.inputs_down[i];
            context->io.inputs_down_once[i] = bs_atomicExchange(&context->io.input_down_events[i], 0);
        }

    } else {
        for (int i = 0; i < BS_KEY_BYTES_COUNT; i++) {
            context->io.inputs_up_once[i] = context->io.input_up_events[i];
            context->io.input_up_events[i] = 0;
        }

        for (int i = 0; i < BS_KEY_BYTES_COUNT; i++) {
            context->io.inputs_down_last[i] = context->io.inputs_down[i];
            context->io.inputs_down_once[i] = context->io.input_down_events[i];
            context->io.input_down_events[i] = 0;
        }
    }

    for (int i = 0; i < BS_KEY_BYTES_COUNT; i++)
        context->io.inputs_down[i] |= context->io.inputs_down_once[i];

    if (context->listener.tick) {
        bs_Context* last_context = _bs_scope_.context;
        _bs_scope_.context = context;
        context->listener.tick(context, NULL);
        _bs_scope_.context = last_context;
    }

    for (int i = 0; i < BS_KEY_BYTES_COUNT; i++)
        context->io.inputs_down[i] &= ~context->io.inputs_up_once[i];

    _bs_instance_->time_old = _bs_instance_->time;
}

static bs_List* _bs_getAllContexts() {
    static _Thread_local bs_List contexts = { .unit_size = sizeof(bs_Context*), .increment = 4 };
    contexts.count = 0;

    bs_List* object_sources = bs_objectSources();

    for (int i = 0; i < object_sources->count; i++) {
        bs_ObjectSource* source = bs_fetchUnit(object_sources, i);
        if (source->type != BS_OBJECT_CONTEXT)
            continue;

        for (int j = 0; j < source->ids_count; j++) {
            if (!source->ids[j].object)
                continue;

            bs_pushBack(&contexts, &source->ids[j].object->context);
        }
    }

    for (int i = 0; i < _bs_instance_->popup_windows.count; i++) {
        bs_Object* obj = *(bs_Object**)bs_fetchUnit(&_bs_instance_->popup_windows, i);
        bs_pushBack(&contexts, &obj->context);
    }

    return &contexts;
}

// not the best
void _bs_closeAllPopupWindows() {
    bs_List* contexts = _bs_getAllContexts();
    for (int i = 0; i < contexts->count; i++) {
        bs_Context* ctx = *(bs_Context**)_bs_fetchUnit(contexts, i);
        if (ctx->window_type == BS_WINDOW_POPUP) {
            bs_closePopupWindow(ctx);
        }
    }
}

static void _bs_renderTick(bs_Callback fixed_tick) {
    //  _bs_checkTimer(&_bs_instance_->timer);
    double frame_start = _bs_instance_->timer.seconds;

    if (fixed_tick) {
        _bs_instance_->in_fixed = true;
        /*
        for (int i = 0; _bs_instance_->advance || (_bs_instance_->elapsed_time < _bs_instance_->time && i < 200 && !_bs_instance_->paused); i++) {
            _bs_instance_->new_time_index = !_bs_instance_->new_time_index;
            _bs_instance_->last_fixed_update_times[_bs_instance_->new_time_index] = _bs_instance_->fixed_time;

            _bs_instance_->delta_time = _bs_instance_->fixed_time;
            fixed_tick();
            _bs_instance_->elapsed_time += _bs_instance_->fixed_time;
            _bs_instance_->advance = false;
        }
        */
    }

    _bs_checkTimer(&_bs_instance_->timer);
    _bs_instance_->time = _bs_instance_->timer.seconds;

    float newer_time = _bs_instance_->last_fixed_update_times[_bs_instance_->new_time_index];
    float older_time = _bs_instance_->last_fixed_update_times[!_bs_instance_->new_time_index];

    if (newer_time != older_time)
        _bs_instance_->fixed_interpolation = (_bs_instance_->time - newer_time) / (newer_time - older_time);
    else
        _bs_instance_->fixed_interpolation = 1.0f;

    _bs_instance_->delta_time = _bs_instance_->time - _bs_instance_->time_old;
    _bs_instance_->in_fixed = false;

    #ifdef _WIN32
    POINT p;
    if (GetCursorPos(&p)) {
        _bs_instance_->screen_cursor = BS_V2(p.x, p.y);
    }
    #endif

   /**
    Tick all contexts
    */
    bs_List* contexts = _bs_getAllContexts();
    for (int i = 0; i < contexts->count; i++) {
        bs_Context* ctx = *(bs_Context**)_bs_fetchUnit(contexts, i);

        _bs_scope_.context = ctx;
        //InvalidateRect(ctx->hwnd, NULL, FALSE);
        //UpdateWindow(ctx->hwnd);
       // if (ctx->swapchain_ok)
       //     _bs_tickContext(ctx);
        _bs_scope_.context = NULL;
    }

    _bs_checkTimer(&_bs_instance_->timer);

    #ifdef _WIN32
    //while ((_bs_instance_->timer.seconds - frame_start) < _bs_instance_->target_frame_time) {
    //    Sleep(0);
    //    _bs_checkTimer(&_bs_instance_->timer);
    //}
    #endif
}

static void _bs_startRenderTick(bs_Callback fixed_tick) {
    while (_bs_instance_->alive) {
        _bs_renderTick(fixed_tick);
    }
}

#ifdef _WIN32
static void _bs_handleMessageAtomic(bs_List* contexts, bs_Context* context, MSG msg) {
    switch (msg.message) {
    case WM_QUIT: PostQuitMessage(0); _bs_instance_->alive = false; return;

    case WM_LBUTTONDOWN:
        SetCapture(context->hwnd);
        bs_setBit(context->io.input_down_events, BS_LEFT_MOUSE_BUTTON);

        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_LEFT_MOUSE_BUTTON, .state = BS_INPUT_PRESSED, });
        break;
    case WM_LBUTTONUP:
        ReleaseCapture();
        bs_setBit(context->io.input_up_events, BS_LEFT_MOUSE_BUTTON);

        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_LEFT_MOUSE_BUTTON, .state = BS_INPUT_RELEASED, });
        break;

    case WM_RBUTTONDOWN:
        SetCapture(context->hwnd);
        bs_setBit(context->io.input_down_events, BS_RIGHT_MOUSE_BUTTON);

        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_RIGHT_MOUSE_BUTTON, .state = BS_INPUT_PRESSED, });
        break;
    case WM_RBUTTONUP:
        ReleaseCapture();
        bs_setBit(context->io.input_up_events, BS_RIGHT_MOUSE_BUTTON);

        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_RIGHT_MOUSE_BUTTON, .state = BS_INPUT_RELEASED, });
        break;

    case WM_MBUTTONDOWN:
        SetCapture(context->hwnd);
        bs_setBit(context->io.input_down_events, BS_MIDDLE_MOUSE_BUTTON);

        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_MIDDLE_MOUSE_BUTTON, .state = BS_INPUT_PRESSED, });
        break;
    case WM_MBUTTONUP:
        ReleaseCapture();
        bs_setBit(context->io.input_up_events, BS_MIDDLE_MOUSE_BUTTON);

        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_MIDDLE_MOUSE_BUTTON, .state = BS_INPUT_RELEASED, });
        break;

    case WM_MOUSEWHEEL: {
        context->io.scroll = (SHORT)HIWORD(msg.wParam) / 120.0;
    } break;
    case WM_CHAR: {
     //   if (msg.wParam < BS_KEYS_COUNT)
     //       bs_setBit(context->io.chars, (bs_U32)msg.wParam);
    } break;
    case WM_KEYDOWN: {
        if (msg.wParam < BS_KEYS_COUNT)
            bs_setBit(context->io.input_down_events, (bs_U32)msg.wParam);
    } break;
    case WM_KEYUP: {
        if (msg.wParam < BS_KEYS_COUNT)
            bs_setBit(context->io.input_up_events, (bs_U32)msg.wParam);
       // if (msg.wParam < 256)
       //     bs_clearBit(context->io.key_events, (bs_U32)msg.wParam);
    } break;
    case WM_SYSKEYDOWN: {
        //if (msg.wParam < 256)
        //    bs_setBit(context->io.key_events, (bs_U32)msg.wParam);
    } break;
    case WM_SYSKEYUP: {
        // if (msg.wParam < 256)
      //     bs_clearBit(context->io.key_events, (bs_U32)msg.wParam);
    } break;
    case WM_NCLBUTTONDOWN:
        bs_setBit(context->io.input_down_events, BS_LEFT_MOUSE_BUTTON_NON_CLIENT);
        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_LEFT_MOUSE_BUTTON, .state = BS_INPUT_PRESSED, .non_client_area = true });
        break;
    case WM_NCLBUTTONUP:
        bs_setBit(context->io.input_up_events, BS_LEFT_MOUSE_BUTTON_NON_CLIENT);
        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_LEFT_MOUSE_BUTTON, .state = BS_INPUT_RELEASED, .non_client_area = true });
        break;
    case WM_NCRBUTTONDOWN:
        bs_setBit(context->io.input_down_events, BS_RIGHT_MOUSE_BUTTON_NON_CLIENT);
        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_RIGHT_MOUSE_BUTTON, .state = BS_INPUT_PRESSED, .non_client_area = true });
        break;
    case WM_NCRBUTTONUP:
        bs_setBit(context->io.input_up_events, BS_RIGHT_MOUSE_BUTTON_NON_CLIENT);
        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_RIGHT_MOUSE_BUTTON, .state = BS_INPUT_RELEASED, .non_client_area = true });
        break;
    case WM_NCMBUTTONDOWN:
        bs_setBit(context->io.input_down_events, BS_MIDDLE_MOUSE_BUTTON_NON_CLIENT);
        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_MIDDLE_MOUSE_BUTTON, .state = BS_INPUT_PRESSED, .non_client_area = true });
        break;
    case WM_NCMBUTTONUP:
        bs_setBit(context->io.input_up_events, BS_MIDDLE_MOUSE_BUTTON_NON_CLIENT);
        if (context->listener.input)
            context->listener.input(context, (bs_ContextInputParams) { .code = BS_MIDDLE_MOUSE_BUTTON, .state = BS_INPUT_RELEASED, .non_client_area = true });
        break;
    //case WM_NCLBUTTONDBLCLK:
    //case WM_NCRBUTTONDBLCLK:
    //case WM_NCMBUTTONDBLCLK:
    case WM_MOUSEMOVE:
        TRACKMOUSEEVENT track_mouse_event = {
            .cbSize = sizeof(TRACKMOUSEEVENT),
            .dwFlags = TME_LEAVE,
            .hwndTrack = context->hwnd,
            .dwHoverTime = HOVER_DEFAULT,
        };

        TrackMouseEvent(&track_mouse_event);

        if (!context->hovering) {
            if (context->listener.enter)
                context->listener.enter(context, NULL);

            context->hovering = true;
        }

        break;
    case WM_MOUSELEAVE:
        if (context->listener.leave)
            context->listener.leave(context, NULL);

        context->hovering = false;

        break;
    }
}
#endif

BSAPI void _bs_tick(bs_Callback fixed_tick) {
    _bs_instance_->alive = true;
    _bs_instance_->timer = _bs_timer();

    if (separate_message_thread)
        bs_createThread((bs_ThreadFunction)_bs_startRenderTick, fixed_tick);

    while (_bs_instance_->alive) {
        bs_List* contexts = _bs_getAllContexts();

       /**
        Message loop
        */
#ifdef _WIN32
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            _bs_scope_.context = NULL;

            bs_Context* context = NULL;
            for (int i = 0; i < contexts->count; i++) {
                bs_Context* ctx = *(bs_Context**)bs_fetchUnit(contexts, i);

                if (ctx->hwnd == msg.hwnd) {
                    context = ctx;
                    break;
                }
            }

            if (context)
                _bs_handleMessageAtomic(contexts, context, msg);

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#endif

        if (!separate_message_thread) {
            _bs_renderTick(fixed_tick);
        }

#ifdef __linux__
        for (int i = 0; i < contexts.count; i++) {
            bs_Context* ctx = *(bs_Context**)bs_fetchUnit(&contexts, i);
            if (strcmp(ctx->title, "Basilisk") == 0) // very temp
            {
                wl_display_dispatch(ctx->display);
            }
        }
#endif

        /*
        while ((_bs_instance_->timer.seconds - frame_start) < _bs_instance_->target_frame_time) {
            Sleep(0);
            _bs_checkTimer(&_bs_instance_->timer);
        }
        */
    }
}

static int
win32_dpi_scale(
    int value,
    UINT dpi
) {
    return (int)((float)value * dpi / 96);
}

static void
win32_center_rect_in_rect(
    RECT* to_center,
    const RECT* outer_rect
) {
    int to_width = to_center->right - to_center->left;
    int to_height = to_center->bottom - to_center->top;
    int outer_width = outer_rect->right - outer_rect->left;
    int outer_height = outer_rect->bottom - outer_rect->top;

    int padding_x = (outer_width - to_width) / 2;
    int padding_y = (outer_height - to_height) / 2;

    to_center->left = outer_rect->left + padding_x;
    to_center->top = outer_rect->top + padding_y;
    to_center->right = to_center->left + to_width;
    to_center->bottom = to_center->top + to_height;
}

// Set this to 0 to remove the fake shadow painting
#define WIN32_FAKE_SHADOW_HEIGHT 1
// The offset of the 2 rectangles of the maximized window button
#define WIN32_MAXIMIZED_RECTANGLE_OFFSET 2
typedef struct {
    RECT close;
    RECT maximize;
    RECT minimize;
} CustomTitleBarButtonRects;

typedef enum {
    CustomTitleBarHoveredButton_None,
    CustomTitleBarHoveredButton_Minimize,
    CustomTitleBarHoveredButton_Maximize,
    CustomTitleBarHoveredButton_Close,
} CustomTitleBarHoveredButton;

static CustomTitleBarButtonRects
win32_get_title_bar_button_rects(
    HWND handle,
    const RECT* title_bar_rect
) {
    UINT dpi = GetDpiForWindow(handle);
    CustomTitleBarButtonRects button_rects;
    // Sadly SM_CXSIZE does not result in the right size buttons for Win10
    int button_width = win32_dpi_scale(47, dpi);
    button_rects.close = *title_bar_rect;
    //button_rects.close.top += WIN32_FAKE_SHADOW_HEIGHT;

    button_rects.close.left = button_rects.close.right - button_width;
    button_rects.maximize = button_rects.close;
    button_rects.maximize.left -= button_width;
    button_rects.maximize.right -= button_width;
    button_rects.minimize = button_rects.maximize;
    button_rects.minimize.left -= button_width;
    button_rects.minimize.right -= button_width;
    return button_rects;
}

static bool
win32_window_is_maximized(
    HWND handle
) {
    WINDOWPLACEMENT placement = { 0 };
    placement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(handle, &placement)) {
        return placement.showCmd == SW_SHOWMAXIMIZED;
    }
    return false;
}

static HFONT _bs_loadFont(bs_U32 dpi, LPCWSTR name) {
    return CreateFontW(
        -win32_dpi_scale(10, dpi),  // height
        0,                          // width
        0,                          // escapement
        0,                          // orientation
        FW_NORMAL,                  // weight
        FALSE,                      // italic
        FALSE,                      // underline
        FALSE,                      // strikeout
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        name
    );
}

static void _bs_updateWindowDPI(bs_Context* context) {
    #ifdef _WIN32
    UINT dpi = GetDpiForWindow(context->hwnd);

    if (dpi == context->dpi)
        return;

    context->dpi = dpi;

    context->border_size.x = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    context->border_size.y = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi);

    if (context->win32.icons_font)
        DeleteObject(context->win32.icons_font);

    context->win32.icons_font = _bs_loadFont(dpi, L"Segoe Fluent Icons"); // Windows 11

    if (!context->win32.icons_font)
        context->win32.icons_font = _bs_loadFont(dpi, L"Segoe MDL2 Assets"); // Windows 10

    if (!context->win32.icons_font)
        context->win32.icons_font = _bs_loadFont(dpi, L"Segoe UI Symbol"); // Windows 8/8.1

    #endif
}

#ifdef _WIN32
static LRESULT _bs_hitTestResize(bs_Context* context, bs_ivec2 pt, LRESULT fallback) {
    RECT rc;
    GetWindowRect(context->hwnd, &rc);

    const bool left = pt.x < rc.left + context->border_size.x;
    const bool right = pt.x >= rc.right - context->border_size.x;
    const bool top = pt.y < rc.top + context->border_size.y;
    const bool bottom = pt.y >= rc.bottom - context->border_size.y;

    if (top && left) return HTTOPLEFT;
    if (top && right) return HTTOPRIGHT;
    if (bottom && left) return HTBOTTOMLEFT;
    if (bottom && right) return HTBOTTOMRIGHT;
    if (top) return HTTOP;
    if (left) return HTLEFT;
    if (bottom) return HTBOTTOM;
    if (right) return HTRIGHT;

    return fallback;
}

static void _bs_drawIcon(bs_Context* context, HDC hdc, RECT* button_rect, WCHAR glyph) {
    COLORREF title_bar_item_color = RGB(234, 234, 234);
    HFONT old_font = (HFONT)SelectObject(hdc, context->win32.icons_font);

    SetTextColor(hdc, title_bar_item_color);
    SetBkMode(hdc, TRANSPARENT);

    DrawTextW(
        hdc,
        &glyph,
        1,
        button_rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(hdc, old_font);
}

LRESULT CALLBACK _bs_windowProcedure(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    bs_List* contexts = _bs_getAllContexts();
    bs_Context* context = NULL;

    for (int i = 0; i < contexts->count; i++) {
        bs_Context* ctx = *(bs_Context**)_bs_fetchUnit(contexts, i);

        if (ctx->hwnd == hwnd) {
            context = ctx;
            break;
        }
    }

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE: {
        uint32_t width = l_param & 0xffff;
        uint32_t height = (l_param >> 16) & 0xffff;

        if (_bs_instance_->physical_device && context && context->surface && width > 0 && height > 0) {
            //   _bs_resizeContext(context, width, height);
        }
        return DefWindowProc(hwnd, msg, w_param, l_param);
    }
    case WM_PAINT:
        if (context && context->window_type == BS_WINDOW_WIN32) {
            bool has_focus = !!GetFocus();

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT title_bar_rect;
            GetClientRect(hwnd, &title_bar_rect);

            COLORREF bg_color = RGB(83, 83, 83);
            HBRUSH bg_brush = CreateSolidBrush(bg_color);
            FillRect(hdc, &ps.rcPaint, bg_brush);
            DeleteObject(bg_brush);

            CustomTitleBarButtonRects button_rects = win32_get_title_bar_button_rects(hwnd, &title_bar_rect);

            _bs_drawIcon(context, hdc, &button_rects.minimize, L'\xE921');
            _bs_drawIcon(context, hdc, &button_rects.maximize, L'\xE922');
            _bs_drawIcon(context, hdc, &button_rects.close, L'\xE8BB');

            EndPaint(hwnd, &ps);
            return 0;
        }
        else {

            return 0;
        }
        return DefWindowProc(hwnd, msg, w_param, l_param);
    case WM_ACTIVATE:
        if (context && context->listener.activate) {
            context->listener.activate(context, (bs_ContextActivateParams) {
                .active = LOWORD(w_param) != WA_INACTIVE,
            });
        }

        break;
    case WM_DPICHANGED:
        _bs_updateWindowDPI(context);
        return DefWindowProc(hwnd, msg, w_param, l_param);
    case WM_NCHITTEST:
        if (context) {
            if (context->window_type == BS_WINDOW_WIN32) 
                break;

            if (context->window_type == BS_WINDOW_POPUP)
                return HTCLIENT;
            else if (_bs_callbacks_.client_area_tick) {
                bs_ivec2 pt = {
                    GET_X_LPARAM(l_param),
                    GET_Y_LPARAM(l_param)
                };

                bs_NonClientArea non_client_area = _bs_callbacks_.client_area_tick(context, pt);

                switch (non_client_area) {
                case BS_CLIENT_AREA:
                    return _bs_hitTestResize(context, pt, HTCLIENT);
                case BS_NON_CLIENT_AREA_CAPTION_BUTTON:
                    return _bs_hitTestResize(context, pt, HTCLIENT);
                case BS_NON_CLIENT_AREA_CAPTION:
                    return _bs_hitTestResize(context, pt, HTCAPTION);
                }

                return HTCLIENT;
            }
        }

        return DefWindowProc(hwnd, msg, w_param, l_param);
    case WM_NCCALCSIZE:

        if (context && context->window_type == BS_WINDOW_WIN32) {
            break;
        }

        RECT* rect = 0;

        if (w_param) {
            rect = ((NCCALCSIZE_PARAMS*)l_param)->rgrc;
        }
        else {
            rect = (RECT*)l_param;
        }

        if (context && context->window_type == BS_WINDOW_NO_TITLE_BAR) {

            // https://handmade.network/forums/articles/t/9073-custom_window_title_bar_and_almost_correctly_drawing_windows_10_borders
            // int padding = GetSystemMetrics(SM_CXPADDEDBORDER);
            // TODO: dpi shit
            int padding = 0;
            int borderLR = GetSystemMetrics(SM_CXFRAME) + padding;
            int borderTB = GetSystemMetrics(SM_CYFRAME) + padding;

            rect->left += borderLR;
            rect->right -= borderLR;
            rect->bottom -= borderTB;

            if (IsZoomed(hwnd)) {
                rect->top += borderTB;
            }
        }

        {
            int width = rect->right - rect->left;
            int height = rect->bottom - rect->top;
            if (_bs_instance_->physical_device && context && context->surface && width > 0 && height > 0) {
                _bs_resizeContext(context, width, height);
            }
        }
        break;


      //  else
      //      return DefWindowProc(hwnd, msg, w_param, l_param);
    case WM_MOUSEACTIVATE:

        if (context && context->window_type == BS_WINDOW_POPUP)
            return MA_NOACTIVATE;
        else
            return MA_ACTIVATE;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
		return 0;
//	case WM_SETCURSOR: {
//		// SetCursor(_bs_scope_.context->cursor_icons[_bs_wnd.cursor_icon].handle);
//	} break;
	default: return DefWindowProc(hwnd, msg, w_param, l_param);
    }
    return 0;
}
#endif

BSAPI void _bs_moveWindow(bs_Context* context, int x, int y) {
    bs_ivec2 resolution = _bs_resolution(context);
    #ifdef _WIN32
	SetWindowPos(context->hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
	#endif
}

void _test(bs_Context* context, const char* title);

BSAPI bs_Context* _bs_queryPopupWindow(bs_I32 id) {
    for (int i = 0; i < _bs_instance_->popup_windows.count; i++) {
        bs_Object* obj = *(bs_Object**)_bs_fetchUnit(&_bs_instance_->popup_windows, i);

        if (obj->context->popup.id == id)
            return obj->context->hidden ? NULL : obj->context;
    }

    return NULL;
}

BSAPI void _bs_closePopupWindow(bs_Context* context) {
    bs_hideWindow(context);
}

BSAPI bs_Result _val_bs_openPopupWindow(bs_ContextListener listener, bs_I32 id, bs_I32 x, bs_I32 y, bs_U32 width, bs_U32 height, static const char* title) {
    BS_VALIDATE(_bs_queryPopupWindow(id) == NULL, BS_RESULT_VALIDATION_ERROR,);
    return _bs_openPopupWindow(listener, id, x, y, width, height, title);
}

BSAPI bs_Result _bs_openPopupWindow(bs_ContextListener listener, bs_I32 id, bs_I32 x, bs_I32 y, bs_U32 width, bs_U32 height, static const char* title) {
    bs_Context* last_context = _bs_scope_.context;
    _bs_scope_.context = NULL;

    bs_Result result;
    bs_Context* context = NULL;

    for (int i = 0; i < _bs_instance_->popup_windows.count; i++) {
        bs_Object* obj = *(bs_Object**)_bs_fetchUnit(&_bs_instance_->popup_windows, i);

        if (obj->context->hidden) {
            context = obj->context;
            bs_showWindow(context);
            break;
        }
    }

    if (!context) {
        bs_Object* new_ctx = BS_CONTEXT(-1, -1, 0);

        result = _bs_window(new_ctx->context, NULL, listener, width, height, title, BS_WINDOW_POPUP);
        if (result != BS_RESULT_OK) {
            _bs_scope_.context = last_context;
            return result;
        }

        result = _bs_swapchain(new_ctx->context);
        if (result != BS_RESULT_OK) {
            _bs_scope_.context = last_context;
            return result;
        }

        bs_Object* obj = *(bs_Object**)bs_pushBack(&_bs_instance_->popup_windows, &new_ctx);
        context = obj->context;

        context->popup.id = id;
        context->popup.queue_obj = BS_QUEUE(-1, -1, BS_OBJECT_SWAPCHAIN_IMAGE_BIT);
        result = _bs_queue(context->popup.queue_obj, 0, BS_QUEUE_GRAPHICS_BIT);
        if (result != BS_RESULT_OK) {
            _bs_scope_.context = last_context;
            return result;
        }
    }
    else {
        _bs_resizeWindow(context, width, height);
    }

    _bs_moveWindow(context, x, y);
    _bs_showWindow(context);

    _bs_scope_.context = last_context;

    return BS_RESULT_OK;
}

BSAPI bs_Result _val_bs_window(
    bs_Context* context,
    bs_Context* parent,
    bs_ContextListener listener,
    bs_U32 width,
    bs_U32 height,
    const char* title,
    bs_WindowType type
) {
    BS_VALIDATE(type != BS_WINDOW_POPUP, BS_RESULT_VALIDATION_ERROR, "Popup windows should be created with _bs_openPopupWindow");

    return _bs_window(context, parent, listener, width, height, title, type);
}

BSAPI bs_Result _bs_window(
    bs_Context* context,
    bs_Context* parent,
    bs_ContextListener listener,
    bs_U32 width,
    bs_U32 height,
    const char* title,
    bs_WindowType type)
{
    bs_Context* last_context = _bs_scope_.context;
    _bs_scope_.context = context;

    context->hidden = true;
    context->listener = listener;
    context->title = title;
    context->dimensions = (bs_ivec2) { width, height };
    context->window_type = type;

    bs_Timer timer = _bs_timer();
    _bs_setTargetFramerate(24);

    #ifdef _WIN32
    const char* class_name = title;
    HINSTANCE hinstance = GetModuleHandle(0);

    HICON hicon = (HICON)LoadImage(
        NULL,
        "content/icon.ico",
        IMAGE_ICON,
        0, 0,
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    );

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_OWNDC,
        .lpfnWndProc = _bs_windowProcedure,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hinstance,
        .hIcon = hicon ? hicon : LoadIcon(NULL, IDI_APPLICATION),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
		//.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
		.lpszMenuName = NULL,
        .lpszClassName = class_name,
        //.hIconSm = LoadIcon(NULL, IDI_APPLICATION),
    };

    if (!RegisterClassEx(&wc)) {
        BS_WARN_WIN32_PATH("RegisterClassEx", title);
        _bs_scope_.context = last_context;
        return _bs_convertWin32Error(GetLastError());
    }

    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    DWORD ex_style = 0;
    HWND parent_hwnd = NULL;

    if (parent) {
        bs_Context* last_node = parent->first_child;
        bs_Context* node = parent->first_child;

        while (node) {
            last_node = node;
            node = node->next;
        }

        if (last_node)
            last_node->next = context;
        else
            parent->first_child = context;

        parent_hwnd = parent->hwnd;
        style = WS_CHILD | WS_VISIBLE;
    }
    else if (type == BS_WINDOW_NO_TITLE_BAR) {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        //style = WS_CAPTION | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_THICKFRAME | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        //ex_style = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_ACCEPTFILES | WS_EX_WINDOWEDGE;
    }
    else if (type == BS_WINDOW_POPUP) {
        style = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        ex_style = WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    } 
    else if (type == BS_WINDOW_WIN32) {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }

    context->hwnd = CreateWindowEx(
        ex_style,
        class_name,
        title,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        parent_hwnd, NULL, hinstance, NULL);

    if (!context->hwnd) {
        BS_WARN_WIN32_PATH("CreateWindowEx", title);
        _bs_scope_.context = last_context;
        return _bs_convertWin32Error(GetLastError());
    }

    PIXELFORMATDESCRIPTOR pixel_format_descriptor = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cRedBits = 0, .cRedShift = 0, .cGreenBits = 0, .cGreenShift = 0, .cBlueBits = 0, .cBlueShift = 0,
        .cAlphaBits = 0,
        .cAlphaShift = 0,
        .cAccumBits = 0,
        .cAccumRedBits = 0, .cAccumGreenBits = 0, .cAccumBlueBits = 0, .cAccumAlphaBits = 0,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .cAuxBuffers = 0,
        .iLayerType = PFD_MAIN_PLANE,
        .dwLayerMask = 0, .dwVisibleMask = 0, .dwDamageMask = 0
    };

    HDC hdc = GetDC(context->hwnd);
    int pixel_format = ChoosePixelFormat(hdc, &pixel_format_descriptor);
    SetPixelFormat(hdc, pixel_format, &pixel_format_descriptor);
    #endif

#ifdef __linux__
    _test(context, title);
#endif
    _bs_createSurface();

    #ifdef _WIN32
    if (context->window_type == BS_WINDOW_NO_TITLE_BAR) {
        RECT rect;
        GetWindowRect(context->hwnd, &rect);

        SetWindowPos(
            context->hwnd,
            NULL,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE
        );

        DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;

        DwmSetWindowAttribute(
            context->hwnd,
            DWMWA_WINDOW_CORNER_PREFERENCE,
            &preference,
            sizeof(preference)
        );
    }

    _bs_updateWindowDPI(context);
    #endif
    context->swapchain_ok = true;

    _bs_scope_.context = last_context;

    return BS_RESULT_OK;
 }



  /*==============================================================================
   * Wayland
   *============================================================================*/

#ifdef __linux__

 /**
  Base Listener
  */
static void _bs_onPing(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
    bs_Context* context = data;

    xdg_wm_base_pong(xdg_wm_base, serial);
}

static struct xdg_wm_base_listener _bs_xdg_wm_base_listener_ = {
    .ping = _bs_onPing,
};

 /**
  Registry Listener
  */
static void _bs_onGlobal(void* data, struct wl_registry *wl_registry, uint32_t name, const char* interface, uint32_t version) {
    bs_Context* context = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
        context->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        context->wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(context->wm_base, &_bs_xdg_wm_base_listener_, context);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
        context->shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
    else if (strcmp(interface, wl_seat_interface.name) == 0)
        context->seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, version);
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
        context->decoration_manager = wl_registry_bind(wl_registry, name, &zxdg_decoration_manager_v1_interface, version);
    else if (strcmp(interface, wp_viewporter_interface.name) == 0)
        context->viewporter = wl_registry_bind(wl_registry, name, &wp_viewporter_interface, version);
    else if (strcmp(interface, wp_single_pixel_buffer_manager_v1_interface.name) == 0)
        context->single_pixel_buffer_manager = wl_registry_bind(wl_registry, name, &wp_single_pixel_buffer_manager_v1_interface, version);
}

static void _bs_onGlobalRemove(void* data, struct wl_registry* wl_registry, uint32_t name) {
}

static struct wl_registry_listener _bs_registry_listener_ = {
    .global = _bs_onGlobal,
    .global_remove = _bs_onGlobalRemove,
};

 /**
  Toplevel Listener
  */
static void _bs_onToplevelConfigure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states) {
    bs_Context* context = data;
    if (width != 0)
        context->dimensions.x = width;
    if (height != 0)
        context->dimensions.y = height;
    xdg_surface_set_window_geometry(context->xdg_surface, 0, 0, context->dimensions.x, context->dimensions.y);
}

static void _bs_onClose(void* data, struct xdg_toplevel* xdg_toplevel) {
    bs_Context* context = data;
    // context->close = true;
}

static void _bs_onConfigureBounds(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height) {
    bs_Context* context = data;
}

static void _bs_onWmCapabilities(void* data, struct xdg_toplevel* xdg_toplevel, struct wl_array* capabilities) {
    bs_Context* context = data;
}

static struct xdg_toplevel_listener _bs_toplevel_listener_ = {
    .configure = _bs_onToplevelConfigure,
    .close = _bs_onClose,
    .configure_bounds = _bs_onConfigureBounds,
    .wm_capabilities = _bs_onWmCapabilities,
};

 /**
  XDG Surface Listener
  */
static void _bs_onConfigureSurfaceListener(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    bs_Context* context = data;

    xdg_surface_ack_configure(xdg_surface, serial);
   //wl_surface_attach(context->_wl_surface, context->buffer, 0, 0);
    wl_surface_damage_buffer(context->_wl_surface, 0, 0, context->dimensions.x, context->dimensions.y);
    wp_viewport_set_destination(context->viewport, context->dimensions.x, context->dimensions.y);
    wl_surface_commit(context->_wl_surface);
}
static struct xdg_surface_listener _bs_surface_listener_ = {
    .configure = _bs_onConfigureSurfaceListener,
};

 /**
  Pointer Listener
  */

static void _bs_onPointerEnter(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t x,
    wl_fixed_t y
)
{
    bs_Context* context = data;
    struct wl_cursor_image* cursor_image = _bs_instance_->wl.cursor_image;
    wl_pointer_set_cursor(
        pointer,
        serial,
        _bs_instance_->wl.cursor_surface,
        cursor_image->hotspot_x,
        cursor_image->hotspot_y);
}

static void _bs_onPointerLeave(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface
) {
    bs_Context* context = data;
}

static void _bs_onPointerMotion(
    void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    wl_fixed_t x,
    wl_fixed_t y
) {
    bs_Context* context = data;
    context->cursor = BS_V2((float)wl_fixed_to_int(x), (float)wl_fixed_to_int(y));
}

static void _bs_onPointerButton(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state
) {
    bs_Context* context = data;

    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        switch (button) {
            case 0x110: bs_setBit(context->io.input_down_events, BS_LEFT_MOUSE_BUTTON); break;
            case 0x111: bs_setBit(context->io.input_down_events, BS_RIGHT_MOUSE_BUTTON); break;
            case 0x112: bs_setBit(context->io.input_down_events, BS_MIDDLE_MOUSE_BUTTON); break;
        }
    } else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
        switch (button) {
            case 0x110: bs_setBit(context->io.input_up_events, BS_LEFT_MOUSE_BUTTON); break;
            case 0x111: bs_setBit(context->io.input_up_events, BS_RIGHT_MOUSE_BUTTON); break;
            case 0x112: bs_setBit(context->io.input_up_events, BS_MIDDLE_MOUSE_BUTTON); break;
        }
    }
}

static void _bs_onPointerAxis(
    void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value
) {
    bs_Context* context = data;
}

static void _bs_onPointerFrame(
    void* data,
    struct wl_pointer* wl_pointer
) {
    bs_Context* context = data;
}

static void _bs_onPointerAxisSource(
    void* data,
    struct wl_pointer* wl_pointer,
    uint32_t axis_source
) {
    bs_Context* context = data;
}

static void _bs_onPointerAxisStop(
    void* data,
    struct wl_pointer* wl_pointer,
    uint32_t time,
    uint32_t axis_source
) {
    bs_Context* context = data;
}

static void _bs_onPointerAxisDiscrete(
    void* data,
    struct wl_pointer* wl_pointer,
    uint32_t axis,
    int32_t discrete
) {
    bs_Context* context = data;
}

static const struct wl_pointer_listener _bs_pointer_listener_ = {
    .enter = _bs_onPointerEnter,
    .leave = _bs_onPointerLeave,
    .motion = _bs_onPointerMotion,
    .button = _bs_onPointerButton,
    .axis = _bs_onPointerAxis,
    .frame = _bs_onPointerFrame,
    .axis_source = _bs_onPointerAxisSource,
    .axis_stop = _bs_onPointerAxisStop,
    .axis_discrete = _bs_onPointerAxisDiscrete,
};

void _test(bs_Context* context, const char* title) {
    context->display = wl_display_connect(NULL);
    if (!context->display) {
        BS_WARN_ERRNO_PATH("wl_display_connect", title);
        return;
    }

    context->registry = wl_display_get_registry(context->display);

    wl_registry_add_listener(context->registry, &_bs_registry_listener_, context);
    wl_display_roundtrip(context->display);

    context->buffer = wp_single_pixel_buffer_manager_v1_create_u32_rgba_buffer(context->single_pixel_buffer_manager, BS_U32_MAX, 0, 0, BS_U32_MAX);

    context->_wl_surface = wl_compositor_create_surface(context->compositor);
    context->viewport = wp_viewporter_get_viewport(context->viewporter, context->_wl_surface);
    context->xdg_surface = xdg_wm_base_get_xdg_surface(context->wm_base, context->_wl_surface);

    xdg_surface_add_listener(context->xdg_surface, &_bs_surface_listener_, context);

    context->xdg_toplevel = xdg_surface_get_toplevel(context->xdg_surface);

    xdg_toplevel_add_listener(context->xdg_toplevel, &_bs_toplevel_listener_, context);
    xdg_toplevel_set_title(context->xdg_toplevel, title);

    if (context->decoration_manager) {
        context->decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(context->decoration_manager, context->xdg_toplevel);
        zxdg_toplevel_decoration_v1_set_mode(context->decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    wl_surface_commit(context->_wl_surface);
    wl_display_roundtrip(context->display);

    if (context->seat) {
        _bs_instance_->wl.pointer = wl_seat_get_pointer(context->seat);
        wl_pointer_add_listener(_bs_instance_->wl.pointer, &_bs_pointer_listener_, context);

        struct wl_cursor_theme *cursor_theme = wl_cursor_theme_load(NULL, 24, context->shm);
        struct wl_cursor *cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");

        _bs_instance_->wl.cursor_image = cursor->images[0];
        struct wl_buffer *cursor_buffer = wl_cursor_image_get_buffer(_bs_instance_->wl.cursor_image);

        _bs_instance_->wl.cursor_surface = wl_compositor_create_surface(context->compositor);
        wl_surface_attach(_bs_instance_->wl.cursor_surface, cursor_buffer, 0, 0);
        wl_surface_commit(_bs_instance_->wl.cursor_surface);
    }
    wl_display_roundtrip(context->display);
}
#endif