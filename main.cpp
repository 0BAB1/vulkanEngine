#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class HelloTriangleApplication {
public:
    void run() {
        //Run routine
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Attributes
    GLFWwindow* window;                                                             // GLFW Window handler
    VkInstance instance;                                                            // VULKAN Instance handler
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;                               // Iniatte the picked GPU (to null for now)
    VkDevice device;                                                                // Logical device

    void initWindow(){
        //window init routine
        glfwInit();                                                                 // Init GLFW

        //Windows hints for creation
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);                               // Prevent default openGl context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);                                 // Prevent RESZABLE

        window = glfwCreateWindow(WIDTH,                                            //Create the window and assign it to the app's handler
                                  HEIGHT,
                                  "Vulkan",
                                  nullptr,
                                  nullptr);
    }

    void initVulkan() {
        //Vulkan init routine
        createInstance();                                                           //Call vulkan instance creator
        pickPhysicalDevice();                                                       //Pick a physical device (Graphic card)
        createLogicalDevice();
    }

    void createLogicalDevice(){
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);             // re-find the queues fimilies specified in QueueFamilyIndices Struct

        VkDeviceQueueCreateInfo queueCreateInfo{};                                  // Initiate an empty info struct for the queue creation
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;         // Inform our info structure type
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();          // Give it the idex of the queue familly we need (specified in findQueueFamilies)
        queueCreateInfo.queueCount = 1;                                             // Create only 1 queue (we can use threads for multpiple calculation stuff)
        float queuePriority = 1.0f;                                                 // Between 0 and 1
        queueCreateInfo.pQueuePriorities = &queuePriority;                          // Give it priority 1, mandatory even though we only have 1 queue.

        VkPhysicalDeviceFeatures deviceFeatures{};                                  // Specify physical defvicefeatures (leave it to default for now)

        VkDeviceCreateInfo createInfo{};                                            // Make the create infos for the logical device
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;                            // Give it pointers to other info structure we just made
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
    }

    void pickPhysicalDevice(){
        uint32_t deviceCount = 0;                                                   
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);                // enumerate the phisical devices

        if (deviceCount == 0){
            throw std::runtime_error("No GPU found with Vumkan suppport");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);                         // Create our physical devices array
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());         // Populate the array

        for (const auto& device : devices) {                                        // Pick first suitable GPU
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if(physicalDevice == VK_NULL_HANDLE){                                       // Error beacause no suitable GPU
            throw std::runtime_error("faild to find a suitable GPU !");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device){
        // Check for pysical device suitablility using queries
        // and checking the differents properties of the device
        VkPhysicalDeviceProperties deviceProperties;          
        VkPhysicalDeviceFeatures deviceFeatures;                      
        vkGetPhysicalDeviceProperties(device, &deviceProperties);                   // Query the device's properties
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);                       // Query for the device features

        QueueFamilyIndices indices = findQueueFamilies(device);

        return deviceProperties.deviceType == (                                     // Check for required stuff and return
                VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) 
            && deviceFeatures.geometryShader
            && indices.isComplete();
    }

    struct QueueFamilyIndices {                                                     // To gathers the required queue families in a sturcture
        std::optional<uint32_t> graphicsFamily;                                     // Optional that hold NO value if not set

        bool isComplete(){
            return graphicsFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device){                  // Actually find those queue families
        // Logic to find graphics queue familly
        QueueFamilyIndices indices;

        uint32_t queueFamiliyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device,
                                                &queueFamiliyCount,
                                                nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliyCount);       // Just the usual ...
        vkGetPhysicalDeviceQueueFamilyProperties(device,
                                                &queueFamiliyCount,
                                                queueFamilies.data());

        int i = 0;
        for(const auto& queueFamily : queueFamilies){
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
                indices.graphicsFamily = i;
            }

            if(indices.isComplete()){
                break;
            }

            i++;
        }
        
        return indices;
    }

    void createInstance(){
        VkApplicationInfo appInfo{};                                                // Create appInfos object (optional)
        appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;           // As usual, struct type
        appInfo.pApplicationName    = "Test Triangle";
        appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName         = "No Engine";
        appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion          = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};                                          // Create info object to create instance
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;                  // As usual, struct type
        createInfo.pApplicationInfo = &appInfo;                                     // Give it our bonus infos

        // GLFW Extension support & checks
        uint32_t glfwExtensionCount = 0;                                            // Basic counter
        const char** glfwExtensions;                                                // String array to hold extension infos
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);    // assign infos and pass it the counter

        //add extensions to create Info
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS){
            throw std::runtime_error("Echec de la création de l'instance !");
        }


        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties( nullptr,
                                                &extensionCount,
                                                extensions.data());

        std::cout << "Extensions :\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
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