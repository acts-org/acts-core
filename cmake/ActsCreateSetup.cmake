# Create a setup.sh file at the install prefix to setup the shell environment
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/setup.sh.in
    ${PROJECT_BINARY_DIR}/this_acts.sh
    @ONLY
)
install(
    FILES ${PROJECT_BINARY_DIR}/this_acts.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/setup_withdeps.sh.in
    ${PROJECT_BINARY_DIR}/this_acts_withdeps.sh
    @ONLY
)
install(
    FILES ${PROJECT_BINARY_DIR}/this_acts_withdeps.sh
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
