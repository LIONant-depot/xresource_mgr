include(${CMAKE_BINARY_DIR}/_deps/xcmake_tools/Common.cmake)

# Fetch dependencies
FetchAndPopulate("https://github.com/LIONant-depot/xresource_guid.git" "main")

DefineInterfaceComponent(xresource_mgr "dependencies/xcore"
  "source/xresource_mgr.h"
  "source/xresource_mgr.cpp"
  "Readme.md"
)
