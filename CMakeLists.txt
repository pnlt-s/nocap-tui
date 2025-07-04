cmake_minimum_required (VERSION 3.31)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("nocapTUI")

include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
)

FetchContent_MakeAvailable(ftxui)

set(CMAKE_BUILD_TYPE "Release")

add_executable (nocapTUI "nocapTUI.cpp" "nocapTUI.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET nocapTUI PROPERTY CXX_STANDARD 23)
endif()

target_link_libraries(nocapTUI
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

if (EMSCRIPTEN)
  string(APPEND CMAKE_CXX_FLAGS " -s USE_PTHREADS")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -s ASYNCIFY")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -s PROXY_TO_PTHREAD")

  foreach(file "index.html" "run_webassembly.py")
    configure_file("src/${file}" ${file})
  endforeach(file)
endif()

string(APPEND CMAKE_CXX_FLAGS " /UMBCS /D_UNICODE /DUNICODE")
