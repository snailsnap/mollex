add_executable(mollex mollex.cxx detect.cxx)
target_link_libraries(mollex ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT})
target_compile_definitions(mollex PRIVATE $<$<CONFIG:Debug>:DEBUG>)
