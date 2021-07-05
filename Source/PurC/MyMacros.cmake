# Append the all C files in the specified directory list to the source list
MACRO(APPEND_ALL_SOURCE_FILES_IN_DIRLIST result modules)
    SET(filelist "")
    FOREACH(module ${modules})
        LIST(APPEND filelist ${module}/*.c)
        LIST(APPEND filelist ${module}/*.cpp)
    ENDFOREACH()
    file(GLOB_RECURSE ${result} RELATIVE ${PURC_DIR} ${filelist})
#    FOREACH(file ${${result}})
#        message(STATUS ${file})
#    ENDFOREACH()
    unset(filelist)
ENDMACRO()