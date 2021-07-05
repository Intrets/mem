cmake_minimum_required(VERSION 3.20)

include("./cmake/MakeModule.cmake")

set(MODULE_NAME mem)

project(${MODULE_NAME})

make_module(
	MODULE_NAME ${MODULE_NAME}
	MODULE_FILES
		ReferenceManager
		Global
		Index
		LazyGlobal
	CXX_STANDARD 20
	OPTIONAL_LIBS
		serial
)
