# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "wx-client/CMakeFiles/tinywechat-client_autogen.dir/AutogenUsed.txt"
  "wx-client/CMakeFiles/tinywechat-client_autogen.dir/ParseCache.txt"
  "wx-client/tinywechat-client_autogen"
  )
endif()
