# message(STATUS "${INPUT_FILE} -> ${OUTPUT_FILE}")

file(READ ${INPUT_FILE} content)

string(
    REGEX REPLACE
    "ACTS_DOES_NOT_COMPILE_BEGIN\\(([A-Za-z0-9]+)\\)"
    "#if defined(\\1)"
    processed
    "${content}"
)
string(REPLACE "ACTS_DOES_NOT_COMPILE_END()" "#endif" processed "${processed}")

file(WRITE ${OUTPUT_FILE} "${processed}")
