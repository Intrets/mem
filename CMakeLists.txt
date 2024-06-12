# mem - A C++ library for managing objects
# Copyright (C) 2021 intrets

cmake_minimum_required(VERSION 2.18)

include("${CMAKE_SOURCE_DIR}/cmake/MakeModule.cmake")

set(MODULE_NAME mem)

project(${MODULE_NAME} LANGUAGES CXX)

make_module(
	MODULE_NAME ${MODULE_NAME}
	MODULE_FILES
		ReferenceManager
		Global
		Index
		LazyGlobal
		Everything
		MutexedObject
	CXX_STANDARD 23
	REQUIRED_LIBS
		tepp
	OPTIONAL_LIBS
		serial
)
