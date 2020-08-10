CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Excludes
  # ...
)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(injecter
  ${PRIVATE_SOURCES}
)

target_include_directories(injecter
  PUBLIC
   ${CMAKE_BINARY_DIR}
  PRIVATE
   ${PRIVATE_INCLUDES}
   ${MINHOOK_INCLUDE_DIR}
   ${DISCORD_INCLUDE_DIR}
   ${Boost_INCLUDE_DIRS}
)

target_link_libraries(injecter
  PRIVATE
   boost
)

add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-DNOMINMAX)

add_dependencies(injecter boost)

install(TARGETS injecter DESTINATION "${CMAKE_INSTALL_PREFIX}")