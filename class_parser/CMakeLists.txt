
include(cotire)

set(CMAKE_CXX_STANDARD 14)

set(CLANG_HOME $ENV{CLANG_HOME})

link_directories(${CLANG_HOME}/lib  )
include_directories(${V8_INCLUDE_DIR})
include_directories(${CLANG_HOME}/include)

execute_process(COMMAND ${CLANG_HOME}/bin/llvm-config --ldflags --libs --system-libs
        OUTPUT_VARIABLE llvm_config_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
string(REPLACE "\n" " " llvm_config_output ${llvm_config_output})

if(UNIX AND NOT APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -stdlib=libstdc++")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -stdlib=libc++")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -pedantic" )
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${llvm_config_output}")

add_library(class_parser_plugin SHARED parsed_method.cpp class_parser.cpp ast_action.cpp annotations.cpp helper_functions.cpp
        annotations.cpp class_handler.cpp bidirectional_bindings.cpp ast_consumer.cpp wrapped_class.cpp
        )

cotire(class_parser_plugin)


# Not actually sure which of these are actually needed, but some of them are
# Do not link against LLVMSupport - causes: CommandLine Error: Option 'view-background' registered more than once!
target_link_libraries(class_parser_plugin clangFrontend clangSerialization clangASTMatchers clangSema
        clangAnalysis clangRewriteFrontend clangEdit clangDriver clangLex
        LLVMSymbolize LLVMDebugInfoPDB LLVMScalarOpts LLVMInstCombine LLVMInstrumentation LLVMBitWriter
        LLVMTransformUtils LLVMExecutionEngine LLVMTarget LLVMAnalysis LLVMRuntimeDyld
        LLVMObject LLVMBitReader LLVMCore clangParse clangAST
        clangBasic clangParse)

target_link_libraries(class_parser_plugin fmt)


add_custom_target(class_parser_driver
        ${CLANG_HOME}/bin/clang++ -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS
        -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -D_GNU_SOURCE
        ${PROJECT_BINARY_DIR}/class_parser/sample.cpp
        -fsyntax-only -std=c++14 -O0
        -Xclang -load
        -Xclang ${CMAKE_BINARY_DIR}/class_parser/$<TARGET_FILE_NAME:class_parser_plugin>
        -Xclang -plugin
        -Xclang v8toolkit-generate-bindings
    )

add_custom_target(copy_driver_resources)
add_custom_command(TARGET copy_driver_resources POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/sample2.h
            ${CMAKE_CURRENT_SOURCE_DIR}/class_parser.h
        ${PROJECT_BINARY_DIR}/class_parser/)

add_dependencies(class_parser_driver class_parser_plugin copy_driver_resources)

install(TARGETS class_parser_plugin DESTINATION ${INSTALL_LIB_DIR})
