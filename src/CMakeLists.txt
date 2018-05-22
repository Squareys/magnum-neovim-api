#
#   Copyright © 2018 Jonathan Hale <squareys@googlemail.com>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

find_package(Corrade REQUIRED TestSuite Containers)
find_package(Magnum REQUIRED)

add_subdirectory(Corrade/Net/)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(generate ALL
    python ${CMAKE_SOURCE_DIR}/generate_bindings.py
           -n ${CMAKE_SOURCE_DIR}/Neovim/bin/nvim.exe
           -t ${CMAKE_SOURCE_DIR}/template
           -o ${CMAKE_CURRENT_BINARY_DIR}
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/neovimapi2.cpp PROPERTIES GENERATED TRUE)

# Compile generated sources to a static lib
add_library(NeovimApi STATIC ${CMAKE_CURRENT_BINARY_DIR}/neovimapi2.cpp)
target_link_libraries(NeovimApi Magnum::Magnum CorradeNet Corrade::Containers mpack)
add_dependencies(NeovimApi generate)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Test)
endif()

