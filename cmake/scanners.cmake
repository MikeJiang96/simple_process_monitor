file(GLOB_RECURSE ALL_SRC_FILES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/include/*.h
     ${CMAKE_SOURCE_DIR}/src/*.c
     ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/test/*.cpp)

add_custom_target(format "clang-format" -i ${ALL_SRC_FILES} COMMENT "Formatting source code...")
