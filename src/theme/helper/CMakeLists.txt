set(helper_dir "${CMAKE_INSTALL_FULL_LIBDIR}/caelestia/bin")

add_executable(caelestia-chromium-policy chromiumpolicy.cpp)
install(TARGETS caelestia-chromium-policy DESTINATION "${helper_dir}")

target_compile_definitions(caelestia-theme PRIVATE
    CAELESTIA_CHROMIUM_HELPER_BIN="${helper_dir}/$<TARGET_FILE_NAME:caelestia-chromium-policy>"
)
