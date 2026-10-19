set(helper_dir "${CMAKE_INSTALL_FULL_LIBDIR}/caelestia/bin")

add_executable(caelestia-chromium-policy chromiumpolicy.cpp)
install(TARGETS caelestia-chromium-policy DESTINATION "${helper_dir}")

set(CAELESTIA_CHROMIUM_HELPER "${helper_dir}/caelestia-chromium-policy")
target_compile_definitions(caelestia-theme PRIVATE
    CAELESTIA_CHROMIUM_HELPER_BIN="${CAELESTIA_CHROMIUM_HELPER}"
)

option(INSTALL_SUDOERS "Install sudoers rules for theming helpers" ON)
if(INSTALL_SUDOERS)
    set(SUDOERS_DIR "/etc/sudoers.d" CACHE PATH "Directory for sudoers drop-in files")
    find_program(PAPIRUS_FOLDERS papirus-folders)
    if(NOT PAPIRUS_FOLDERS)
        set(PAPIRUS_FOLDERS "/usr/bin/papirus-folders")
    endif()
    configure_file("${CMAKE_SOURCE_DIR}/cmake/caelestia-theming.sudoers.in" "${CMAKE_BINARY_DIR}/caelestia-theming.sudoers" @ONLY)
    install(FILES "${CMAKE_BINARY_DIR}/caelestia-theming.sudoers"
        DESTINATION "${SUDOERS_DIR}"
        RENAME caelestia-theming
        PERMISSIONS OWNER_READ GROUP_READ
    )
endif()
