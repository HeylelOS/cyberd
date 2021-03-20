cmake_minimum_required(VERSION 3.14)

project(Cyberd LANGUAGES C)

#########
# Build #
#########

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_SYSCONFDIR "etc" CACHE STRING "System configuration directory.")
set(CMAKE_INSTALL_LOCALSTATEDIR "var" CACHE STRING "Local state directory.")
set(CMAKE_INSTALL_RUNSTATEDIR "${CMAKE_INSTALL_LOCALSTATEDIR}/run" CACHE STRING "Run state directory.")

set(CONFIG_DAEMONCONFS_DIRECTORY "${CMAKE_INSTALL_SYSCONFDIR}/daemons" CACHE STRING "Directory where cyberd will look for daemon configuration files.")
set(CONFIG_ENDPOINTS_DIRECTORY "${CMAKE_INSTALL_RUNSTATEDIR}/init" CACHE STRING "Directory where cyberd will create its endpoints.")
set(CONFIG_ENDPOINT_ROOT "initctl" CACHE STRING "Name of the first endpoint to be created.")
set(CONFIG_DEFAULT_UMASK 022 CACHE STRING "Default umask for daemons.")
set(CONFIG_CONNECTIONS_LIMIT 64 CACHE STRING "Maximum connections cyberd can hold at one time.")
set(CONFIG_READ_BUFFER_SIZE 512 CACHE STRING "Size of the buffer used to read from endpoints.")
set(CONFIG_NAME_BUFFER_DEFAULT_CAPACITY 16 CACHE STRING "Default capacity of the buffer holding pathnames while parsing endpoints.")

configure_file(include/config.h.in include/config.h)
include_directories(${PROJECT_BINARY_DIR}/include include)

file(GLOB_RECURSE CYBERD_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/cyberd/*.c)
add_executable(cyberd ${CYBERD_SOURCES})

file(GLOB_RECURSE CYBERCTL_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/cyberctl/*.c)
add_executable(cyberctl ${CYBERCTL_SOURCES})

#################
# Documentation #
#################

find_package(Doxygen)

# We need doxygen with markdown support
if(DOXYGEN_FOUND AND DOXYGEN_VERSION VERSION_GREATER_EQUAL 1.8.0)
	set(DOXYGEN_EXTRACT_ALL YES)
	set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)

	doxygen_add_docs(doc
		${PROJECT_BINARY_DIR}/include/config.h
		docs/cyberctl.1.md
		docs/cyberd.5.md
		docs/cyberd.8.md
		docs/inter-process-communications.md
	)
endif()

###########
# Install #
###########

install(
	TARGETS cyberd cyberctl
	RUNTIME
)

install(DIRECTORY
	${PROJECT_SOURCE_DIR}/man/man1
	${PROJECT_SOURCE_DIR}/man/man5
	${PROJECT_SOURCE_DIR}/man/man8
	TYPE MAN
)

if(TARGET doc)
	install(DIRECTORY ${PROJECT_BINARY_DIR}/html TYPE DOC)
endif()

