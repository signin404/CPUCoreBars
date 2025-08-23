// Tester/Tester.cpp - Final Corrected Version

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <evntprov.h>
#include "nvml.h"

#pragma comment(lib, "advapi32.lib")

static const GUID WHEA_PROVIDER_GUID =
{ 0xc2d578f2, 0x5948, 0x4527, { 0x95, 0x98, 0x34, 0x5a, 0x21, 0x2c, 0x4e, 0xce } };

void TestWheaInjection() {
    REGHANDLE providerHandle = 0;
    ULONG status = EventRegister(&WHEA_PROVIDER_GUID, NULL, NULL, &providerHandle);
    if (status != ERROR_SUCCESS) {
        printf("ETW EventRegister failed with error %lu. Is the program running as Administrator?\n", status);
        return;
    }
    EVENT_DESCRIPTOR eventDescriptor = {0};
    eventDescriptor.Id = 1; eventDescriptor.Level = 4;
    printf("Injecting fake WHEA ETW event...\n");
    status = EventWrite(providerHandle, &eventDescriptor, 0, NULL);
    if (status == ERROR_SUCCESS) {
        printf("SUCCESS: Event sent. After the next check interval (up to 60s), the plugin icon should turn RED.\n");
    } else {
        printf("FAIL: EventWrite failed with error %lu.\n", status);
    }
    EventUnregister(providerHandle);
}

void TestNvmlQuery() {
    printf("Querying NVML for comprehensive GPU status...\n");

    HMODULE nvml_dll = LoadLibrary(L"nvml.dll");
    if (!nvml_dll) {
        printf("FAIL: Could not load nvml.dll. Is the NVIDIA driver installed?\n");
        return;
    }

    typedef nvmlReturn_t(*nvmlInit_t)(void);
    typedef nvmlReturn_t(*nvmlShutdown_t)(void);
    typedef nvmlReturn_t(*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t*);
    typedef nvmlReturn_t(*nvmlDeviceGetTotalEccErrors_t)(nvmlDevice_t, nvmlMemoryErrorType_t, nvmlEccCounterType_t, unsigned long long*);
    typedef nvmlReturn_t(*nvmlDeviceGetRetiredPages_t)(nvmlDevice_t, nvmlPageRetirementCause_t, unsigned int*, unsigned long long*);
    typedef nvmlReturn_t(*nvmlDeviceGetLastXid_t)(nvmlDevice_t, unsigned int*, unsigned long long*);

    nvmlInit_t pfn_nvmlInit = (nvmlInit_t)GetProcAddress(nvml_dll, "nvmlInit_v2");
    nvmlShutdown_t pfn_nvmlShutdown = (nvmlShutdown_t)GetProcAddress(nvml_dll, "nvmlShutdown");
    nvmlDeviceGetHandleByIndex_t pfn_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    nvmlDeviceGetTotalEccErrors_t pfn_nvmlDeviceGetTotalEccErrors = (nvmlDeviceGetTotalEccErrors_t)GetProcAddress(nvml_dll, "nvmlDeviceGetTotalEccErrors");
    nvmlDeviceGetRetiredPages_t pfn_nvmlDeviceGetRetiredPages = (nvmlDeviceGetRetiredPages_t)GetProcAddress(nvml_dll, "nvmlDeviceGetRetiredPages");
    nvmlDeviceGetLastXid_t pfn_nvmlDeviceGetLastXid = (nvmlDeviceGetLastXid_t)GetProcAddress(nvml_dll, "nvmlDeviceGetLastXid");

    if (!pfn_nvmlInit || !pfn_nvmlShutdown || !pfn_nvmlDeviceGetHandleByIndex) {
        printf("FAIL: Couldn't find essential NVML functions. Your driver is likely very old.\n");
        FreeLibrary(nvml_dll);
        return;
    }

    if (pfn_nvmlInit() != NVML_SUCCESS) {
        printf("FAIL: nvmlInit() failed.\n");
        FreeLibrary(nvml_dll);
        return;
    }

    nvmlDevice_t device;
    if (pfn_nvmlDeviceGetHandleByIndex(0, &device) != NVML_SUCCESS) {
        printf("FAIL: Could not get handle to GPU 0.\n");
        pfn_nvmlShutdown();
        FreeLibrary(nvml_dll);
        return;
    }

    printf("SUCCESS: Query complete.\n");
    printf("--------------------------------------------------\n");
    printf("Driver Feature Support Report:\n");

    if (pfn_nvmlDeviceGetTotalEccErrors) {
        unsigned long long corrected = 0, uncorrected = 0;
        nvmlReturn_t ecc_status = pfn_nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_CORRECTED, NVML_VOLATILE_ECC, &corrected);
        if (ecc_status == NVML_SUCCESS) {
            pfn_nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_UNCORRECTED, NVML_VOLATILE_ECC, &uncorrected);
            printf("  > Volatile Corrected ECC Errors  : %llu\n", corrected);
            printf("  > Volatile Uncorrected ECC Errors: %llu\n", uncorrected);
        } else if (ecc_status == NVML_ERROR_NOT_SUPPORTED) {
            printf("  > ECC Error Reporting            : Not Supported on this GPU\n");
        } else {
            printf("  > ECC Error Reporting            : Error querying (code: %d)\n", ecc_status);
        }
    } else {
        printf("  > ECC Error Reporting            : Function not found in nvml.dll (Driver too old)\n");
    }

    if (pfn_nvmlDeviceGetRetiredPages) {
        unsigned int retired_page_count = 0;
        // FIX: Corrected the typo in the enum name below
        nvmlReturn_t retired_status = pfn_nvmlDeviceGetRetiredPages(device, NVML_PAGE_RETIREMENT_CAUSE_MULTIPLE_SINGLE_BIT_ECC_ERRORS, &retired_page_count, NULL);
         if (retired_status == NVML_SUCCESS) {
            printf("  > Retired Pages (Remapped Rows)  : %u\n", retired_page_count);
        } else if (retired_status == NVML_ERROR_NOT_SUPPORTED) {
            printf("  > Retired Pages Reporting        : Not Supported on this GPU\n");
        } else {
            printf("  > Retired Pages Reporting        : Error querying (code: %d)\n", retired_status);
        }
    } else {
        printf("  > Retired Pages Reporting        : Function not found in nvml.dll (Driver too old)\n");
    }

    if (pfn_nvmlDeviceGetLastXid) {
        unsigned int last_xid = 0;
        pfn_nvmlDeviceGetLastXid(device, &last_xid, NULL);
        printf("  > Last Xid Error Code          : %u\n", last_xid);
    } else {
        printf("  > Last Xid Error Reporting       : Function not found in nvml.dll (Driver too old)\n");
    }
    printf("--------------------------------------------------\n");

    pfn_nvmlShutdown();
    FreeLibrary(nvml_dll);
}

int main() {
    printf("======================================\n");
    printf(" CPUCoreBars Plugin Tester\n");
    printf("======================================\n");
    printf("IMPORTANT: Run TrafficMonitor with the CPUCoreBars plugin loaded BEFORE using this tool.\n\n");
    char choice;
    do {
        printf("\nSelect a test to run:\n");
        printf("  [1] Inject fake WHEA Event (Requires Administrator)\n");
        printf("  [2] Query comprehensive GPU status from NVML\n");
        printf("  [0] Exit\n");
        printf("Your choice: ");
        choice = _getch();
        printf("%c\n\n", choice);
        switch (choice) {
        case '1': TestWheaInjection(); break;
        case '2': TestNvmlQuery(); break;
        case '0': printf("Exiting.\n"); break;
        default: printf("Invalid choice. Please try again.\n"); break;
        }
    } while (choice != '0');
    return 0;
}