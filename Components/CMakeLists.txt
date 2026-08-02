# Register the shared types and ports so they generate C++ code
register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/Types.fpp"
)

# Add your component subdirectories
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/BeeLogic/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/WiiBoardManager/")