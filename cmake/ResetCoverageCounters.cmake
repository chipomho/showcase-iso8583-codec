# Deletes the gcov counter files (*.gcda) under DIR. gcov adds to existing
# counters, so without this a test that has since been deleted would still
# count towards coverage. Run by CTest before every test in a coverage build.
file(GLOB_RECURSE counters "${DIR}/*.gcda")
if(counters)
    file(REMOVE ${counters})
endif()
list(LENGTH counters removed)
message(STATUS "Removed ${removed} coverage counter files")
