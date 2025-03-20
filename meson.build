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

# Get the value of DESTDIR environment variable (set by the runtime_hook.py in build_master_meson)
python_result = run_command(find_program('python'), '-c', 'import os; print(os.environ["DESTDIR"])', check : false)
dest_dir = ''
if python_result.returncode() == 0
  str = python_result.stdout()
  # Unix
  if str.startswith('/')
    dest_dir = str.replace(';', ':').split(':')[0]
  # Windows
  else
    dest_dir = str.split(';')[0]
  endif
  dest_dir = dest_dir.strip()
endif

meson_build_template_install_dir = join_paths(dest_dir, 'share', 'build_master')

defines = [
  '-DBUILDMASTER_VERSION_MAJOR=' + meson.project_version().split('.')[0],
  '-DBUILDMASTER_VERSION_MINOR=' + meson.project_version().split('.')[1],
  '-DBUILDMASTER_VERSION_MICRO=' + meson.project_version().split('.')[2], 
  '-DMESON_BUILD_TEMPLATE_PATH="' + join_paths(meson_build_template_install_dir, 'meson.build.template') + '"'
]

# Include directories
inc = include_directories()

# Main executable source
sources = files('source/main.cpp')

dependencies = [ 
  dependency('cli11'), 
  dependency('nlohmann_json'), 
  dependency('invoke'),
  dependency('common'),
  dependency('spdlog')
]

# ------------------------------ INTERNALS ---------------------------------------

extra_link_args = []
os_name = host_machine.system()
if os_name == 'windows'
	extra_link_args += '-lws2_32'
endif

# Compiler configuration
add_project_arguments('-m64', language : 'c')
add_project_arguments('-m64', language : 'cpp')
# Linker configuration
add_project_link_arguments('-m64', extra_link_args, language : 'c')
add_project_link_arguments('-m64', extra_link_args, language : 'cpp')

# Build type specific defines
if get_option('buildtype') == 'release'
  add_project_arguments(release_defines + defines, language : 'c')
  add_project_arguments(release_defines + defines, language : 'cpp')
else
  add_project_arguments(debug_defines + defines, language : 'c')
  add_project_arguments(debug_defines + defines, language : 'cpp')
endif

# Main executable
executable('build_master',
  sources,
  dependencies: dependencies,
  include_directories : inc,
  install : true
)

install_data('meson.build.template', install_dir : meson_build_template_install_dir)
