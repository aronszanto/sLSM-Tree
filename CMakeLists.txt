cmake_minimum_required(VERSION 3.12)

# set the project name
project(sLSM-Tree)

# add the executable
file(GLOB sLSM-Tree_SRC CONFIGURE_DEPENDS "src/*.*")
add_executable(sLSM-Tree ${sLSM-Tree_SRC})