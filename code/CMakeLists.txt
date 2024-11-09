target_sources(${PROJECT_NAME}            PRIVATE main.cpp)
target_link_libraries(${PROJECT_NAME}    PRIVATE GLEW::GLEW glfw GLUT::GLUT)



set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/build"
                                                 RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build")