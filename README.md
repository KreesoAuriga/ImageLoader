Coding task for a job interview.

Created in Visual Studio 2022 as a CMake 'project' to allow it to be cross platform.  In Visual Studio 2022, load the 'ImageLoader' folder from the root of the repository. 

Major functionality declared as abstract classes, serving as interfaces.
The default implementations of these interfaces are currently defined in the same files as the interfaces. A more completed implementation would move the implementations to different files inside of separate namespaces.
The major functionality is intended for implementations to accept interfaces, allowing for dependency injection and for these types to be agnostic of implementation. This also facilitates unit testing by allowing mock implementations of interfaces in order to test functions without needing to construct fully implemented objects which depend on other objects also being fully implemented.

The main program function currently serves to execute test code, whereas a complete implementation would move this code to a dedicated test project, including ImageLoader as a library.

ImageLoader:
The top level class of the implementation. Caching behaviour is defined by an implementation of the IImageCache interface, provided to the constructor of ImageLoader.
The TryGetImage function accepts a file path for the image to be loaded and returns an std::future for acquiring the result via an asynchronous execution.
This function attempts to acquire the image from the ImageCache interface first, and if not acquired proceeds to load the image file.

ImageFileLoader:
Loads image data from a file path. This makes use of image loading functionality from stb https://github.com/nothings/stb 
stb consists of a header file only implementations for loading various image formats, and so the header file has been directly included for simplicity.
This class returns image data as an unsigned char array, along with the image width and height.

The ImageLoader class constructs an instance of Image, which implements the IImage interface from the ImageData returned by ImageFileLoader.
The implementation of IImage also defines ImageResized, which serves to represent the resized version of an image with reference to the original image instance before resizing.

Note that while the ImageLoader class defines a TryGetImage function that accepts arguments for getting an image at an explicit width and height this has not yet been implemented. Resizing would be performed using the resizing functionality that is also from stb https://github.com/nothings/stb, the relevant header file is already included in this repository.

ImageCache:
This is the most fully implemented class, and supports adding images at both their original source dimensions, as well as at resized dimensions. It also has functionality implemented to get an image at a specific width and height if the image has been placed in the cache. The cache stores images using the filepath as a key, and a container consisting of the original size image as loaded from disk, and another std::map to contain the resized copies of that image.
It also has implemented an affordance to check the memory usage after adding a new item, and if above the configurable memory limitation, removes the image items least recently accessed until memory consumption is no longer above the threshhold.

A unit test is defined for the ImageFileLoader class, and there are placeholders for unit tests for other classes that have not yet been implemented.

