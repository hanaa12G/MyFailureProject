add_executable (${PROJECT_NAME} main.cc)

SET (LIBS D2D1 DWRITE)
target_link_libraries(${PROJECT_NAME} ${LIBS})

