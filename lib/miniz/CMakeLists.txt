cmake_minimum_required(VERSION 3.5)
project(miniz LANGUAGES C VERSION 3.0.2)

add_library(miniz STATIC)
add_library(miniz::miniz ALIAS miniz)

target_sources(miniz PRIVATE
	${PROJECT_SOURCE_DIR}/miniz.c
	${PROJECT_SOURCE_DIR}/miniz.h
)

target_include_directories(miniz PUBLIC ${PROJECT_SOURCE_DIR})
