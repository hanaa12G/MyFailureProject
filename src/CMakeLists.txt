add_executable (${PROJECT_NAME} platform_win32.cc)

SET (LIBS D2D1 DWRITE)
target_link_libraries(${PROJECT_NAME} ${LIBS})

