# BuildMaster
BuildMaster is a wrapper around meson to make C/C++ project configuration convenient, and faster.

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

## Usage
```
$ build_master init --name "BufferLib"
```
The above commanad would create a file `build_master.json` containing default project configuration and `meson.build` for meson build system
```
$ build_master --meson setup build --buildtype=release
```
The above command would invoke `meson setup build --buildtype=release` which will create a `build` directory and configure the project build config as release
```
$ build_master --meson compile -C build
```
The above command would compile the project

## More Info
- If `build_master.json` already exists then running `build_master init --name "BufferLib"` would produce an error/warning, to overwrite the existing file pass '--force' flag
- **IMPORTANT** - If `build_master.json` is modified then running `build_master` command would regenerate the `meson.build` file overwritting the existing one 

## Build Master config file
```json
{
	// This project name could be anything (it is not used as filename for any of the build artifacts)
	"project_name" : "BufferLib",
	// This is similar to the project name but it is used for file names, so it must form a valid file name
	"conanical_name" : "bufferlib",
	// Dependencies of this project (for all of its targets)
	"dependencies" : [ "calltrace" ],
	"release_defines": [ "-DBUF_RELEASE" ],
	"debug_defines": [ "-DBUF_DEBUG" ],
	// All of the subdirectories inside include will be installed into the environment's include directory
	// Typically you use should structure your project to have 'include/bufferlib' subdir. 
	"header_dir" : "include",
	"targets" :
	{
		"bufferlib_static" :
		{
			"is_static_library" : true,
			// These defines will be enabled when building bufferlib_static target
			"build_defines" : [ "-DBUF_BUILD_STATIC_LIBRARY" ],
			// These defines will be added as extra_cflags in pkg-config files
			"use_defines" : [ "-DBUF_USE_STATIC_LIBRARY" ]
		},
		"bufferlib_shared" : 
		{
			"is_shared_library" : true,
			// These defines will be enabled when building bufferlib_shared target
			"build_defines" : [ "-DBUF_BUILD_SHARED_LIBRARY" ],
			// These defines will be added as extra_cflags in pkg-config files
			"use_defines" : [ "-DBUF_USE_SHARED_LIBRARY" ]
		},
		"client" :
		{
			"is_executable" : true,
			"defines" : [ "-DCLIENT_BUILD" ],
			// These source files are specific (in addition) to client target
			"sources" : [ "source/main.client.c" ]
		},
		"server" :
		{
			"is_executable" : true,
			"defines" : [ "-DSERVER_BUILD" ],
			// These source files are specific (in addition) to server target
			"sources" : [ "source/main.server.c" ]
		},
		"main" :
		{
			"is_executable" : true,
			// These source files are specific (in addition) to main target
			"sources" : [ "source/main.c" ]
		}
	},
	// These source files will be compiled in each target as common source files
	"sources" :
	[
		"source/buffer.c",
		"source/buffer_test.c"
	]
}
```
### Optional variables
- `is_install`  | boolean | must be used in `"target"` context
