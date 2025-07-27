file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/main.py
     ${CMAKE_CURRENT_BINARY_DIR}/main.py SYMBOLIC)
add_custom_target(test COMMAND python main.py ${CMAKE_BUILD_TYPE})
add_dependencies(test vaq-make)
