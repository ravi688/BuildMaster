# Build Master
![image](https://github.com/user-attachments/assets/e36ca337-a89b-4f49-85df-aa5976f8920a)

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT) <br>
[![Linux Release Mode Unit Tests](https://github.com/ravi688/BuildMaster/actions/workflows/release_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/release_unit_test.yml)
[![Linux Debug Mode Unit Tests](https://github.com/ravi688/BuildMaster/actions/workflows/debug_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/debug_unit_test.yml) <br>
[![Windows Debug Mode Unit Test](https://github.com/ravi688/BuildMaster/actions/workflows/windows_debug_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/windows_debug_unit_test.yml)
[![Windows Release Mode Unit Test](https://github.com/ravi688/BuildMaster/actions/workflows/windows_release_unit_test.yml/badge.svg)](https://github.com/ravi688/BuildMaster/actions/workflows/windows_release_unit_test.yml)

## Objective
Build Master is a wrapper around meson to make C/C++ project configuration convenient, and faster with a minimalistic json configuration file.
It can do the same thing in just 70 lines of json that the bare meson does in 240 lines of meson.build! <br> <br>
I started this project to integrate frequently occuring **Build Configurations**, **Packaging Workflow**, and **Generating CI Workflows for GitLab and GitHub** across all my projects into just one system called `Build Master`. In fact, the goal of this project is to make it so good that any type of C/C++ software can be **Built**, **Tested**, **Packaged** and **Deployed** in just a few commands.

## Building and Installing
Get the source code
```
$ git clone https://github.com/ravi688/BuildMaster.git
```
Setup build configuration
```
$ meson setup build --buildtype=release
```
Now compile and install
```
$ meson install -C build
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
You may also pass `--directory=<path to a directory>` into the above `init` command to specify where to put the `build_master.json` file. <br>
By default `init` command would create `main.c` file in `source` directory, if you want C++ file then pass `--create-cpp` flag.
### Configuring the project
```
$ build_master meson setup build --buildtype=release
```
The above command would invoke `meson setup build --buildtype=release` which will create a `build` directory and configure the project build config as release
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
### Optional variables
| Variable | Type | Description
-----------|------|--------------
| `is_install` | bool | It must be used in the `"target"` context, by default its value is `"false"` for executable target and `"true"` for library target 
| `description` | string | It must be used inside the root (at the same level as `"project_name"`), or inside a `"target"` context, by default its value is `"Description is not provided"` and if it is not provided for a target then its value (for the target) is inherited from the project's description.
| `sources` | list of string(s) | it is optional for targets
| `defines`, `use_defines`, `debug_defines`, `release_defines`, `build_defines` | list of string(s) | All are optional
| `install_header_dirs` | list of strings(s) | It is optional if you do not intend to install any libraries
| `windows_link_args` | list of string(s) | It is optional, but useful for specifying window specific libraries
| `linux_link_args` | list of string(s) | It is optioanl, but useful for specifying linux specific libraries
| `darwin_link_args` | list of string(s) | It is optional, but useful for specifying darwin (macOS) sepcific libraries
| `dependencies` | list of string(s) | It is optional, it can be used in project scope or in target context both, if used in target context then the specified dependencies will only be applicable to that target only

### Have you got benefited with my work?
<a href="https://www.buymeacoffee.com/raviprakashsingh" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>
