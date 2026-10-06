#define GR_VK_VERISON VK_API_VERSION_1_3
#define VMA_IMPLEMENTATION

#include "pch.hpp"
#include "scope.hpp"
#include <glfw/glfw3.h>

void RenderScope::_createDevice()
{
	std::vector<const char*> extensionsList = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	auto enumerateExtensions = [extensionsList](const VkPhysicalDevice& physicalDevice)
	{
		uint32_t extensionCount;

		vkEnumerateDeviceExtensionProperties(physicalDevice, VK_NULL_HANDLE, &extensionCount, VK_NULL_HANDLE);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, VK_NULL_HANDLE, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(extensionsList.begin(), extensionsList.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	};

	uint32_t devicesCount;
	std::vector<VkPhysicalDevice> devicesArray;

	vkEnumeratePhysicalDevices(m_VkInstance, &devicesCount, VK_NULL_HANDLE);
	devicesArray.resize(devicesCount);
	vkEnumeratePhysicalDevices(m_VkInstance, &devicesCount, devicesArray.data());

	for (auto physicalDevice : devicesArray)
	{
		if (enumerateExtensions(physicalDevice) && _createLogicalDevice(physicalDevice, extensionsList))
		{
			m_PhysicalDevice = physicalDevice;
			break;
		}
	}

	assert(m_PhysicalDevice != VK_NULL_HANDLE && m_LogicalDevice != VK_NULL_HANDLE);
}

bool RenderScope::_createLogicalDevice(VkPhysicalDevice physicalDevice, const std::vector<const char*>& device_extensions)
{
	VkPhysicalDeviceSynchronization2Features availableSynchronization2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
	VkPhysicalDeviceVulkan12Features availableFeaturesVk12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &availableSynchronization2Features };
	VkPhysicalDeviceFeatures2 availableFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &availableFeaturesVk12 };
	vkGetPhysicalDeviceFeatures2(physicalDevice, &availableFeatures);

	if (availableFeatures.features.imageCubeArray != VK_TRUE ||
		availableFeatures.features.fullDrawIndexUint32 != VK_TRUE ||
		availableFeatures.features.shaderFloat64 != VK_TRUE ||
		availableFeatures.features.samplerAnisotropy != VK_TRUE ||
		availableFeatures.features.independentBlend != VK_TRUE ||
		availableFeatures.features.fillModeNonSolid != VK_TRUE ||
		availableFeatures.features.geometryShader != VK_TRUE ||
		availableFeatures.features.multiDrawIndirect != VK_TRUE ||
		availableFeaturesVk12.drawIndirectCount != VK_TRUE ||
		availableSynchronization2Features.synchronization2 != VK_TRUE)
	{
		return false;
	}

	VkPhysicalDeviceSynchronization2Features synchronization2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
	synchronization2Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceVulkan12Features featureVk12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &synchronization2Features };
	featureVk12.drawIndirectCount = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.imageCubeArray = VK_TRUE;
	deviceFeatures.fullDrawIndexUint32 = VK_TRUE;
	deviceFeatures.shaderFloat64 = VK_TRUE;
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.independentBlend = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.multiDrawIndirect = VK_TRUE;

	uint32_t qFamiliesCount;
	std::vector<VkDeviceQueueCreateInfo> qInfos;
	std::vector<VkQueueFamilyProperties> qFamilies;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamiliesCount, VK_NULL_HANDLE);
	qFamilies.resize(qFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamiliesCount, qFamilies.data());

	float queuePriority = 1.0f;
	for (auto& queueFamily : qFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfo.queueFamilyIndex = qInfos.size();
		queueCreateInfo.queueCount = 1;
		qInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.ppEnabledExtensionNames = device_extensions.data();
	createInfo.enabledExtensionCount = device_extensions.size();
	createInfo.queueCreateInfoCount = qInfos.size();
	createInfo.pQueueCreateInfos = qInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.pNext = &featureVk12;

	return vkCreateDevice(physicalDevice, &createInfo, VK_NULL_HANDLE, &m_LogicalDevice) == VK_SUCCESS;
}

VkBool32 RenderScope::_checkValidationLayerSupport() const
{
#if _DEBUG
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, VK_NULL_HANDLE);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	const std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	for (const char* layerName : ValidationLayers)
	{
		VkBool32 layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
#else
	return false;
#endif
}

VkBool32 RenderScope::_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void RenderScope::_createVulkanInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "AVR_App";
	appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.pEngineName = "AVR";
	appInfo.engineVersion = VK_MAKE_API_VERSION(3, 0, 0, 0);
	appInfo.apiVersion = GR_VK_VERISON;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	bool validation = _checkValidationLayerSupport();

	if (validation)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
		debugCreateInfo.pfnUserCallback = _debugCallback;

		createInfo.ppEnabledLayerNames = layers.data();
		createInfo.enabledLayerCount = layers.size();
		createInfo.pNext = &debugCreateInfo;
	}

	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = extensions.size();

	auto res = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &m_VkInstance);
	assert(res == VK_SUCCESS);

	if (validation)
	{
		if (PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VkInstance, "vkCreateDebugUtilsMessengerEXT"))
		{
			func(m_VkInstance, &debugCreateInfo, VK_NULL_HANDLE, &m_DebugMessenger);
		}
	}
}

void RenderScope::_createMemoryAllocator()
{
	VmaAllocatorCreateInfo createInfo{};
	createInfo.physicalDevice = m_PhysicalDevice;
	createInfo.vulkanApiVersion = GR_VK_VERISON;
	createInfo.device = m_LogicalDevice;
	createInfo.instance = m_VkInstance;

	auto res = vmaCreateAllocator(&createInfo, &m_Allocator);
	assert(res == VK_SUCCESS);
}

void RenderScope::_collectQueues()
{
	uint32_t gQ = ~0, cQ = ~0, tQ = ~0;

	uint32_t qFamiliesCount;
	std::vector<VkQueueFamilyProperties> qFamilies;

	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &qFamiliesCount, VK_NULL_HANDLE);
	qFamilies.resize(qFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &qFamiliesCount, qFamilies.data());

	uint32_t qFamilyIndex = 0;
	for (auto& qProp : qFamilies)
	{
		if (CheckFlag(qProp.queueFlags, VK_QUEUE_GRAPHICS_BIT) && gQ == ~0)
		{
			gQ = qFamilyIndex;
		}
		else
		{
			if (CheckFlag(qProp.queueFlags, VK_QUEUE_COMPUTE_BIT) && cQ == ~0)
			{
				cQ = qFamilyIndex;
			}
			else if (CheckFlag(qProp.queueFlags, VK_QUEUE_TRANSFER_BIT) && tQ == ~0)
			{
				tQ = qFamilyIndex;
			}
		}

		qFamilyIndex++;
	}

	assert(gQ != ~0);

	m_Queues.emplace(std::piecewise_construct, std::forward_as_tuple(VK_QUEUE_GRAPHICS_BIT), std::forward_as_tuple(m_LogicalDevice, gQ));

	if (cQ != ~0)
		m_Queues.emplace(std::piecewise_construct, std::forward_as_tuple(VK_QUEUE_COMPUTE_BIT), std::forward_as_tuple(m_LogicalDevice, cQ));

	if (tQ != ~0)
		m_Queues.emplace(std::piecewise_construct, std::forward_as_tuple(VK_QUEUE_TRANSFER_BIT), std::forward_as_tuple(m_LogicalDevice, tQ));
}

RenderScope::RenderScope()
{
	_createVulkanInstance();
	_createDevice();

	_createMemoryAllocator();
	_collectQueues();

	m_Cache = std::make_shared<GVkResourceCache>(m_LogicalDevice);
}

RenderScope::~RenderScope()
{
	m_Cache->_flush();
	m_Queues.clear();

	vmaDestroyAllocator(m_Allocator);
	vkDestroyDevice(m_LogicalDevice, VK_NULL_HANDLE);

	if (m_DebugMessenger)
	{
		PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VkInstance, "vkDestroyDebugUtilsMessengerEXT");
		func(m_VkInstance, m_DebugMessenger, VK_NULL_HANDLE);
	}

	vkDestroyInstance(m_VkInstance, nullptr);
}

void RenderScope::IncrementFlightIndex()
{
	m_ResourceIndex = (m_ResourceIndex + 1) % FramesInFlight;
}