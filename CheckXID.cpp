#include <iostream>
#include <windows.h>
#include "nvml.h" // You will need the nvml.h header file

// Define function pointers for dynamically loading nvml.dll
typedef nvmlReturn_t (*nvmlInit_t)(void);
typedef nvmlReturn_t (*nvmlShutdown_t)(void);
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t*);
typedef nvmlReturn_t (*nvmlDeviceGetLastXid_t)(nvmlDevice_t, unsigned int*, unsigned long long*);

int main() {
    std::cout << "--- NVIDIA XID Error Detection Utility ---" << std::endl;

    // 1. Load nvml.dll
    HMODULE nvml_dll = LoadLibrary(TEXT("nvml.dll"));
    if (!nvml_dll) {
        std::cerr << "[ERROR] Failed to load nvml.dll. Please ensure NVIDIA drivers are installed correctly." << std::endl;
        return 1;
    }
    std::cout << "[INFO] nvml.dll loaded successfully." << std::endl;

    // 2. Get function pointers from the DLL
    nvmlInit_t nvmlInit = (nvmlInit_t)GetProcAddress(nvml_dll, "nvmlInit_v2");
    nvmlShutdown_t nvmlShutdown = (nvmlShutdown_t)GetProcAddress(nvml_dll, "nvmlShutdown");
    nvmlDeviceGetHandleByIndex_t nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    nvmlDeviceGetLastXid_t nvmlDeviceGetLastXid = (nvmlDeviceGetLastXid_t)GetProcAddress(nvml_dll, "nvmlDeviceGetLastXid");

    if (!nvmlInit || !nvmlShutdown || !nvmlDeviceGetHandleByIndex || !nvmlDeviceGetLastXid) {
        std::cerr << "[ERROR] Failed to find required NVML functions in nvml.dll." << std::endl;
        FreeLibrary(nvml_dll);
        return 1;
    }

    // 3. Initialize NVML
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        std::cerr << "[ERROR] Failed to initialize NVML. Error code: " << result << std::endl;
        FreeLibrary(nvml_dll);
        return 1;
    }
    std::cout << "[INFO] NVML initialized." << std::endl;

    // 4. Get handle for the first GPU (device 0)
    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        std::cerr << "[ERROR] Failed to get handle for GPU device 0. Error code: " << result << std::endl;
        nvmlShutdown();
        FreeLibrary(nvml_dll);
        return 1;
    }
    std::cout << "[INFO] Acquired handle for GPU 0." << std::endl;

    // 5. Check for the last XID error
    unsigned int xid_type;
    unsigned long long xid_timestamp;
    result = nvmlDeviceGetLastXid(device, &xid_type, &xid_timestamp);

    std::cout << "\n--- DETECTION RESULT ---" << std::endl;
    if (result == NVML_SUCCESS) {
        std::cout << "[CRITICAL] XID ERROR DETECTED!" << std::endl;
        std::cout << "  > Error Code (XID): " << xid_type << std::endl;
        std::cout << "  > Timestamp: " << xid_timestamp << std::endl;
        std::cout << "  > An unrecoverable driver or hardware error has occurred." << std::endl;
    } else if (result == NVML_ERROR_NOT_FOUND) {
        std::cout << "[OK] No XID errors found on the device since last driver load." << std::endl;
        std::cout << "  > The GPU is operating normally." << std::endl;
    } else {
        std::cerr << "[WARNING] An error occurred while querying for XID errors. Code: " << result << std::endl;
        std::cout << "  > Could not determine the GPU error state." << std::endl;
    }
    std::cout << "------------------------\n" << std::endl;

    // 6. Clean up
    nvmlShutdown();
    FreeLibrary(nvml_dll);
    std::cout << "[INFO] NVML shut down and resources released." << std::endl;

    system("pause"); // Wait for user to press a key
    return 0;
}