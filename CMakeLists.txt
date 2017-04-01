cmake_minimum_required(VERSION 3.6)

# project name
project(fake_node)

# head file path

set(CMAKE_CXX_STANDARD 11)

set(fake_node main.cpp out/x64.release/libv8_base.a out/x64.release/libv8_libbase.a out/x64.release/libicudata.a out/x64.release/libicuuc.a out/x64.release/libicui18n.a out/x64.release/libv8_external_snapshot.a out/x64.release/libv8_libplatform.a)
add_executable(fake_node ${fake_node})