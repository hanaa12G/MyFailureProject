add_executable (${PROJECT_NAME} platform_win32.cc)


add_library (application application.cc)

SET (LIBS D2D1 DWRITE)
target_link_libraries(${PROJECT_NAME} ${LIBS} application)
