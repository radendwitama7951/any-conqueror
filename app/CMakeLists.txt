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
  ktx_linux_x86_64
  URL https://github.com/KhronosGroup/KTX-Software/releases/download/v4.4.2/KTX-Software-4.4.2-Linux-x86_64.tar.bz2
  URL_HASH SHA256=a8781bad05f9624edbf910b7f258cd0a4ba7d3e63b49ecc0a0ab440bf6a0a245)
FetchContent_MakeAvailable(ktx_linux_x86_64)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/extern/rxi_log/)
  FetchContent_Declare(
    rxi_log
    DOWNLOAD_EXTRACT_TIMESTAMP ON #
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/rxi_log/ #
    SUBDIR_SOURCE ""
    URL https://github.com/rxi/log.c/archive/refs/heads/master.zip #
  )
endif()

if(ANDROID)
  add_library(${APP} SHARED)

  set(APP_RESOURCES_SOURCE_DIR "")
  set(APP_ASSETS_SOURCE_DIR "")
else()
  add_executable(${APP})

  set(APP_RESOURCES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/main/res")
  set(APP_ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/main/assets")

  find_program(GLSLANG_VALIDATOR_EXE NAMES glslangValidator)
  find_program(TOKTX_EXE NAMES toktx)
  find_program(KTXINFO_EXE NAMES ktxinfo)
endif()

set(APP_RESOURCES_SHADERS_SOURCE_DIR "${APP_RESOURCES_SOURCE_DIR}/shaders")
set(APP_RESOURCES_TEXTURES_SOURCE_DIR "${APP_RESOURCES_SOURCE_DIR}/drawables")

set(APP_ASSETS_TEXTURES_SOURCE_DIR "${APP_ASSETS_SOURCE_DIR}/drawables")
set(APP_ASSETS_SHADERS_SOURCE_DIR "${APP_ASSETS_SOURCE_DIR}/shaders")

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
          $<$<PLATFORM_ID:Linux>:${ktx_linux_x86_64_SOURCE_DIR}/include>
)

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
                 $<$<PLATFORM_ID:Android>:log>
          $<$<PLATFORM_ID:Linux>:${ktx_linux_x86_64_SOURCE_DIR}/lib/libktx.so>
)

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

# ASSETS MANAGER
set(APP_RESOURCES_TEXTURES_DIRS "1920x1080" "1024x1024")
set(APP_ASSETS_TEXTURES_OUTPUT_NAMES "textures1920x1080.ktx2"
                                     "textures1024x1024.ktx2")

list(LENGTH APP_RESOURCES_TEXTURES_DIRS COUNT)
math(EXPR COUNT_MINUS_ONE "${COUNT} - 1")

# TEXTURES BUNDLER
foreach(i RANGE ${COUNT_MINUS_ONE})
  list(GET APP_RESOURCES_TEXTURES_DIRS ${i} TEXTURES_DIR)
  list(GET APP_ASSETS_TEXTURES_OUTPUT_NAMES ${i} OUTPUT_NAME)

  set(SRC_DIR ${APP_RESOURCES_TEXTURES_SOURCE_DIR}/${TEXTURES_DIR})
  set(OUT_FILE ${APP_ASSETS_TEXTURES_SOURCE_DIR}/${OUTPUT_NAME})

  file(GLOB PNG_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.png)
  list(LENGTH PNG_FILES TEXTURES_COUNT)

  # message(STATUS "[${APP}] TEXTURES SRC: ${SRC_DIR}")

  # --target_type array # --layers ${TEXTURES_COUNT} #
  add_custom_command(
    OUTPUT ${OUT_FILE} #
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${APP_ASSETS_TEXTURES_SOURCE_DIR}/
    COMMAND ${TOKTX_EXE} --t2 --genmipmap  --encode uastc  --layers ${TEXTURES_COUNT} #
            -- "${OUT_FILE}" ${PNG_FILES} #
    COMMAND ${KTXINFO_EXE} ${OUT_FILE}
    DEPENDS ${PNG_FILES}
    COMMENT "[${APP}] Bundling textures from ${TEX_DIR}/ into ${OUT_FILE}"
    VERBATIM)

  list(APPEND KTX_TARGETS "${OUT_FILE}")

endforeach()

add_custom_target(${APP}-bundle-textures ALL DEPENDS ${KTX_TARGETS})
add_dependencies(${APP}-run ${APP}-bundle-textures)
