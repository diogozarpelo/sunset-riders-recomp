# Preserve the tested MSVC compatibility fix across fresh checkouts.
if(MSVC)
    find_package(Git REQUIRED)
    set(_ppu_patch "${CMAKE_CURRENT_LIST_DIR}/../patches/msvc-ppu-alignment.patch")

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SNESRECOMP_ROOT}"
                apply --reverse --check "${_ppu_patch}"
        RESULT_VARIABLE _already_applied
        OUTPUT_QUIET ERROR_QUIET
    )

    if(_already_applied EQUAL 0)
        message(STATUS "MSVC PPU alignment patch already applied")
    else()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${SNESRECOMP_ROOT}"
                    apply --check "${_ppu_patch}"
            RESULT_VARIABLE _can_apply
            ERROR_VARIABLE _patch_error
        )
        if(NOT _can_apply EQUAL 0)
            message(FATAL_ERROR
                "Cannot apply the MSVC PPU alignment patch. "
                "Check the framework pin and local changes.\n${_patch_error}")
        endif()

        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${SNESRECOMP_ROOT}"
                    apply "${_ppu_patch}"
            RESULT_VARIABLE _apply_result
            ERROR_VARIABLE _patch_error
        )
        if(NOT _apply_result EQUAL 0)
            message(FATAL_ERROR "MSVC PPU alignment patch failed:\n${_patch_error}")
        endif()
        message(STATUS "Applied MSVC PPU alignment patch")
    endif()
endif()
