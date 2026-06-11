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

# include(FetchContent) FetchContent_Declare( cglm SOURCE_DIR
# ${CMAKE_CURRENT_SOURCE_DIR}/extern/cglm/ # SOURCE_SUBDIR "" #
# DOWNLOAD_EXTRACT_TIMESTAMP ON URL
# https://github.com/recp/cglm/archive/refs/tags/v0.9.6.tar.gz)
# FetchContent_MakeAvailable(cglm)

if(ANDROID)
  add_library(${APP} SHARED)

  set(APP_RESOURCES_SOURCE_DIR "")
  set(APP_RESOURCES_SHADERS_SOURCE_DIR "${APP_RESOURCES_SOURCE_DIR}/shaders")
  set(APP_ASSETS_SOURCE_DIR "")
else()
  add_executable(${APP})

  set(APP_RESOURCES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/main/res")
  set(APP_RESOURCES_SHADERS_SOURCE_DIR "${APP_RESOURCES_SOURCE_DIR}/shaders")
  set(APP_ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/main/assets")

  find_program(GLSLANG_VALIDATOR_EXE NAMES glslangValidator)
endif()

set_target_properties(
  ${APP}
  PROPERTIES
    LANGUAGES "C CXX"
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME
    "$<$<PLATFORM_ID:Android>:main>$<$<NOT:$<PLATFORM_ID:Android>>:app>"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/)

target_sources(
  ${APP}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main/main.c
          ${CMAKE_CURRENT_SOURCE_DIR}/src/main/vma_impl.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/extern/rxi_log/src/log.c
  PRIVATE FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/
          ${CMAKE_CURRENT_SOURCE_DIR}/extern/rxi_log/src/
          ${cglm_SOURCE_DIR}/include)

target_compile_definitions(
  ${APP} PRIVATE LOG_USE_COLOR APP_USE_VK APP_NAME="${APP}"
                 APP_ASSETS_SOURCE_DIR="${APP_ASSETS_SOURCE_DIR}/")
target_compile_options(
  ${APP}
  PRIVATE -Wno-deprecated-declarations
          -Wno-nullability-completeness # ignore clang warning on vma library
)
target_link_libraries(
  ${APP} PRIVATE m SDL3::SDL3 volk::volk_headers VulkanMemoryAllocator
                 $<$<PLATFORM_ID:Android>:log>)

add_custom_target(
  ${APP}-run
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMAND cmake --build build -j --target ${APP} --config $<CONFIG>
  COMMAND ./bin/${PROJECT_NAME}/$<CONFIG>/app
  USES_TERMINAL VERBATIM COMMAND_EXPAND_LISTS
  COMMENT "${PROJECT_NAME} Building app config $<CONFIG>..."
  #
)

# SHADER COMPILER
file(GLOB APP_SHADER_SOURCES CONFIGURE_DEPENDS
     "${APP_RESOURCES_SHADERS_SOURCE_DIR}/*.*")

foreach(APP_SHADER_SOURCE_FILE IN LISTS APP_SHADER_SOURCES)
  # set(APP_SHADER_SOURCE_FILE
  # "${APP_RESOURCES_SHADERS_SOURCE_DIR}/main.[vert|frag]")
  # set(APP_SHADER_OUT_FILE
  # "${APP_RESOURCES_SHADERS_SOURCE_DIR}/main.[vert|frag].spv")

  get_filename_component(APP_SHADER_FILE_NAME "${APP_SHADER_SOURCE_FILE}" NAME)
  message(STATUS "[${APP}] Detected shaders: ${APP_SHADER_SOURCE_FILE}")
  set(APP_SHADER_OUT_FILE
      "${APP_ASSETS_SOURCE_DIR}/shaders/${APP_SHADER_FILE_NAME}.spv")

  add_custom_command(
    OUTPUT ${APP_SHADER_OUT_FILE} #
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_ASSETS_SOURCE_DIR}/shaders/
    COMMAND ${GLSLANG_VALIDATOR_EXE} -V ${APP_SHADER_SOURCE_FILE} -o
            ${APP_SHADER_OUT_FILE}
    DEPENDS ${APP_SHADER_SOURCE_FILE}
    COMMENT "[${APP}] Compiling shader: ${APP_SHADER_OUT_FILE}..."
    VERBATIM)

  list(APPEND APP_SHADER_TARGETS "${APP_SHADER_OUT_FILE}")
endforeach()

add_custom_target(${APP}-compile-shaders ALL DEPENDS ${APP_SHADER_TARGETS})

add_dependencies(${APP}-run ${APP}-compile-shaders)
