# ------------------------------ DOCUMENTATION ---------------------------------
# Release build
# -------------------------
# $ meson setup --wipe <builddir> # wipe the build artifacts (like object files)
# $ meson setup <builddir> --reconfigure --buildtype=release # reconfigure the build directory for release build
# $ meson compile -C <builddir> # compile the project
#
# Debug build
# -------------------------
# $ meson setup --wipe <buildir> # wipe the build artifacts (like object files)
# $ meson setup <builddir> --reconfigure --buildtype=debug # reconfigure the build directory for debug build
# $ meson compile -C <builddir> # compile the project
#
# Install
# --------------------------
# $ meson install -C <builddir> # install the static library
#
# -------------------------------- PROJECT CONFIGS -----------------------------

project('BuildMaster', 'c', 'cpp',
  version : '1.0.0',
  meson_version: '>=1.1',
  default_options : [
    'warning_level=3',
    'buildtype=debug',
    'c_std=c17',
    'cpp_std=c++20'
  ]
)

# Release Build Defines
release_defines = [
  '-DBUILD_MASTER_RELEASE' 
]

# Debug Build Defines
debug_defines = [
  '-DBUILD_MASTER_DEBUG'
]

# Include directories
inc = include_directories('include/build_master')

# Main executable source
sources = files('source/main.cpp')

# Whether to install executable?
is_install_executable = true

dependencies = [ dependency('cli11') ]

# ------------------------------ INTERNALS ---------------------------------------

# Compiler configuration
add_project_arguments('-m64', language : 'c')
add_project_arguments('-m64', language : 'cpp')
# Linker configuration
add_project_link_arguments('-m64', language : 'c')
add_project_link_arguments('-m64', language : 'cpp')

# Build type specific defines
build_mode_defines = []
if get_option('buildtype') == 'release'
  add_project_arguments(release_defines, language : 'c')
  add_project_arguments(release_defines, language : 'cpp')
  build_mode_defines += release_defines
else
  add_project_arguments(debug_defines, language : 'c')
  add_project_arguments(debug_defines, language : 'cpp')
  build_mode_defines += debug_defines
endif

# Main executable
exe = executable('build_master',
  sources,
  dependencies: dependencies,
  include_directories : inc,
  install : is_install_executable
)
