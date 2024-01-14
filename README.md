### qt-scrcpyservice  
这是一个使用Qt连接[scrcpy-server](https://github.com/Genymobile/scrcpy)的示例程序，运行这个示例需要在CMAKE中定义两个变量：
- FFMPEG_DIR：ffmpeg库路径
- SCRCPY_VERSION：scrcpy-server的版本
  
此外，需要准备adb和scrcpy-server程序，例如：
- 3rdparty/adb/adb.exe
- 3rdparty/adb/AdbWinApi.dll
- 3rdparty/adb/AdbWinUsbApi.dll
- 3rdparty/scrcpy/scrcpy-server
  
编译成功时将其拷贝到运行目录：
```cmake
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND (taskkill /f /t /im adb.exe) || echo "adb not found"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/3rdparty/adb $<TARGET_FILE_DIR:${PROJECT_NAME}>/adb
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/3rdparty/scrcpy $<TARGET_FILE_DIR:${PROJECT_NAME}>/scrcpy
)
```
原理看文章：[https://blog.csdn.net/baidu_30570701/article/details/135583763](https://blog.csdn.net/baidu_30570701/article/details/135583763)