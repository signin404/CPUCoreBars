// Tester/Tester.cpp
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <evntprov.h> // For generating ETW events
#include "nvml.h"     // NVML header

#pragma comment(lib, "advapi32.lib") // For ETW functions
#pragma comment(lib, "nvml.lib")     // For NVML functions

// This GUID MUST match the one being listened for in your plugin.
// {C2D578F2-5948-4527-9598-345A212C4ECE}
static const GUID WHEA_PROVIDER_GUID =
{ 0xc2d578f2, 0x5948, 0x4527, { 0x95, 0x98, 0x34, 0x5a, 0x21, 0x2c, 0x4e, 0xce } };

// Function to inject a fake WHEA event via ETW
void TestWheaInjection() {
    REGHANDLE providerHandle = 0;
    ULONG status = EventRegister(&WHEA_PROVIDER_GUID, NULL, NULL, &providerHandle);

    if (status != ERROR_SUCCESS) {
        printf("ETW EventRegister failed with error %lu. Is the program running as Administrator?\n", status);
        return;
    }

    printf("Injecting fake WHEA ETW event...\n");
    // Fire a simple event. The plugin only checks for the provider, not the content.
    status = EventWrite(providerHandle, 0, 0, NULL);

    if (status == ERROR_SUCCESS) {
        printf("SUCCESS: Event sent. Check the plugin icon; it should turn RED.\n");
    } else {
        printf("FAIL: EventWrite failed with error %lu.\n", status);
    }

    EventUnregister(providerHandle);
}

// Function to query GPU ECC errors via NVML
void TestNvmlQuery() {
    printf("Querying NVML for ECC status...\n");

    HMODULE nvml_dll = LoadLibrary(L"nvml.dll");
    if (!nvml_dll) {
        printf("FAIL: Could not load nvml.dll. Is the NVIDIA driver installed?\n");
        return;
    }

    // Define function pointer types
    typedef nvmlReturn_t(*nvmlInit_t)(void);
    typedef nvmlReturn_t(*nvmlShutdown_t)(void);
    typedef nvmlReturn_t(*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t*);
    typedef nvmlReturn_t(*nvmlDeviceGetTotalEccErrors_t)(nvmlDevice_t, nvmlMemoryErrorType_t, nvmlEccCounterType_t, unsigned long long*);

    // Get function pointers
    nvmlInit_t pfn_nvmlInit = (nvmlInit_t)GetProcAddress(nvml_dll, "nvmlInit_v2");
    nvmlShutdown_t pfn_nvmlShutdown = (nvmlShutdown_t)GetProcAddress(nvml_dll, "nvmlShutdown");
    nvmlDeviceGetHandleByIndex_t pfn_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    nvmlDeviceGetTotalEccErrors_t pfn_nvmlDeviceGetTotalEccErrors = (nvmlDeviceGetTotalEccErrors_t)GetProcAddress(nvml_dll, "nvmlDeviceGetTotalEccErrors");

    if (!pfn_nvmlInit || !pfn_nvmlShutdown || !pfn_nvmlDeviceGetHandleByIndex || !pfn_nvmlDeviceGetTotalEccErrors) {
        printf("FAIL: Couldn't find required NVML functions in nvml.dll.\n");
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

    unsigned long long corrected = 0, uncorrected = 0;
    // Query volatile (since boot) ECC counts.
    pfn_nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_CORRECTED, NVML_VOLATILE_ECC, &corrected);
    pfn_nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_UNCORRECTED, NVML_VOLATILE_ECC, &uncorrected);

    printf("SUCCESS: Query complete.\n");
    printf("  > Volatile Corrected ECC Errors: %llu\n", corrected);
    printf("  > Volatile Uncorrected ECC Errors: %llu\n", uncorrected);
    printf("\nNOTE: If these values are > 0, the plugin icon should be ORANGE or RED.\n");
    printf("This test does not generate errors, only reads existing ones.\n");

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
        printf("  [1] Inject fake WHEA ETW Event (Requires Administrator)\n");
        printf("  [2] Query current GPU ECC Errors\n");
        printf("  [0] Exit\n");
        printf("Your choice: ");
        
        choice = _getch();
        printf("%c\n\n", choice);

        switch (choice) {
        case '1':
            TestWheaInjection();
            break;
        case '2':
            TestNvmlQuery();
            break;
        case '0':
            printf("Exiting.\n");
            break;
        default:
            printf("Invalid choice. Please try again.\n");
            break;
        }
    } while (choice != '0');

    return 0;
}```

---

### 2. Visual Studio Project Files for Tester

**Instructions:**
1.  Inside the `Tester` folder, create a new file named `Tester.vcxproj` and paste the XML content below.
2.  Create another file named `Tester.vcxproj.filters` and paste its XML content.

#### `Tester/Tester.vcxproj`
```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Label="Globals">
    <VCProjectVersion>17.0</VCProjectVersion>
    <Keyword>Win32Proj</Keyword>
    <ProjectGuid>{a1b2c3d4-e5f6-7890-1234-567890abcdef}</ProjectGuid>
    <RootNamespace>Tester</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <PlatformToolset>v143</PlatformToolset>
    <WholeProgramOptimization>true</WholeProgramOptimization>
    <CharacterSet>Unicode</CharacterSet>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <WarningLevel>Level3</WarningLevel>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <SDLCheck>true</SDLCheck>
      <PreprocessorDefinitions>NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <ConformanceMode>true</ConformanceMode>
      <!-- Assumes nvml.h is in a folder named 'nvml' at the solution level -->
      <AdditionalIncludeDirectories>$(SolutionDir)nvml;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <!-- Assumes nvml.lib is in a folder named 'nvml' at the solution level -->
      <AdditionalLibraryDirectories>$(SolutionDir)nvml;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="Tester.cpp" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
