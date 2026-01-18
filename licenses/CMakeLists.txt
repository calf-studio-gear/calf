# List of all license files you want to combine
set(MY_LICENSES
    "${CMAKE_CURRENT_SOURCE_DIR}/GPL-2.1.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/LGPL-2.1.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/MIT.txt"
    "${CMAKE_CURRENT_SOURCE_DIR}/ISC.txt"
)

# Output file
set(ALL_LICENSES "${CMAKE_CURRENT_BINARY_DIR}/ALL_LICENSES.txt")

# Make sure output directory exists
# file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/licenses")

# Generate combined license file at configure time
file(WRITE "${ALL_LICENSES}" "")  # clear

foreach(lic IN LISTS MY_LICENSES)
  get_filename_component(licname "${lic}" NAME)

  file(APPEND "${ALL_LICENSES}" "========================\n")
  file(APPEND "${ALL_LICENSES}" "${licname}\n")
  file(APPEND "${ALL_LICENSES}" "========================\n\n")

  # READ the file contents and append them
  file(READ "${lic}" lic_text)
  file(APPEND "${ALL_LICENSES}" "${lic_text}\n\n\n")
endforeach()
