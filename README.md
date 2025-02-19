# muduo_modernCpp

A modern C++ implementation of the Muduo network library components.

## Build Instructions

### Option 1: Using Docker (Recommended)
```bash
# Build the Docker image
docker build -t muduo-modern .

# Run the Docker container
docker run -it --rm -v $(pwd):/app muduo-modern bash
```

### Option 2: Local Build

#### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- pthread support

#### Building the Project
```bash
# Create a build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build the project
cmake --build .
```

### Project Structure
- `Thread.h/cpp`: Thread wrapper implementation
- `noncopyable.h`: Base class for non-copyable objects