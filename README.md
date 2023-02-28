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

为应用编译准备环境
```
set QT_BUILDDIR=C:\Qt\qteverywhere.5.15.2
set LD_LIBRARY_PATH=%QT_BUILDDIR%\lib;%LD_LIBRARY_PATH%
set QT_QPA_PLATFORM_PLUGIN_PATH=%QT_BUILDDIR%\plugins\platforms
set QT_PLUGIN_PATH=%QT_BUILDDIR%\plugin
set PKG_CONFIG_PATH=%QT_BUILDDIR%\lib\pkgconfig;%PKG_CONFIG_PATH%
set Path=%QT_BUILDDIR%\bin;%QT_BUILDDIR%\gnuwin32\bin;C:\devtools\Strawberry\perl\bin;%Path%

打开命令行提示符.

```
> set CL=/MP
> qmake ..  QMAKE_EXTRA_ARGS+="-proprietary-codecs -webengine-jumbo-build 0 -webengine-kerberos"
> nmake
> nmake install
```


For Windows, bison, flex and gperf are provided with the source code at c:\pathToQtsrc\gnuwin32\bin

To accelerate the bootstrap of qmake with MSVC, it may be useful to pass "-make-tool jom"

on the configure command line. If you do not use jom,adding "/MP" to the CL environment variable is a good idea.
