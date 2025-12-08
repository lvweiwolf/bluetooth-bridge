set(CORE_SOURCES "")

aux_source_directory(utils UTILS_SRC)
aux_source_directory(mqtt MQTT_SRC)
aux_source_directory(bluetooth BLUETOOTH_SRC)
aux_source_directory(bluetooth/proxy BLUETOOTH_PROXY_SRC)
aux_source_directory(bluetooth/object BLUETOOTH_OBJECT_SRC)
aux_source_directory(bluetooth/rfcomm BLUETOOTH_RFCOMM_SRC)

configure_file(version.h.in version.h @ONLY)
set_source_files_properties("version.h" PROPERTIES GENERATED true)
list(APPEND CORE_SOURCES ${UTILS_SRC} 
						 ${MQTT_SRC} 
						 ${BLUETOOTH_SRC} 
						 ${BLUETOOTH_PROXY_SRC} 
						 ${BLUETOOTH_OBJECT_SRC}
						 ${BLUETOOTH_RFCOMM_SRC}
						 defines.h
						 version.h)


# 静态库
add_library(core ${CORE_SOURCES})
apply_compiler_config(core)

								   
target_link_libraries(core PUBLIC ${DEPENDENCY_LIBS})

set_target_properties(core PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)