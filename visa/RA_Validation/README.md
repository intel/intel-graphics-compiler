# vISA RA Validation Tool
### Daniel Man - August 2023

## Description
This is a GTReplay Tool for detecting bad register assignments. It stores the byte values of every virtual variable, updated after each Def by inspecting the physical register state. At a Use, it performs a byte-by-byte comparison between the stored byte values and the byte values currently in the physical register state, and reports mismatch errors. A mismatch indicates that a Def to one virtual variable clobbered the values of another virtual variable, thus revealing the existence of a bad assignment with overlapping live ranges.

## Setup
### Verification Driver
The tool depends on virtual variable names to operate, but these are not actually kept in the compiler except in Debug mode or offline vISA executable. As a result, we need to add VV names to Release-Internal mode. Achieve this with the following steps:

1. Open the file **Source/visa/BuildIRImpl.cpp**
2. Search for the method with signature:

   ```cpp
   const char *IR_Builder::getNameString(size_t size, const char* format, ...)
   ```
3. At the macro on the first line of this method, add Release-Internal to the conditions by changing the line from

   ```cpp
   #if defined(_DEBUG) || !defined(DLL_MODE)
   ```
   to
   ```cpp
   #if defined(_DEBUG) || !defined(DLL_MODE) || defined(_RELEASE_INTERNAL)
   ```
4. Build a verification driver with this change. Download the release internal driver at **Artifacts/Windows/Driver-Release-Internal-64-bit.7z** and install it onto a DG2 machine by following [this](https://wiki.ith.intel.com/pages/viewpage.action?pageId=2689630455#SystemProvisioning-ManualProvisioning) guide (I recommend installing the certificate manually)

### GTPin
1. Download the latest version of the GTPin package [here](https://www.intel.com/content/www/us/en/developer/articles/tool/gtpin.html)
2. Copy and paste the **ra_validate.cpp** file (found in the same directory as this README) into the following directory of the package: **Profilers/GTReplay/examples**
3. Modify the **CMakeLists.txt** file in that same directory (Profilers/GTReplay/examples) so that we build this custom validation tool. Make the following changes:
    - Change this line from
      ```
      set ( GTREPLAY_EXAMPLES icount imix ops mem toggle bbltrace)
      ```
      to
      ```
      set ( GTREPLAY_EXAMPLES icount imix ops mem toggle bbltrace ra_validate)
      ```

    - Add this line
      ```
      add_library( ra_validate SHARED ra_validate.cpp)
      ```
      after the other ```add_library``` lines

    - Add this line
      ```
      set_property(TARGET ra_validate PROPERTY POSITION_INDEPENDENT_CODE ON)
      ```
      after the other ```set_property``` lines

4. From within that same directory (Profilers/GTReplay/examples), follow the steps for "How to build GTReplay Tool" from the GTReplay Getting Started documentation found [here](https://software.intel.com/sites/landingpage/gtpin/_g_t_r_e_p_l_a_y__g_e_t_t_i_n_g__s_t_a_r_t_e_d.html). As an example, here are the commands I used:
    - mkdir build
    - cd build
    - cmake .. -G "Visual Studio 17 2022" -DARCH=intel64 -DGTPIN_KIT=C:\GTPin\Profilers
    - cmake --build . --config Debug --target install
    - (Note that these commands are dependent on your Visual Studio version and GTPin package location)
5. Add the following directories to **PATH** (modify environment variable)
    - Profilers\Lib\intel64
    - Profilers\GTReplay\intel64

## Usage

### Enabling Metadata Emission
1. Open Registry Editor and navigate to **Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Intel\IGFX\IGC**
2. Add a new DWORD key named **ShaderDumpEnable** and set its value data to **1**
3. Add a new DWORD key named **ShaderDumpEnableRAMetadata** and set its value data to **1**
4. Add a new String key named **VISAOptions** and set its value data to **-dumpgenoffset**

### GTPin Gentrace
Run the following commands to collect the runtime trace of your application using GTPin Gentrace

```
Profilers/Bin/gtpin.exe -t gentrace.dll --phase 1 -- <application executable path> <application args>
```
```
Profilers/Bin/gtpin.exe -t gentrace.dll --phase 2 --enqueue <kernel number> -- <application executable path> <application args>
```
(Here, kernel number refers to which kernel of the application the user is attempting to run validation on)

Registry keys do not need to be set after Gentrace phase 2 has completed

### GTReplay Validation Tool
1. Verify that the runtime trace for the kernel was successfully generated. The traces can be found under **GTPIN_PROFILE_GENTRACE1/Session_Final/...**
2. Verify that the shader dump directory was generated (by default, at path **C:\Intel\IGC**)
3. Recommended to run, as a sanity check, the **icount** sample tool included with GTReplay. Instructions found [here](https://software.intel.com/sites/landingpage/gtpin/_i_c_o_u_n_t__t_o_o_l.html)
4. Execute the following command to run the validation tool:
```
gtreplay.exe -t Profilers/GTReplay/examples/build/Lib/intel64/ra_validate.dll --shaderDumpDir "<path to shader dump directory>" -- <path to runtime trace>
```
Here is an example of what this command looked like for my test:
```
gtreplay.exe -t Profilers/GTReplay/examples/build/Lib/intel64/ra_validate.dll --shaderDumpDir "C:\Intel\IGC\rodinia-nw.exe_7468" -- GTPIN_PROFILE_GENTRACE1/Session_Final/CS_asmaed32e0839ffdb4e_simd16_aed32e0839ffdb4e_0/device_0__bus_3__bus_0__enqueue_0
```