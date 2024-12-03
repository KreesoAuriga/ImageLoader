Coding task for a job interview.

Created in Visual Studio 2022 as a CMake 'project' to allow it to be cross platform.  In Visual Studio 2022, load the 'ImageLoader' folder from the root of the repository. 

Major functionality declared as abstract classes, serving as interfaces.
The default implementations of these interfaces are currently defined in the implementations folder.
The major functionality is intended for implementations to accept interfaces, allowing for dependency injection and for these types to be agnostic of implementation. This also facilitates unit testing by allowing mock implementations of interfaces in order to test functions without needing to construct fully implemented objects which depend on other objects also being fully implemented.

The main program function currently serves to execute test code, whereas a complete implementation would move this code to a dedicated test project, using ImageLoader as an included library.

ImageLoader:
The top level class of the implementation. Caching behaviour is defined by an implementation of the IImageCache interface, provided to the constructor of ImageLoader.
The TryGetImage function accepts a file path for the image to be loaded, and a callback function to return the result. This places a task on a queue and returns a status value to indicate if a new task was created or if there was already a task for the request imaged. TryGetImageAtSize allows for specifying an exact width and height version of the image to be returned instead of the size as loaded from disk.

This task created by this function attempts to acquire the image from the ImageCache interface first, if image has been loaded but the requested size is not in the cache it creates the resized image and places it in the cache. If the cache has no entry for the source iamage it proceeds to load the image file.

ImageDataReader:
Loads image data from a file path. This makes use of image loading functionality from stb https://github.com/nothings/stb 
stb consists of a header file only implementations for loading various image formats, and so the header file has been directly included for simplicity.
This class returns image data as an unsigned char array, along with the image width and height.

The ImageLoader class uses an implementation of IImageFactory constructs an instance of an implementation defined IImage, constructed from the width, height, and pixel data.

ImageCache:
This stores entries for instances of the source image, as well as the instances of the implementation defined IImage at various resolutions. IImage instances are provided as a shared pointer, so that the cache can automatically removed an instance once all references to the shared pointer have been destructed, and once all instances of an image at all sizes are destructed, it can delete the source image and it's entry from the cache.
The current implementation returns an image load status reporting out of memory if there loading are creating a resized image exceeds the maximum. Because the cache is agnostic of the usage of each instance of an IImage created by the IImageFactory, this approach is used rather than flushing older items from the cache.

A unit test is defined for the ImageDataReader class, and there are placeholders for other tests not yet been implemented.

The main program loop currently serves to run the ImageDataReader unit tests, and performs an acceptance test using images from the TestData folder to confirm image loading at different max thread counts, as well as behaviour when the maximum memory size for the cache is exceeded.

