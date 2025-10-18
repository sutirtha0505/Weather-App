cmake_minimum_required(VERSION 3.10)
project(weather-app)

# Set C standard
set(CMAKE_C_STANDARD 99)

# Include directories
include_directories(include)
include_directories(/opt/homebrew/include)  # Homebrew includes

# Link directories
link_directories(lib)
link_directories(/opt/homebrew/lib)  # Homebrew lib

# Add executable target
add_executable(weather-app src/main.c src/weather.c)

# Link libraries (cJSON and curl)
target_link_libraries(weather-app cJSON curl)
