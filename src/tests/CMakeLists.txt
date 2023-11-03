get_property(CATCH_INCLUDE_DIRECTORIES TARGET Catch2 PROPERTY INCLUDE_DIRECTORIES)
if (CATCH_INCLUDE_DIRECTORIES)
	#message(STATUS "Catch2: ${Catch2_INCLUDE_DIRECTORIES}; LIBS: ${Catch2_LIBRARIES}")
	message(STATUS "Catch2 includes: ${CATCH_INCLUDE_DIRECTORIES}")
	target_include_directories(openttd_test
		PUBLIC ${CATCH_INCLUDE_DIRECTORIES})
endif()

add_test_files(
    landscape_partial_pixel_z.cpp
    math_func.cpp
    string_func.cpp
    strings_func.cpp
    test_main.cpp
    test_script_admin.cpp
    test_window_desc.cpp
)

