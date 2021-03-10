include("${PROJECT_SOURCE_DIR}/cmake/MakeModule.cmake")

set(MODULE_NAME mem)

set(MODULE_FILES
	ReferenceManager
	Locator
)

make_module(${MODULE_NAME} "${MODULE_FILES}")

# target_link_libraries(${MODULE_NAME} PRIVATE ...)
