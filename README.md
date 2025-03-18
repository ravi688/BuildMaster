# Build Master
![image](https://github.com/user-attachments/assets/e36ca337-a89b-4f49-85df-aa5976f8920a)
[![Latest Release](https://img.shields.io/github/v/release/ravi688/BuildMaster?label=latest&logo=github)](https://github.com/ravi688/BuildMaster/releases/latest)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
![GitHub stars](https://img.shields.io/github/stars/ravi688/BuildMaster?style=social)
![GitHub stars](https://img.shields.io/github/forks/ravi688/BuildMaster?style=social)
[![GitHub stars](https://img.shields.io/github/issues/ravi688/BuildMaster?style=social)](https://github.com/ravi688/BuildMaster/issues)
<br>
| OS | Build Status
|----|--------
| Linux | [![Linux Release Mode Unit Tests](https://github.com/ravi688/BuildMaster/actions/workflows/release_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/release_unit_test.yml)
| Linux | [![Linux Debug Mode Unit Tests](https://github.com/ravi688/BuildMaster/actions/workflows/debug_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/debug_unit_test.yml) <br>
| Windows | [![Windows Debug Mode Unit Test](https://github.com/ravi688/BuildMaster/actions/workflows/windows_debug_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/windows_debug_unit_test.yml)
| Windows | [![Windows Release Mode Unit Test](https://github.com/ravi688/BuildMaster/actions/workflows/windows_release_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/windows_release_unit_test.yml)
| FreeBSD | TODO
| MacOS | TODO


## Objective
Build Master is a wrapper around meson to make C/C++ project configuration convenient, and faster with a minimalistic json configuration file.
It can do the same thing in just 70 lines of json that the bare meson does in 240 lines of meson.build! <br> <br>
I started this project to integrate frequently occuring **Build Configurations**, **Packaging Workflow**, and **Generating CI Workflows for GitLab and GitHub** across all my projects into just one system called `Build Master`. In fact, the goal of this project is to make it so good that any type of C/C++ software can be **Built**, **Tested**, **Packaged** and **Deployed** in just a few commands.
> [!Note]
> Build Master uses a fork of the original meson project <br>
> Link to the fork: https://github.com/ravi688/meson.git

## Current Capabilities
- [x] Only for C and C++ based projects
- [x] Multiple Targets
- [x] Simple and Minimalistic Json based build script (`build_master.json`)
- [x] Tested with projects with upto 5 to 7 dependencies (with diamond dependencies) and multiple targets can be built easily
- [ ] Single command packaging and bundling for popular package managers (apt, dpkg, pacman, rpm), and window installers (.msi and .exe)
- [ ] Github/Gitlab workflow generation for Building the project in different environments on different platforms each
- [ ] Distributed project building for large number of source files to compile, C++ takes more time to compile so this feature is particularly beneficial for C++ projects

## Building and Installing
Get the source code
```
$ git clone https://github.com/ravi688/BuildMaster.git
```
#### Install modified meson
For Msys2 (windows)
```
$ ./install_meson.sh
```
For Linux
```
$ sudo chmod +x ./install_meson.sh
$ sudo ./install_meson.sh
```
#### Install build_master
Setup build configuration
```
$ meson setup build --buildtype=release
```
Now compile and install
```
$ meson install -C build --skip-subprojects
```

## Resolving Build Errors
If you get certificate verify errors then execute the following commands in msys2 (mingw64)
```
$ pacman -S mingw-w64-x86_64-ca-certificates
$ update-ca-trust
```
If you're in linux then execute the following commands
```
$ sudo apt-get install ca-certifactes
$ update-ca-trust
```

## Usage
### Initializing a project
```
$ build_master init --name "BufferLib" --canonical_name "bufferlib"
```
The above commanad would create a file `build_master.json` containing default project configuration and `meson.build` for meson build system.
By default `init` command would create `main.c` file in `source` directory, if you want C++ file then pass `--create-cpp` flag.
> [!Tip]
> You may also pass `--directory=<path to a directory>` into the above `init` sub-command to specify where to initialize the new project, all the project files will be created relative to that directory. <br>
> There are two ways you can specify the `--directory`: <br>
> `$ build_master init --directory=path/to/my/directory --name "BufferLib" --canonical_name "bufferlib"`, OR <br>
> `$ build_master --directory init --name "BufferLib" --canonical_name "bufferlib"` <br>
> In the latter case, the `--directory` is supplied to the `build_master` command, it acts as a fallback if `init` sub-command hasn't given the `--directory` value.

### Configuring the project
```
$ build_master meson setup build --buildtype=release
```
The above command would invoke `meson setup build --buildtype=release` which will create a `build` directory and configure the project build config as release
> [!Tip]
> You can also pass `--directory=path/to/dir` as `build_master --directory=path/to/dir meson setup build --buildtype=release`

### Compiling the project
```
$ build_master meson compile -C build
```
The above command would compile the project
### Regenerating the meson.build if build_master.json changes
```
$ build_master --update-meson-build
```
The above command checks if the current `meson.build` file is out of date and then regenerates overwriting the existing one. <br>
OR
```
$ build_master --update-meson-build --force
```
The above command does forcibly regenerates the existing `meson.build` even if it is upto date with `build_master.json`.
### Displaying version of the build_master
```
$ build_master --version
```
The above command prints version and build mode information on stdout.

## More Info
- If `build_master.json` already exists then running `build_master init --name "BufferLib"` would produce an error/warning, to overwrite the existing file pass `--force` flag
- If `source` directory already exists then running `build_master init <args>` would skip creating the directory and a `main.c` or `main.cpp` file. You'd have to remove the existing directory to automatically create `source/main.cpp` or `source/main.c`, OR you may also manually create.
> [!WARNING]
> If `build_master.json` is modified then running `build_master` command would regenerate the `meson.build` file overwritting the existing one 

## Example Build Master json file
> [!NOTE]
> build_master.json also allows C++ style single line comments
```cpp
{
    // This project name could be anything (it is not used as filename for any of the build artifacts)
    "project_name" : "BufferLib",
    // This is similar to the project name but it is used for file names, so it must form a valid file name
    "canonical_name" : "bufferlib",
    "description" : "A memory buffer library",
    // Dependencies of this project (for all of its targets)
    "dependencies" : [ "calltrace", "meshgen" ],
    "defines" : [ "-DUSE_VULKAN_DRIVER" ],
    "release_defines": [ "-DBUF_RELEASE" ],
    "debug_defines": [ "-DBUF_DEBUG" ],
    // All of the subdirectories inside include will be installed into the environment's include directory
    // Typically you use should structure your project to have 'include/bufferlib' subdir. 
    "install_header_dirs" : [ "include/bufferlib" ],
    // Header inclusion directory common to all targets
    "include_dirs" : [ "include" ],
    // Extra linker arguments when compiling on Windows platform
    // It will be passed for each targets
    "windows_link_args" : [ "-lws2 " ],
    "targets" :
    [
        {
            "name" : "bufferlib_static",
            "is_static_library" : true,
            "description" : "Static Library for BufferLib",
            // These defines will be enabled when building bufferlib_static target
            "build_defines" : [ "-DBUF_BUILD_STATIC_LIBRARY" ],
            // These defines will be added as extra_cflags in pkg-config files
            "use_defines" : [ "-DBUF_USE_STATIC_LIBRARY" ]
        }, 
        {
            "name" : "bufferlib_shared",
            "is_shared_library" : true,
            // These defines will be enabled when building bufferlib_shared target
            "build_defines" : [ "-DBUF_BUILD_SHARED_LIBRARY" ],
            // These defines will be added as extra_cflags in pkg-config files
            "use_defines" : [ "-DBUF_USE_SHARED_LIBRARY" ]
        },
        {
            "name" : "client",
            "is_executable" : true,
            "defines" : [ "-DCLIENT_BUILD" ],
            // These source files are specific (in addition) to client target
            "sources" : [ "source/main.client.c" ],
            // Extra linker arguments specific for this target
            "windows_link_args" : [ "-lgdi" ]
        },
        {
            "name" : "server",
            "is_executable" : true,
            "defines" : [ "-DSERVER_BUILD" ],
            // These source files are specific (in addition) to server target
            "sources" : [ "source/main.server.c" ]
        },
        {
            "name" : "main",
            "is_executable" : true,
            "is_install" : true,
            // These source files are specific (in addition) to main target
            "sources" : [ "source/main.c" ]
        },
        {
            "name" : "main_test",
            "is_executable" : true,
            "dependencies" : [ "catch2-with-main" ],
            "is_install" : false,
            "sources" : [ "source/main_test.c" ]
        }
    ],
    // These source files will be compiled in each target as common source files
    "sources" :
    [
        "source/buffer.c",
        "source/buffer_test.c"
    ]
}
```
## Example Build Master json file (Taken from [TempSys](https://github.com/ravi688/TemplateSystem))
```cpp
{
    "project_name": "TemplateSystem",
    "canonical_name": "tempsys",
    "description" : "Macro based Templates in C",
    "install_headers" : [
        {
            "files" : [ "include/template_system.h" ]
        }
    ],
    "include_dirs": "include",
    "targets": [
        {
            "name": "tempsys",
            "is_header_only_library" : true
        }
    ]
}
```
## Example Build Master json file (Taken from [SKVMOIP](https://github.com/ravi688/SKVMOIP))
```cpp
{
    "project_name" : "SKVMOIP",
    "canonical_name" : "skvmoip",
    "description" : "Scalable KVM Over IP",
    "dependencies" : [ "common", "playvk", "bufferlib" ],
    "vars" :
    {
        "cuda_path" : "run_command(find_program('python'), '-c', 'import os; print(os.environ[\"CUDA_PATH\"])', check : false).stdout().strip()",
        "cuda_includes" : "cuda_path + '/include'",
        "cuda_libs_path" : "cuda_path + '/lib/x64'",
        "vulkan_sdk_path" : "run_command(find_program('python'), '-c', 'import os; print(os.environ[\"VK_SDK_PATH\"])', check : false).stdout().strip()",
        "vulkan_libs_path" : "vulkan_sdk_path + '/Lib/'",
        "gtk3_cflags" : "run_command('pkg-config', 'gtk+-3.0', '--cflags', check : false).stdout().strip().split(' ')",
        "gtk3_libs" : "run_command('pkg-config', 'gtk+-3.0', '--libs', check : false).stdout().strip().split(' ')",
        "gui_sources" : [
            "source/GUI/AddUI.cpp",
            "source/GUI/MachineUI.cpp",
            "source/GUI/MainUI.cpp"
        ],
        "server_target_sources" : [
            "source/Encoder.cpp",
            "source/HDMIEncodeNetStream.cpp",
            "source/Win32/Win32ImagingDevice.cpp"
        ],
        "client_target_sources" : [
            "source/third_party/NvDecoder.cpp",
            "source/Decoder.cpp",
            "source/Window.cpp",
            "source/Win32/Win32DrawSurface.cpp",
            "source/Win32/Win32RawInput.cpp",
            "source/Win32/Win32Window.cpp",
            "source/HDMIDecoderNetStream.cpp",
            "source/KMNetStream.cpp",
            "source/HIDUsageID.cpp",
            "source/RDPSession.cpp",
            "source/PresentEngine.cpp",
            "source/MachineData.cpp",
            "source/FIFOPool.cpp",
            "source/Event.cpp",
            "source/Network/NetworkPacket.cpp",
            "source/ErrorHandling.cpp"
        ]
    },
    "defines" : [ 
        "-DUSE_PERSISTENT_SETTINGS",
        "-DUSE_VULKAN_PRESENTATION",
        "-DUSE_VULKAN_FOR_COLOR_SPACE_CONVERSION",
        "-DUSE_DIRECT_FRAME_DATA_COPY"
    ],
    "release_defines": [ "-DSKVMOIP_RELEASE" ],
    "debug_defines": [ "-DSKVMOIP_DEBUG" ],
    "install_header_dirs" : [ "include/SKVMOIP" ],
    "include_dirs" : [ "include", "external-dependencies" ],
    "windows_link_args" : [
        "-lws2_32",
        "-lole32",
        "-loleaut32",
        "-lmfreadwrite",
        "-lmfplat",
        "-lmf",
        "-lmfuuid",
        "-lgdi32",
        "-lwmcodecdspuuid"
    ],
    "targets" :
    [
        {
            "name" : "server",
            "is_executable" : true,
            "windows_link_args" : [
                "link_dir: './external-dependencies/x264'", "-lx264"
            ],
            "defines" : ["-DBUILD_SERVER"],
            "sources": [ "$server_target_sources", "source/main.server.cpp" ]
        },
        {
            "name" : "client",
            "is_executable" : true,
            "dependencies" : ["gtk+-3.0", "vulkanheaders"],
            "include_dirs" : [
                "./external-dependencies/NvidiaCodec/include/",
                "$cuda_includes"
            ],
            "windows_link_args" : [
                "link_dir: $cuda_libs_path", "-l:cuda.lib",
                "link_dir: './external-dependencies/NvidiaCodec/'", "-l:nvcuvid.lib",
                "link_dir: $vulkan_libs_path", "-lvulkan-1"                
            ],
            "defines" : ["-DBUILD_CLIENT"],
            "sources" : [ "$client_target_sources", "$gui_sources", "source/main.client.cpp" ]
        },
        // The guitest target was just for experiment, it won't build!
        {
            "name" : "guitest",
            "is_executable" : true,
            "defines" : ["-DBUILD_GUITEST"],
            "dependencies" : ["gtk+-3.0"],
            "sources" : [
                "source/main.guitest.cpp",
                "$gui_sources"
            ]
        },
        // The main target was just for experiment, it won't build!
        {
            "name" : "main",
            "is_executable" : true,
            "defines" : ["-DBUILD_TEST", "-DBUILD_CLIENT"],
            "dependencies" : ["gtk+-3.0", "vulkanheaders"],
            "include_dirs" : [
                "./external-dependencies/NvidiaCodec/include/",
                "$cuda_includes"
            ],
            "windows_link_args" : [
                "link_dir: './external-dependencies/x264'", "-lx264",
                "link_dir: './external-dependencies/NvidiaCodec/'", "-l:nvcuvid.lib",
                "link_dir: $cuda_libs_path", "-l:cuda.lib",
                "link_dir: $vulkan_libs_path", "-lvulkan-1"                
            ],
            "sources" : [
                "source/main.cpp",
                "$client_target_sources",
                "$gui_sources"
            ]
        }
    ],
    "sources" : [
        "source/Network/NetworkSocket.cpp",
        "source/Network/NetworkAsyncQueueSocket.cpp",
        "source/VideoSourceStream.cpp",
        "source/Win32/Win32.cpp"
    ]
}
```
### Optional variables
| Variable | Type | Description
-----------|------|--------------
| `is_install` | bool | It must be used in the `"target"` context, by default its value is `"false"` for executable target and `"true"` for library target 
| `description` | string | It must be used inside the root (at the same level as `"project_name"`), or inside a `"target"` context, by default its value is `"Description is not provided"` and if it is not provided for a target then its value (for the target) is inherited from the project's description.
| `sources` | list of string(s) | it is optional for targets
| `defines`, `use_defines`, `debug_defines`, `release_defines`, `build_defines` | list of string(s) | All are optional
| `install_header_dirs` | list of strings(s) | It is optional if you do not intend to install any libraries
| `install_headers` | list of json values containing `files` list of strings and optional `subdir` | It is optional if you do not intend to install any specific header files
| `windows_link_args` | list of string(s) | It is optional, but useful for specifying window specific libraries
| `linux_link_args` | list of string(s) | It is optioanl, but useful for specifying linux specific libraries
| `darwin_link_args` | list of string(s) | It is optional, but useful for specifying darwin (macOS) sepcific libraries
| `dependencies` | list of string(s) | It is optional, it can be used in project scope or in target context both, if used in target context then the specified dependencies will only be applicable to that target only
| `include_dirs` | list of string(s) | It is optional in the context of target, When specified in a target context then these include directories are only used for that target and won't affect other targets
| `sub_dirs` | list of string(s) | It is optional, and can only be used in header only library target context. It specifies the list of sub-directories containing header file which needs to be exported via pkg-config package file.

### Pre Configure Script Execution
Different projects have different dependencies, and some require execution of complex commands to build and install such dependencies.
Often initial procedures are documented in the wikis of the respective projects.
But this can be automated by creating a bash script and hooking it to the generated meson.build, so that whenever the project is (re)configured, the script will be run first to download or build/install complex dependencies.
#### Usage Example
```cpp
{
    "canonical_name": "bufferlib",
    "include_dirs": "include",
    "project_name": "BufferLib",
    "pre_configure_hook" : "download_packages.sh", // <---- here
    "targets": [
        {
            "is_executable": true,
            "name": "bufferlib",
            "sources": [
                "source/main.c"
            ]
        }
    ]
}
```
In the `download_packages.sh` script:
```sh
#! /usr/bin/bash
meson wrap install glfw
meson wrap install vulkan-headers
```
> [!Note]
> If the hooked script returns non-zero code then an error about it will be printed. <br>
> If the script returns without any errors and returns zero, the success will be printed.

> [!Warning]
> The pre-config hook script executs everytime `meson setup` command is executed via `build_master`.
> Therefore, make sure the result/behaviour of the script is idempotent, i.e. if the script is executed multiple times then it should lead to the same result as if it ran only once.

### Targets
The following boolean config vars can only be specified in `target` context in `build_master.json`, And only one of them can exist in a target. That means all of them are mutually exclusive. 
| Target Type | Description
|-------------|-------------
| `is_static_library` | If set `true`, then the target builds as static library
| `is_shared_library` | If set `true`, then the target builds as shared library
| `is_header_only_library` | If set `true`, then the target builds as header only library, that means no binaries, just header file installations
| `is_executable` | If set `true`, then the target builds as executable

### Have you got benefited with my work?
<a href="https://www.buymeacoffee.com/raviprakashsingh" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>
