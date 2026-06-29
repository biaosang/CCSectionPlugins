option(PLUGIN_STANDARD_QSECTION_PROFILE "Install qSectionProfile plugin" OFF)

if (PLUGIN_STANDARD_QSECTION_PROFILE)
	project(QSECTION_PROFILE_PLUGIN)

	AddPlugin(NAME ${PROJECT_NAME})

	target_sources(${PROJECT_NAME}
		PRIVATE
			${CMAKE_CURRENT_LIST_DIR}/include/qSectionProfile.h
			${CMAKE_CURRENT_LIST_DIR}/include/SectionProfileDialog.h
			${CMAKE_CURRENT_LIST_DIR}/include/SectionProfileExtractor.h
			${CMAKE_CURRENT_LIST_DIR}/src/qSectionProfile.cpp
			${CMAKE_CURRENT_LIST_DIR}/src/SectionProfileDialog.cpp
			${CMAKE_CURRENT_LIST_DIR}/src/SectionProfileExtractor.cpp
	)

	target_include_directories(${PROJECT_NAME}
		PRIVATE
			${CMAKE_CURRENT_LIST_DIR}/include
	)

endif()
