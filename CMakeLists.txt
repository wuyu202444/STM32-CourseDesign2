cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME CurseDesign_2)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        Drivers/BSP/global.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)




# 1. 定义你的 BSP 文件夹路径 (相对于工程根目录)
set(MY_BSP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP")   #这个路径是相对于根目录的

# 2. 告诉编译器去哪里找头文件 (.h)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${MY_BSP_PATH}
)

# 3. 自动寻找该文件夹下所有的 .c 文件，并存入变量
file(GLOB MY_BSP_SOURCES "${MY_BSP_PATH}/*.c")

# 4. 把找到的 .c 文件加入编译列表
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${MY_BSP_SOURCES}
)


# 设置浮点运算模式（根据MCU选择）
# 对于没有硬件FPU的MCU使用soft
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=soft")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=soft")

# 对于有硬件FPU的MCU（如F4系列）使用
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=hard -mfpu=fpv4-sp-d16")

# 使能printf浮点数支持
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u_printf_float)

# 可选：使能scanf浮点数支持
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u_scanf_float)

# 可选：使用nano库减小代码体积
# target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
#     --specs=nano.specs
#     --specs=nosys.specs
# )