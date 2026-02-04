cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(loudcards)

set(PARTITION_TABLE_CSV "${CMAKE_SOURCE_DIR}/partitions.csv")
set(PARTITION_TABLE_CUSTOM 1)