
CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Excludes
  # ...
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(patchdll SHARED
  ${PRIVATE_SOURCES}
)

set_property(TARGET patchdll PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(patchdll
  PRIVATE
    Zydis
    minhook
    fmt
)

target_include_directories(patchdll
  PUBLIC
    ${CMAKE_BINARY_DIR}
  PRIVATE
    ${PRIVATE_INCLUDES}
    ${MINHOOK_INCLUDE_DIR}
)

add_dependencies(patchdll Zydis minhook fmt)

add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

install(TARGETS patchdll DESTINATION "${CMAKE_INSTALL_PREFIX}")
