set(gammaray_launcher_ui_srcs main.cpp)
# TODO we don't need all the class icons here, so split the qrc file accordingly
qt4_add_resources(gammaray_launcher_ui_srcs ${CMAKE_SOURCE_DIR}/resources/gammaray.qrc)

add_executable(gammaray-launcher WIN32 ${gammaray_launcher_ui_srcs})

target_link_libraries(gammaray-launcher
  ${QT_QTGUI_LIBRARIES}
  gammaray_common
  gammaray_launcher_ui
)

gammaray_embed_info_plist(gammaray-launcher ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in)
set_target_properties(gammaray-launcher PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${LIBEXEC_INSTALL_DIR}"
)

install(TARGETS gammaray-launcher DESTINATION ${LIBEXEC_INSTALL_DIR})
