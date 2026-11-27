cmake_minimum_required(VERSION 3.24)

set(payload "${stage}/payload")
set(app "${payload}/DAZ Shifter.app")
set(component_options "${stage}/components.plist")
set(product "${stage}/installer.pkg")

file(REMOVE_RECURSE "${payload}")
file(REMOVE "${component_options}" "${stage}/component.pkg" "${product}")
file(MAKE_DIRECTORY "${app}/Contents/MacOS" "${app}/Contents/Resources")
file(COPY_FILE "${bundle_info}" "${app}/Contents/Info.plist")
file(COPY_FILE "${executable}" "${app}/Contents/MacOS/daz-shifter-desktop")
file(CHMOD "${app}/Contents/MacOS/daz-shifter-desktop" PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(COPY_FILE "${license}" "${app}/Contents/Resources/LICENSE")

if(NOT app_sign_identity STREQUAL "-")
  set(code_sign_options --timestamp --options runtime)
endif()
execute_process(
  COMMAND /usr/bin/plutil -lint "${app}/Contents/Info.plist"
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND /usr/bin/codesign --force --sign "${app_sign_identity}" ${code_sign_options} "${app}"
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND /usr/bin/codesign --verify --strict "${app}"
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND /usr/bin/pkgbuild --analyze --root "${payload}" "${component_options}"
  COMMAND_ERROR_IS_FATAL ANY
)
# Keep upgrades in the selected domain when the app exists in both domains.
execute_process(
  COMMAND /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "${component_options}"
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND /usr/bin/pkgbuild --root "${payload}" --component-plist "${component_options}"
    --identifier io.github.lexcibien.daz-shifter --version "${version}"
    --install-location /Applications "${stage}/component.pkg"
  COMMAND_ERROR_IS_FATAL ANY
)

if(NOT installer_sign_identity STREQUAL "")
  set(installer_sign_options --sign "${installer_sign_identity}")
endif()
execute_process(
  COMMAND /usr/bin/productbuild --distribution "${distribution}" --package-path "${stage}"
    ${installer_sign_options} "${product}"
  COMMAND_ERROR_IS_FATAL ANY
)
file(RENAME "${product}" "${artifact}")
