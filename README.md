# 98VideoConverter
**Converts videos into a format compatible with 98VIDEOP.COM**

# About the program
**98VideoConverter** is a GUI application using libav\* to convert videos into .98v format, so that they can be played by 98VIDEOP.COM. It supports variable bitrate, dithering, and a rolling preview. Simply load a video file, fiddle with the parameters, check if it looks good and then export!

This program currently has a lot of hardcoded constants. These will be converted to modifiable variables in due time. Also, it's currently only a Windows program, so sorry if you don't use Windows.

# Building
This is a Visual Studio project, so if you already have Visual Studio open this repository in Visual Studio. If you don't want to use Visual Studio, then you'll have to convert the project files. Remember to link against libav\*, otherwise you won't be able to compile the program.