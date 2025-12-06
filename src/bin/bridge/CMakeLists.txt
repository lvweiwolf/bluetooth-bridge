
aux_source_directory(. SOURCES)

add_executable (bridge ${SOURCES})
apply_compiler_config(bridge)

# 链接依赖库
target_link_libraries(bridge PUBLIC core)


set_target_properties(bridge PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# TODO: 如有需要，请添加测试并安装目标。
install(TARGETS bridge
		COMPONENT runtime
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})