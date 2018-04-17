add_executable(mollex mollex.cxx)
target_link_libraries(mollex ${OpenCV_LIBS})
target_compile_definitions(mollex PRIVATE $<$<CONFIG:Debug>:DEBUG>)
