//#include <vulkan/vulkan.h>//already included with glfw3
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//for compiling to SPIR-V internally
#include <shaderc/shaderc.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <fstream>



const int WIDTH = 800;
const int HEIGHT = 600;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExtensions = { //for determining swapchain support
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
//#define NDEBUG
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


class HelloTriangleApplication {

public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device; //logical device
    VkQueue graphicsQueue; //from logical device
    VkQueue presentQueue; //what to present on the surface
    VkSwapchainKHR swapChain; //struct
    std::vector<VkImage> swapChainImages; //actual images to queue in swapChain
    VkFormat swapChainImageFormat; //in the swap chain, but used later
    VkExtent2D swapChainExtent; //in the swap chain, but used later
    std::vector<VkImageView> swapChainImageViews;//To use any VkImage, like ones in swap chain, in the render pipeline we have to create a VkImageView object
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout; //for uniform shader values (TODO)
    VkPipeline graphicsPipeline;

    void initWindow() {
      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

    }

    void initVulkan() {
      createInstance();
      createSurface();
      pickPhysicalDevice();
      createLogicalDevice();
      createSwapChain();
      createImageViews();
      createRenderPass();
      //this is the piece that we're gonna abstract for classwork
      createGraphicsPipeline();
    }
    /**** Create Vulkan Instance ****/
    void createInstance() {
      //check for validation layers when relevant
      if (enableValidationLayers) {
        std::cout << "Running in DEBUG mode" << std::endl;
      }

      if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
      }

      //optional struct, good habit)
      VkApplicationInfo appInfo = {};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pNext = nullptr; //point to extension information in the future
      appInfo.pApplicationName = "Hello Triangle";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "No Engine";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_0; //current as of 2019

      //required struct, tells the Vulkan driver global extensions and validation layers to use)
      VkInstanceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pNext = nullptr; //pointer to an extension-specific structure
      //createInfo.flags = VkInstanceCreateFlags struct, unused in 2019
      createInfo.pApplicationInfo = &appInfo; //struct from above
      //Layers
      if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
      } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr; //array of names of layers to enable
      }
      //Extensions
      //GLFW has a built-in fn that returns the extension(s) it needs to interface with the window system
      uint32_t glfwExtensionCount = 0;
      const char** glfwExtensions;
      glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
      createInfo.enabledExtensionCount = glfwExtensionCount; //number of global extensions to enable
      createInfo.ppEnabledExtensionNames = glfwExtensions; //array of names of extensions to enable

      //create an vulkan instance, put it into VkInstance instance
      //                  pCreateInfo pAllocator pInstance
      if(vkCreateInstance(&createInfo, nullptr, &instance)!=VK_SUCCESS){
        //means either VK_ERROR_LAYER_NOT_PRESENT or VK_ERROR_EXTENSION_NOT_PRESENT
        throw std::runtime_error("createInstance() failed!");
      }
      showInstanceInfo(createInfo);
    }

    bool checkValidationLayerSupport() {

      uint32_t layerCount;
      vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
      std::vector<VkLayerProperties> availableLayers(layerCount);
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

      for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
          if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
          }
        }
        if (!layerFound) {
          return false;
        }
      }
      return true;
    }

    void showInstanceInfo(VkInstanceCreateInfo info) {
      //extension support (all shown for optimality)
      uint32_t extensionCount = 0;         //Layer    count            VkExtensionProperties
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
      //array to hold extension details now that we know how many there are
      std::vector<VkExtensionProperties> availableExtensions(extensionCount);
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
      //list the extensions out, check if required by GLFW
      std::cout << "available extensions:" << std::endl;
      bool enabled;
      for (const auto& extension : availableExtensions) {
        enabled = false;
        for (int i = 0; i < info.enabledExtensionCount; i++) {
          if(strcmp(info.ppEnabledExtensionNames[i], extension.extensionName)==0) {
            enabled = true;
          }
        }
        if (enabled) {
          std::cout << "\t" << extension.extensionName << " (enabled)" << std::endl;
        }
        else{
          std::cout << "\t" << extension.extensionName << std::endl;
        }
      }
      //layers enabled
      std::cout << "enabled layers:" << std::endl;
      if (info.enabledLayerCount == 0) {
        std::cout << "\tno layers enabled" << std::endl;
      }
      else{
        for (int i = 0; i < info.enabledLayerCount; i++) {
          std::cout << "\t" << info.ppEnabledLayerNames[i] << std::endl;
        }
      }
    }
    /**** Create Vulkan Instance ****/

    /**** Create Window Surface ****/
    void createSurface() {
      if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
      }
    }
    /**** Create Window Surface ****/

    /**** Pick Compatiable Physical Device ****/
    void pickPhysicalDevice() {
      //pick the gpu(s) to use
      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
      if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
      }
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
      //just pick the first valid one
      for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
          physicalDevice = device;
          break;
        }
      }
      if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
      }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
      //could search for and rank physical devices based on below vk command
      /* VkPhysicalDeviceProperties deviceProperties;
      // vkGetPhysicalDeviceProperties(device, &deviceProperties);
      // VkPhysicalDeviceFeatures deviceFeatures;
      // vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
      // int score = 0;
      // // Discrete GPUs have a significant performance advantage
      // if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      //   score += 1000;
      // }
      // // Maximum possible size of textures affects graphics quality
      // score += deviceProperties.limits.maxImageDimension2D;
      // // Application can't function without geometry shaders
      // if (!deviceFeatures.geometryShader) {
      //   return 0;
      // }
      // return score;
      */

      //see if pysical device queue family supports queues for logical device
      QueueFamilyIndices indices = findQueueFamilies(device);
      //see if physical device supports extensions needed for swapchain
      bool extensionsSupported = checkDeviceExtensionSupport(device);
      //confirm that swapchain and window surface are compatable
      bool swapChainAdequate = false;
      if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
      }
      return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    struct QueueFamilyIndices {
      std::optional<uint32_t> graphicsFamily; //optional<> allows has_value() query
      std::optional<uint32_t> presentFamily; //for ensuring device surface is compatible
      bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
      }
    };
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
      QueueFamilyIndices indices;
      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
      /* queueFamilies.data populated with:
      typedef struct VkQueueFamilyProperties {
            VkQueueFlags    queueFlags;
            uint32_t        queueCount;
            uint32_t        timestampValidBits;
            VkExtent3D      minImageTransferGranularity;
        }
      */
      int i = 0;
      for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          indices.graphicsFamily = i;
        }
        //if we find valid graphics, see if also valid surface compatibility
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
          indices.presentFamily = i;
        }
        if (indices.isComplete()) {
          break; //dunno why this isn't just above but okay
        }
        i++;
      }
      return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
      //enumerate the extensions and check if all of the required extensions are amongst them
      uint32_t extensionCount;
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions(extensionCount);
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
      std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
      for (const auto& extension : availableExtensions) {
          requiredExtensions.erase(extension.extensionName);
      }

      return requiredExtensions.empty();
    }
    /**** Pick Compatiable Physical Device ****/

    /**** Use Compatable Physical Device to create Logical Device ****/
    void createLogicalDevice() { //initializes vk structs for queues in queueFamilies
      QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
      std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
      //after finding queue indices, figure out which ones in the family(s) are unique
      std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
      for (uint32_t queueFamily : uniqueQueueFamilies) {
        //logical device queue
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.pNext = nullptr;
        //queueCreateInfo.flags = VkInstanceCreateFlags struct //bitmask indicating behavior of the queue
        queueCreateInfo.queueFamilyIndex = queueFamily;//indices.[graphics/present]Family.value();
        queueCreateInfo.queueCount = 1; //THIS DESCRIBES THE NUMBER OF QUEUES TO USE IN A SINGLE QUEUE FAMILY https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues#page_Specifying-the-queues-to-be-created
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
      }
      //logical device features from physical device
      VkPhysicalDeviceFeatures deviceFeatures = {}; //coulg get these to put into VkDeviceCreateInfo struct, but optional

      //logical device!
      VkDeviceCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      createInfo.pNext = nullptr;
      //createInfo.flags = VkInstanceCreateFlags struct;
      createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
      createInfo.pQueueCreateInfos = queueCreateInfos.data();
      if (enableValidationLayers) {
          createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
          createInfo.ppEnabledLayerNames = validationLayers.data();
      } else {
          createInfo.enabledLayerCount = 0;
      }
      //currently only used for swapchain extension
      createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
      createInfo.ppEnabledExtensionNames = deviceExtensions.data();
      createInfo.pEnabledFeatures = &deviceFeatures;

      if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
      }
      //use build in fn to get logical device's queue handle (for the graphicsQueue)
      vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
      //use build in fn to get logical device's queue handle (for the graphicsQueue)
      vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }
    /**** Use Compatable Physical Device to create Logical Device ****/

    /**** Create SwapChain ****/
    void createSwapChain() {
      //see "Creating the Swap Chain" section of SwapChain Chapter in tutorial for details
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
      VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
      VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
      VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

      uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
      if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
      }
      VkSwapchainCreateInfoKHR createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      createInfo.pNext = nullptr;
      //queueCreateInfo.flags = //bitmask of VkSwapchainCreateFlagBitsKHR indicating parameters of the swapchain creation.
      createInfo.surface = surface;
      //image details
      createInfo.minImageCount = imageCount;
      createInfo.imageFormat = surfaceFormat.format;
      swapChainImageFormat = surfaceFormat.format;//for later use in createImageViews
      createInfo.imageColorSpace = surfaceFormat.colorSpace;
      createInfo.imageExtent = extent; //currently only 2d but can be 3d
      swapChainExtent = extent;//for later use in viewPort creation
      createInfo.imageArrayLayers = 1; //amount of layers each image consists of; always 1 unless you are developing a stereoscopic 3D application
      createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //kind of operations we'll use the images in the swap chain for
      //queue details
      QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
      uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

      if (indices.graphicsFamily != indices.presentFamily) {
          createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
          createInfo.queueFamilyIndexCount = 2;
          createInfo.pQueueFamilyIndices = queueFamilyIndices;
      } else {
          createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
          createInfo.queueFamilyIndexCount = 0; // Optional
          createInfo.pQueueFamilyIndices = nullptr; // Optional
      }
      createInfo.preTransform = swapChainSupport.capabilities.currentTransform;;
      createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //ignore alpha
      createInfo.presentMode = presentMode;
      createInfo.clipped = VK_TRUE; //ignore pixels with other things in front of them
      //createInfo.oldSwapchain;//assume we only ever make one swapchain, come back to this later TODO
      if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
      }
      //now get the images
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
      swapChainImages.resize(imageCount);
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    }

    /*
    Just checking if a swap chain is available is not sufficient, because it may
    not actually be compatible with our window surface. Creating a swap chain
    also involves a lot more settings than instance and device creation, so we
    need to query for some more details.
    */
    struct SwapChainSupportDetails {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
    };
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
      SwapChainSupportDetails details;
      //fill in capabilities
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
      //fill in supported surface formats
      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
      if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
      }
      //fill in supported presentation modes
      uint32_t presentModeCount;
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
      if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
      }
      return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
      VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_UNORM;
      VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      //similar to physical device, can rank and select best, but for now just select first working one
      for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == preferredFormat && availableFormat.colorSpace == preferredColorSpace) {
          return availableFormat;
        }
      }
      std::cout << "Preferred format and colorSpace combination for swapchain surface not found."
                << " Using first available format." << std::endl;
      return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
      // VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR
      // VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR
      VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == preferredPresentMode) {
          return availablePresentMode;
        }
      }
      //garunteed
      return VK_PRESENT_MODE_FIFO_KHR;
    }

    //swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution of the window that we're drawing to
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
      if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
      } else {
        VkExtent2D actualExtent = {WIDTH, HEIGHT};
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
      }
    }
    /**** Create SwapChain ****/

    /**** Create Image Views ****/
    void createImageViews() {
      swapChainImageViews.resize(swapChainImages.size());
      for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        //ALSO has pNext and flags, but just not gonna write it out again
        createInfo.image = swapChainImages[i];
        //guidelines for interpreting data
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        //color channels
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        //image's purpose is and which part of the image should be accessed.
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create image views!");
        }
      }
    }
    /**** Create Image Views ****/
    /*Before we can finish creating the pipeline, we need to tell Vulkan about
    the framebuffer attachments that will be used while rendering. We need to
    specify how many color and depth buffers there will be, how many samples to
    use for each of them and how their contents should be handled throughout the
    rendering operations. All of this information is wrapped in a render pass object*/
    /**** Create Render Pass ****/
    void createRenderPass() {
      //attachment description
      VkAttachmentDescription colorAttachment = {};
      colorAttachment.format = swapChainImageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      //subpasses and attachment references
      VkAttachmentReference colorAttachmentRef = {};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;
      //build the render pass!
      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = 1;
      renderPassInfo.pAttachments = &colorAttachment;
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;

      if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
          throw std::runtime_error("failed to create render pass!");
      }
    }
    /**** Create Render Pass ****/

    /**** Create Graphics Pipeline ****/
    void createGraphicsPipeline() {
      //use libshaderc to compile shaders internally! (NOT FROM TUTORIAL)
      //first, convert .vert and .frag to strings
      std::ifstream vertStream("shaders/vertexShaderHack.vert");
      std::string vertShaderString((std::istreambuf_iterator<char>(vertStream)), std::istreambuf_iterator<char>());
      std::ifstream fragStream("shaders/fragmentShaderRed.frag");
      std::string fragShaderString((std::istreambuf_iterator<char>(fragStream)), std::istreambuf_iterator<char>());
      //second, compile into SPIR-V, which return a shaderc_compilation_result_t
      shaderc_compiler_t compiler = shaderc_compiler_initialize();
      shaderc_compilation_result_t vertShaderCode = shaderc_compile_into_spv(
         compiler, vertShaderString.c_str(), vertShaderString.size(),
         shaderc_glsl_vertex_shader, "shaders/vertexShaderHack.vert", "main", nullptr);
      shaderc_compilation_result_t fragShaderCode = shaderc_compile_into_spv(
         compiler, fragShaderString.c_str(), fragShaderString.size(),
         shaderc_glsl_fragment_shader, "shaders/fragmentShaderRed.frag", "main", nullptr);
      //initialize vectors from the getter functions of the shaderc_compilation_result_t (|byte| == |char|)
      std::vector<char> vertShaderCodeVector(shaderc_result_get_bytes(vertShaderCode),
                          shaderc_result_get_bytes(vertShaderCode)+shaderc_result_get_length(vertShaderCode));
      std::vector<char> fragShaderCodeVector(shaderc_result_get_bytes(vertShaderCode),
                          shaderc_result_get_bytes(vertShaderCode)+shaderc_result_get_length(vertShaderCode));
      // // if you were to do manual compilation, you might read the binary code like this
      // auto vertShaderCodeVector = readFileSPV("shaders/vert.spv");
      // auto fragShaderCodeVector = readFileSPV("shaders/frag.spv");
      VkShaderModule vertShaderModule = createShaderModule(vertShaderCodeVector);
      VkShaderModule fragShaderModule = createShaderModule(fragShaderCodeVector);

      //vertex shader in pipeline
      VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
      vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
      vertShaderStageInfo.module = vertShaderModule;
      vertShaderStageInfo.pName = "main"; //function to invoke aka entrypoint
      vertShaderStageInfo.pSpecializationInfo = nullptr; //specify values for shader constants
      //fragment shader in pipeline
      VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
      fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      fragShaderStageInfo.module = fragShaderModule;
      fragShaderStageInfo.pName = "main";
      fragShaderStageInfo.pSpecializationInfo = nullptr; //specify values for shader constants
      //for reference later
      VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
      //explicit declaration: Vertex input: format of the vertex data
      VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
      vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        //empty right now because we hard-coded the vertex stuff in the shader. come back later TODO
      //Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
      vertexInputInfo.vertexBindingDescriptionCount = 0;
      vertexInputInfo.pVertexBindingDescriptions = nullptr;
      //Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
      vertexInputInfo.vertexAttributeDescriptionCount = 0;
      vertexInputInfo.pVertexAttributeDescriptions = nullptr;
      //explicit declaration: Input assembly:  what kind of geometry will be drawn from the vertices and if primitive restart should be enabled.
      VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
      inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //we're draing a triangle
      inputAssembly.primitiveRestartEnable = VK_FALSE;
      //explicit declaration: Viewport: where in the buffer we render to (usually 0,0 to width,height)
      VkViewport viewport = {};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = (float) swapChainExtent.width;
      viewport.height = (float) swapChainExtent.height;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      /*Remember that the size of the swap chain and its images may differ from
      the WIDTH and HEIGHT of the window. The swap chain images will be used as
      framebuffers later on, so we should stick to their size.
      The minDepth and maxDepth values specify the range of depth values to use
      for the framebuffer. These values must be within the [0.0f, 1.0f] range,
      but minDepth may be higher than maxDepth. If you aren't doing anything
      special, then you should stick to the standard values of 0.0f and 1.0f.*/
      VkRect2D scissor = {};
      scissor.offset = {0, 0};
      scissor.extent = swapChainExtent;
      /*In this tutorial we simply want to draw to the entire framebuffer, so
      we'll specify a scissor rectangle that covers it entirely:.*/
      VkPipelineViewportStateCreateInfo viewportState = {};
      viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      viewportState.viewportCount = 1;
      viewportState.pViewports = &viewport;
      viewportState.scissorCount = 1;
      viewportState.pScissors = &scissor;
      //explicit declaration: Rasterizer: The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering)
      VkPipelineRasterizationStateCreateInfo rasterizer = {};
      rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rasterizer.depthClampEnable = VK_FALSE;
      rasterizer.rasterizerDiscardEnable = VK_FALSE; //we need to output to the framebuffer
      rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //fill, line, or point
      rasterizer.lineWidth = 1.0f;
      rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
      rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
      rasterizer.depthBiasEnable = VK_FALSE; //bias for shadowmapping
      rasterizer.depthBiasConstantFactor = 0.0f; // Optional
      rasterizer.depthBiasClamp = 0.0f; // Optional
      rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
      //explicit declaration: Multisampling: perform anti-aliasing
      VkPipelineMultisampleStateCreateInfo multisampling = {};
      multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      multisampling.sampleShadingEnable = VK_FALSE;
      multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
      multisampling.minSampleShading = 1.0f; // Optional
      multisampling.pSampleMask = nullptr; // Optional
      multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
      multisampling.alphaToOneEnable = VK_FALSE; // Optional
      //DEPTH AND STENCIL TESTS OMITTED HERE ::: TODO

      //explicit declaration: Color Blending
      VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = VK_FALSE;
      colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
      colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
      colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
      colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
      colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
      colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
      //alternative for blending enabled:
      // colorBlendAttachment.blendEnable = VK_TRUE;
      // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
      // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
      VkPipelineColorBlendStateCreateInfo colorBlending = {};
      colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      colorBlending.logicOpEnable = VK_FALSE; //change if blending is enabled
      colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
      colorBlending.attachmentCount = 1;
      colorBlending.pAttachments = &colorBlendAttachment;
      colorBlending.blendConstants[0] = 0.0f; // Optional
      colorBlending.blendConstants[1] = 0.0f; // Optional
      colorBlending.blendConstants[2] = 0.0f; // Optional
      colorBlending.blendConstants[3] = 0.0f; // Optional
      //explicit declaration: Dynamic state: don't have to recreate entire pipeline sometimes
      VkDynamicState dynamicStates[] = {
          VK_DYNAMIC_STATE_VIEWPORT,
          VK_DYNAMIC_STATE_LINE_WIDTH
      };
      VkPipelineDynamicStateCreateInfo dynamicState = {};
      dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      dynamicState.dynamicStateCount = 2;
      dynamicState.pDynamicStates = dynamicStates;
      //explicit declaration: Pipeline layout: specify uniform values (TODO cuz we don't have any yet)
      VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
      pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutInfo.setLayoutCount = 0; // Optional
      pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
      pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
      pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
      if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
          throw std::runtime_error("failed to create pipeline layout!");
      }
      //final steps
      VkGraphicsPipelineCreateInfo pipelineInfo = {};
      pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipelineInfo.stageCount = 2;
      pipelineInfo.pStages = shaderStages;
      pipelineInfo.pVertexInputState = &vertexInputInfo;
      pipelineInfo.pInputAssemblyState = &inputAssembly;
      pipelineInfo.pViewportState = &viewportState;
      pipelineInfo.pRasterizationState = &rasterizer;
      pipelineInfo.pMultisampleState = &multisampling;
      pipelineInfo.pDepthStencilState = nullptr; // Optional
      pipelineInfo.pColorBlendState = &colorBlending;
      pipelineInfo.pDynamicState = nullptr; // Optional
      pipelineInfo.layout = pipelineLayout;
      pipelineInfo.renderPass = renderPass;
      pipelineInfo.subpass = 0;
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
      pipelineInfo.basePipelineIndex = -1; // Optional
      if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
          throw std::runtime_error("failed to create graphics pipeline!");
      }
      vkDestroyShaderModule(device, fragShaderModule, nullptr);
      vkDestroyShaderModule(device, vertShaderModule, nullptr);
      shaderc_result_release(vertShaderCode);
      shaderc_result_release(fragShaderCode);
      shaderc_compiler_release(compiler);
    }

    //for reading the shader's binary spirv code
    static std::vector<char> readFileSPV(const std::string& filename) {
      std::ifstream file(filename, std::ios::ate | std::ios::binary);
      if (!file.is_open()) {
          throw std::runtime_error("failed to open file!");
      }
      size_t fileSize = (size_t) file.tellg();
      std::vector<char> buffer(fileSize);
      file.seekg(0);
      file.read(buffer.data(), fileSize);
      file.close();
      return buffer;
    }

    /*
    Creating a shader module is simple, we only need to specify a pointer to the
    buffer with the bytecode and the length of it. This information is specified
    in a VkShaderModuleCreateInfo structure. The one catch is that the size of
    the bytecode is specified in bytes, but the bytecode pointer is a uint32_t
    pointer rather than a char pointer. Therefore we will need to cast the
    pointer with reinterpret_cast as shown below. When you perform a cast like
    this, you also need to ensure that the data satisfies the alignment
    requirements of uint32_t. Lucky for us, the data is stored in an std::vector
    where the default allocator already ensures that the data satisfies the
    worst case alignment requirements.
    */
    VkShaderModule createShaderModule(const std::vector<char>& code) {
      VkShaderModuleCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      //pNext, flags
      createInfo.codeSize = code.size();
      createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
      VkShaderModule shaderModule;
      if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
      }
      return shaderModule;
    }
    /**** Create Graphics Pipeline ****/


    void mainLoop() {
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
      }
    }

    void cleanup() {
      vkDestroyPipeline(device, graphicsPipeline, nullptr);
      vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
      vkDestroyRenderPass(device, renderPass, nullptr);
      for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
      }
      vkDestroySwapchainKHR(device, swapChain, nullptr);
      vkDestroyDevice(device, nullptr);
      vkDestroySurfaceKHR(instance, surface, nullptr);
      vkDestroyInstance(instance, nullptr);

      glfwDestroyWindow(window);
      glfwTerminate();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
