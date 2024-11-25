Coding task for a job interview.

Created in Visual Studio 2022 as a CMake 'project' to allow it to be cross platform.  In Visual Studio 2022, load the 'ImageLoader' folder from the root of the repository. 

Major functionality declared as abstract classes, serving as interfaces.
The default implementations of these interfaces are currently defined in the same files as the interfaces. A more completed implementation would move the implementations to different files inside of separate namespaces.
The major functionality is intended for implementations to accept interfaces, allowing for dependency injection and for these types to be agnostic of implementation. This also facilitates unit testing by allowing mock implementations of interfaces in order to test functions without needing to construct fully implemented objects which depend on other objects also being full implemented.

The main program function currently serves to execute test code, whereas a complete implementation would move this code to a dedicated test project, including ImageLoader as a library.

ImageLoader:
The top level class of the implementation. Caching behaviour is defined by an implementation of the IImageCache interface, provided to the constructor of ImageLoader.
The TryGetImage function accepts a file path for the image to be loaded and returns an std::future for acquiring the result via an asynchronous execution.
This function attempts to acquire the image from the ImageCache interface first, and if not acquired proceeds to load the image file.

ImageFileLoader:
Loads image data from a file path. This makes use of image loading functionality from stb https://github.com/nothings/stb 
stb consists of a header file only implementations for loading various image formats, and so the header file has been directly included for simplicity.
