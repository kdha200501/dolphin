# How to

##### Install dependencies

```shell
$ sudo dnf builddep dolphin
```



##### Compile and install `dolphin`

```shell
$ dolphin --version
$ git checkout customize/v<x.y.z>
$ mkdir build
$ cd build
$ cmake ..
$ make -j$(nproc)
$ sudo make install
```





# Customization

##### Disable chain edit



##### Do not convert vertical scroll into horizontal



##### Display mouse-over effect when dragging, only



##### Customize shortcuts

This is part of an effort to bring back the Mac OS 9 desktop environment



##### Ensure one focus at a time



##### Use triangle in tree view

This is part of an effort to bring back the Mac OS 9 desktop environment



##### Interpret the enter QKeyEvent on file as rename signal

This is part of an effort to bring back the Mac OS 9 desktop environment



##### Change application icon

This is part of an effort to bring back the Mac OS 9 desktop environment

> [!TIP]
>
> To resize an image
>
> ```shell
> $ inkscape /path/to/origin.svg --export-type="png" --export-filename=/path/to/source/16-apps-org.kde.dolphin.png -w 16 -h 16
> ```



> [!TIP]
>
> To create "Finder" launcher
>
> ```c
> $ sudo touch /usr/share/applications/finder.desktop
> $ sudo vim /usr/share/applications/finder.desktop
> ```
>
> Copy and paste:
>
> ```shell
> [Desktop Entry]
> Type=Application
> Name=Finder
> Exec=dolphin
> Icon=/path/to/finder.svg
> Terminal=false
> Categories=Utility;FileManager;
> ```
