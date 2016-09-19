GET_FILENAME_COMPONENT(myDir ${CMAKE_CURRENT_LIST_FILE} PATH)

SET(QJSON_LIBRARIES qjson)
SET(QJSON_INCLUDE_DIR "C:/Program Files (x86)/qjson/include")

include(${myDir}/QJSONTargets.cmake)
