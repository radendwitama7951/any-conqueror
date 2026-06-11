set(APP ${PROJECT_NAME})

find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(
  sdl3
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.8.tar.gz)
FetchContent_MakeAvailable(sdl3)

include(FetchContent)
FetchContent_Declare(
  volk
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/zeux/volk/archive/refs/tags/1.4.350.tar.gz)
FetchContent_MakeAvailable(volk)

include(FetchContent)
FetchContent_Declare(
  vma
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.4.0.tar.gz
)
FetchContent_MakeAvailable(vma)

include(FetchContent)
FetchContent_Declare(
  glm
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/g-truc/glm/archive/refs/tags/1.0.3.tar.gz)
FetchContent_MakeAvailable(glm)

include(FetchContent)
FetchContent_Declare(
  tinyobjloader
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/tinyobjloader/tinyobjloader/archive/refs/tags/v1.0.6.tar.gz
)
FetchContent_MakeAvailable(tinyobjloader)

include(FetchContent)
FetchContent_Declare(
  slang_linux_x86_64
  SUBDIR_SOURCE ""
  URL_HASH
    SHA256=e1993c89fe6eb9fce148672ac2b633af1578e413c41b730be446dd8be251376b
  URL https://github.com/shader-slang/slang/releases/download/v2026.10.2/slang-2026.10.2-linux-x86_64.tar.gz
)
FetchContent_MakeAvailable(slang_linux_x86_64)
message(STATUS "[${APP}] SLANG_SOURCE_DIR ${slang_linux_x86_64_SOURCE_DIR}")
find_package(slang REQUIRED CONFIG HINTS
             ${SLANG_LINUX_X86_64_SOURCE_DIR}/lib/cmake/slang/)

include(FetchContent)
FetchContent_Declare(
  ktx_software
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/KhronosGroup/KTX-Software/releases/download/v4.4.2/KTX-Software-4.4.2-Linux-x86_64.tar.bz2
)
FetchContent_MakeAvailable(ktx_software)
message(STATUS "[${APP}] KTX_SOFTWARE_SOURCE_DIR ${ktx_software_SOURCE_DIR}")
find_package(Ktx REQUIRED CONFIG HINTS
             ${ktx_software_SOURCE_DIR}/lib/cmake/ktx/)

if(ANDROID)
  add_library(${APP} SHARED)
  set(APP_OUTPUT_NAME "main")
else()
  add_executable(${APP})
  set(APP_OUTPUT_NAME "app")
endif()

set(APP_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(APP_ASSETS_DIR_PATH
    "$<IF:$<PLATFORM_ID:Android>,,${CMAKE_CURRENT_SOURCE_DIR}/src/main/assets/>"
)

target_sources(
  ${APP}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/main.cpp
  PRIVATE FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/
          ${ktx_software_SOURCE_DIR}/include)

set_target_properties(
  ${APP}
  PROPERTIES LANGUAGES "C CXX"
             C_STANDARD 17
             C_STANDARD_REQUIRED ON
             C_EXTENSIONS OFF
             CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
             CXX_SCAN_FOR_MODULES OFF
             OUTPUT_NAME ${APP_OUTPUT_NAME}
             RUNTIME_OUTPUT_DIRECTORY ${APP_RUNTIME_OUTPUT_DIRECTORY})

target_compile_definitions(
  ${APP}
  PRIVATE VK_NO_PROTOTYPES #
          APP_NAME="${PROJECT_NAME}"
          APP_ASSETS_DIR_PATH="${APP_ASSETS_DIR_PATH}")

target_compile_options(
  ${APP}
  PRIVATE -Wno-deprecated-declarations
          -Wno-nullability-completeness # ignore clang warning on vma library
)

target_link_libraries(
  ${APP}
  PRIVATE SDL3
          volk::volk_headers
          VulkanMemoryAllocator
          slang::slang
          ${ktx_software_SOURCE_DIR}/lib/libktx.so
          tinyobjloader
          glm)

add_custom_target(
  ${APP}-run
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${APP}
  COMMAND $<TARGET_FILE:${APP}>
  COMMENT "[${APP}] Running ${APP} on $<CONFIG> mode..."
  USES_TERMINAL VERBATIM)

if(WIN32)
  target_compile_definitions(${APP} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
  target_compile_definitions(${APP} PRIVATE VK_USE_PLATFORM_MACOS_MVK)
  # Link against Metal framework for MoltenVK
  target_link_libraries(${APP} PRIVATE "-framework Metal" "-framework MetalKit"
                                       "-framework Cocoa")
elseif(LINUX)
  # Linux-specific configuration
  target_compile_definitions(${APP} PRIVATE VK_USE_PLATFORM_XCB_KHR)
  # You might need to link against XCB or Wayland libraries
endif()
