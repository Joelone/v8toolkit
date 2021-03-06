project ("Class Parser Test")
set(CMAKE_VERBOSE_MAKEFILE on)



add_executable(test-class-parser test.cpp ${CLASS_PARSER_SOURCES})
target_include_directories(test-class-parser SYSTEM PRIVATE
        ${gtest_SOURCE_DIR}/include
        ${gmock_SOURCE_DIR}/include
        ${CLANG_HOME}/include
    )


# Uncomment next line to print debugging info from xl::template library
target_compile_definitions(test-class-parser PRIVATE -DXL_TEMPLATE_LOG_ENABLE -DTEST_MODE)


find_package(fmt REQUIRED)
target_link_libraries(test-class-parser fmt gmock gmock_main)

# Can't have RTTI because the clang libs don't have it which means
target_compile_options(test-class-parser PRIVATE -fno-rtti)

target_link_libraries(test-class-parser clangFrontend clangSerialization clangASTMatchers clangSema
        clangAnalysis clangRewriteFrontend clangEdit clangDriver clangLex
        LLVMSymbolize LLVMDebugInfoPDB LLVMScalarOpts LLVMInstCombine LLVMInstrumentation LLVMBitWriter
        LLVMTransformUtils LLVMExecutionEngine LLVMTarget LLVMAnalysis LLVMRuntimeDyld
        LLVMObject LLVMBitReader LLVMCore clangParse clangAST
        clangBasic clangParse clangTooling c++experimental pcre)


add_custom_target(copy_test_resources)
add_custom_command(TARGET copy_test_resources PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../class_parser.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include/v8toolkit/wrapped_class_base.h
            ${PROJECT_BINARY_DIR}/
        )

add_dependencies(test-class-parser gmock gtest copy_test_resources)


