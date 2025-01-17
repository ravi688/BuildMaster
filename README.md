# BuildMaster
BuildMaster is a wrapper around meson to make project configuration convenient, and faster.

## Usage
Following command must be run in the root directory of the project and where `build_master.json` file also exists:
```
$ build_master <meson args>
```
OR
```
$ build_master -f build_master.json <meson args>
```

## Build Master config file
```json
{
	"project_name" : "BufferLib",
	"conanical_name" : "bufferlib",
	"dependencies" : [ "calltrace" ],
	"release_defines": [ "-DBUF_RELEASE" ],
	"debug_defines": [ "-DBUF_DEBUG" ],
	"header_dir" : "include",
	"targets" :
	{
		"bufferlib_static" :
		{
			"is_static_library" : true,
			"build_defines" : [ "-DBUF_BUILD_STATIC_LIBRARY" ],
			"use_defines" : [ "-DBUF_USE_STATIC_LIBRARY" ]
		},
		"bufferlib_shared" : 
		{
			"is_shared_library" : true,
			"build_defines" : [ "-DBUF_BUILD_SHARED_LIBRARY" ],
			"use_defines" : [ "-DBUF_USE_SHARED_LIBRARY" ]
		},
		"client" :
		{
			"is_executable" : true,
			"defines" : [ "-DCLIENT_BUILD" ],
			"sources" : [ "source/main.client.c" ]
		},
		"server" :
		{
			"is_executable" : true,
			"defines" : [ "-DSERVER_BUILD" ],
			"sources" : [ "source/main.server.c" ]
		},
		"main" :
		{
			"is_executable" : true,
			"sources" : [ "source/main.c" ]
		}
	}
}
```
### Optional variables
- `is_install`  | boolean | must be used in `"target"` context
