# qtwebengine


HOW TO BUILD
================

   Debian
   -----------


安装qtbase的依赖
```
$ sudo apt build-dep qtbase-opensource-src
```

为应用编译准备环境
```
export QT_BUILDDIR=/opt/qt/qteverywhere.5.15.2
export LD_LIBRARY_PATH=$QT_BUILDDIR/lib:$LD_LIBRARY_PATH
export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_BUILDDIR/plugins/platforms
export QT_PLUGIN_PATH=$QT_BUILDDIR/plugin
export PKG_CONFIG_PATH=$QT_BUILDDIR/lib/pkgconfig:$PKG_CONFIG_PATH
export PATH=$QT_BUILDDIR/bin:$PATH
```

编译
```
$ cd qtwebengine

$ mkdir & cd build

$ qmake .. QT_BUILD_PARTS+=tests QMAKE_EXTRA_ARGS+="-proprietary-codecs -webengine-jumbo-build 0 -system-webp -webengine-kerberos -nomake tests"

$ make -j `nproc`
$ make install
```



   Windows
   ----------

拷贝gperf

qteverywhere\gnuwin32\bin to C:\Qt\qteverywhere.5.15.2

安装perl [https://strawberryperl.com/download/5.32.1.1/strawberry-perl-5.32.1.1-64bit.msi]

安装python2/python3 [https://www.python.org/ftp/python/2.7.17/python-2.7.17.amd64.msi]

下载node.exe放到环境变量Path里 [https://nodejs.org/dist/v18.14.2/node-v18.14.2-win-x64.zip] 已经包含在源码目录中[tools\nodejs\node.exe]

为应用编译准备环境
```
set QT_BUILDDIR=C:\Qt\qteverywhere.5.15.2
set LD_LIBRARY_PATH=%QT_BUILDDIR%\lib;%LD_LIBRARY_PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=%QT_BUILDDIR%\plugins\platforms
set QT_PLUGIN_PATH=%QT_BUILDDIR%\plugin
set PKG_CONFIG_PATH=%QT_BUILDDIR%\lib\pkgconfig;%PKG_CONFIG_PATH%
set Path=%QT_BUILDDIR%\bin;%QT_BUILDDIR%\gnuwin32\bin;C:\devtools\Strawberry\perl\bin;%Path%
```

打开命令行提示符配置参数，此参数支持视频解码，网页视频可以通过ffmpeg解码播放
```shell
> set CL=/MP
> qmake ..  QMAKE_EXTRA_ARGS+="-proprietary-codecs -webengine-jumbo-build 0 -webengine-kerberos"
> nmake
> nmake install
```

编译中Core部分的命令
```
> C:\workspace\qtwebengine.5.15.2\build\src\3rdparty\ninja\ninja.exe -v  -C C:\workspace\qtwebengine.5.15.2\build\src\core\release QtWebEngineCore -k 100
```

For Windows, bison, flex and gperf are provided with the source code at c:\pathToQtsrc\gnuwin32\bin

To accelerate the bootstrap of qmake with MSVC, it may be useful to pass "-make-tool jom"

on the configure command line. If you do not use jom,adding "/MP" to the CL environment variable is a good idea.
