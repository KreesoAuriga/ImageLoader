# Implement an Asynchronous Image Loader

## Objective:
In this assignment, you will implement a multithreaded asynchronous image loader in C++ that supports:

- Asynchronous image loading: Support non-blocking operations to load image files from disk. 

- Asynchronous image resizing: Dynamically resize requested images to specified dimensions without blocking the calling thread.

- A configurable memory-limited cache: This cache will store images that have already been loaded and resized, allowing future requests for the same file and dimensions to return the cached result immediately.

- A configurable thread pool: The thread pool limits the number of concurrent operations, balancing performance and resource usage.

Your task is to create the implementation of the ImageLoader class and its supporting components.

## Example usage:
A practical example of the ImageLoader class usage would be in a real-time graphics application or a multimedia application, such as a photo viewer, video editor, or game engine. These applications need to load and display images quickly and efficiently, often resizing them dynamically to fit different resolutions or UI components.

## Deliverables:

1. Source Code:

- Please submit source code, either directly or via a private repository.

2. Build Instructions:

- Include clear instructions for building and running your implementation. 
  - It should be possible to build the implementation via a script or IDE (e.g. Xcode on macOS or Visual Studio on Windows).

- Any dependencies or external libraries should be listed with installation instructions.

3. Documentation:

- Provide a brief README with an overview of your approach, any known limitations, and how to run and test the core features.

## Additional Notes:

- How far you take the implementation is up to you; however, the minimum requirements outlined above should be completed.

- Focus on code quality, modular design, and clear documentation.

- You may select the image format, and make use of a third-party for image reading and resizing. 
  - Use C++, and the STL, for all other components.
