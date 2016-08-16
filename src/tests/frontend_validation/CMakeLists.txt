
if (NOT GTEST_DIR)
message(FATAL_ERROR "Must define prefix install dir for GoogleTest via -DGTEST_DIR=")
endif()

include_directories( ${GTEST_DIR}/include )
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GTEST_DIR}/lib")
include_directories( ${CMAKE_SOURCE_DIR}/src/tests/frontend_validation )

set(frontendtestfiles gtest_main.cc )
function(add_unit_test test_name)
set(frontendtestfiles "${frontendtestfiles};${test_name}.cc" PARENT_SCOPE)
add_executable(${test_name} ${test_name}.cc gtest_main.cc)
target_link_libraries(${test_name} ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} "")
set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
  FAIL_REGULAR_EXPRESSION "FAILED;should be deleted but never is"
  PASS_REGULAR_EXPRESSION "PASSED")
endfunction()

add_unit_test(test_read_access)
add_unit_test(test_initial_access)
add_unit_test(test_create_work)
add_unit_test(test_access_handle)
#add_unit_test(test_segmented_key)
add_unit_test(test_functor)
add_unit_test(test_serialize)
add_unit_test(test_simple_key)
add_unit_test(test_create_condition)
add_unit_test(test_oo)
add_unit_test(test_collectives)
add_unit_test(test_array)

add_executable(run_all_frontend_tests ${frontendtestfiles})

function(add_oo_tag_generate_step target tag)
    set(DEST_FOLDER ${PROJECT_BINARY_DIR}/include)
    add_custom_command(
            OUTPUT ${DEST_FOLDER}/ootag_${tag}.generated.h
            COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/../scripts/generate_oo_header.py
            --tag ${tag} --folder ${DEST_FOLDER}
            DEPENDS
            ${PROJECT_SOURCE_DIR}/include/darma/impl/oo/macros.h
            ${PROJECT_SOURCE_DIR}/../scripts/generate_oo_header.py
        )
    add_custom_target(${target}_ootag_${tag}_generated_header
            DEPENDS
              ${DEST_FOLDER}/ootag_${tag}.generated.h
            )
    add_dependencies(${target} ${target}_ootag_${tag}_generated_header)
    add_dependencies(run_all_frontend_tests ${target}_ootag_${tag}_generated_header)
endfunction()

function(add_oo_class_generate_step target tag)
    set(DEST_FOLDER ${PROJECT_BINARY_DIR}/include)
    add_custom_target(${DEST_FOLDER}/oocls_${tag}.generated.h
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/../scripts/generate_oo_header.py
            --cls ${tag} --folder ${DEST_FOLDER}
    )
endfunction()

# generate macro expansions for test_oo
# TODO make this only enabled if a special flag is given
if (DARMA_GENERATE_OO_MACRO_FILES)
    find_package(PythonInterp)
    if( PYTHON_VERSION_STRING VERSION_LESS 2.7 )
        message(FATAL_ERROR "Can't generate OO headers for test_oo without Python2.7 or later")
    endif()
    if(PYTHONINTERP_FOUND)
        add_oo_tag_generate_step(test_oo larry)
        add_oo_tag_generate_step(test_oo curly)
        add_oo_tag_generate_step(test_oo moe)
        add_oo_tag_generate_step(test_oo bart)
        add_oo_tag_generate_step(test_oo lisa)
        add_oo_tag_generate_step(test_oo marge)
        add_oo_tag_generate_step(test_oo homer)
    else()
        message(FATAL_ERROR "Can't generate OO headers for test_oo without Python2.7 or later")
    endif()
else()
    target_compile_definitions(test_oo PUBLIC )
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDARMA_CANNOT_GENERATE_TAG_HEADERS=1")
endif()

target_link_libraries(run_all_frontend_tests ${DARMA_BACKEND_LIBNAME} gtest gmock pthread)
if (DARMA_COVERAGE)
setup_target_for_coverage(frontend_coverage run_all_frontend_tests coverage)
endif()
