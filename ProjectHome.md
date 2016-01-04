## 简介 ##

本项目包含以下几个子项目：

  * LunarDate
> 这是一个基于gobject的农历转换库，依赖于glib，提供主要的公农历转换接口函数。
    1. 目前能提供1900-2049年的公农历信息，包括农历、节气、节日、纪念日、干支、八字、生肖等等。
    1. 支持大陆、台湾和港澳地区的本地节日，程序运行时将通过locale自动判断并显示。
    1. 支持自定义纪念日，自定义纪念日功能需用户自行修改**$XDG\_CONFIG\_HOME/liblunar/hodiday.dat**文件。
    1. 支持多语言绑定，通过gobject-introspection，可以支持其它语言(如vala、python、ruby、lua、java、javascript等等)调用。

  * LunarCalendar
> 基于LunarDate，继承了GtkCalendar而实现的一个gtk的农历部件，可用于gtk编写的带日历部件的程序中。目前有两个版本2.x和3.x，分别对应于gtk2和gtk3，这两个版本可选择安装或同时安装。
    1. 使用preload, 可让使用GtkCalendar部件(不支持农历的gtk自带日历)的已有程序在运行时显示农历日历。
    1. 支持对自定义节日设置不同颜色
    1. 支持多语言绑定，通过gobject-introspection，可以支持其它语言(如vala、python、ruby、lua、java、javascript等等)调用。

  * QLunarDate (计划中...)
> 将参考gstreamer-qt，实现Qt对LunarDate的封装，并编写相应的Qt部件QLunarCalendar，以在KDE桌面上实现农历的显示。

## 安装与配置 ##

```
./configure --prefix=/usr --enable-gtk-doc ...
make
make install
```
配置阶段可以使用的选项及含义如下：

**--enable-gtk-doc**       编译参考手册

**--enable-introspection** 编译gobject-introspection支持

**--enable-vala-bindings** 编译vala语言绑定(vala目前仍处于变化之中，暂不建议编译)

## 参考手册 ##

**[LunarDate Reference Manual](http://liblunar.googlecode.com/svn/docs/lunar-date/index.html)**

**[LunarCalendar Reference Manual](http://liblunar.googlecode.com/svn/docs/lunar-calendar/index.html)**

**[LunarCalendar 3 Reference Manual](http://liblunar.googlecode.com/svn/docs/lunar-calendar3/index.html)**

## FAQ ##

**1. 如何让现有的日历程序显示农历**
> 如果需要系统中所有带日历的程序都显示农历，需要编辑文件 **/etc/X11/xinit/xinitrc.d/99-liblunar-preload** , 在其中设置环境变量 **LD\_PRELOAD** ，并加上可执行权限，然后重新登录 (重启X)。文件内容如下：

```
  $ cat /etc/X11/xinit/xinitrc.d/99-liblunar-preload 
  #!/bin/sh
  LD_PRELOAD="/usr/lib/liblunar-calendar-preload.so"
  export LD_PRELOAD
  $ chmod +x /etc/X11/xinit/xinitrc.d/99-liblunar-preload
```

> 这将使得所有基于GtkCalendar的程序在运行时自动使用LunarCalendar显示农历日期，达到运行时切换。如果只是临时想使某个软件显示农历，比如查看empathy的聊天纪录，可以这样来运行：

```
  LD_PRELOAD=/usr/lib/liblunar-calendar-preload-3.0.so empathy
```

注意：lunar-calendar-3.x以上版本，应该使用/usr/lib/liblunar-calendar-preload-3.0.so这个库。

**2. 怎样修改代码来显示农历日历**

  * #include <lunar-calendar/lunar-calendar.h>
  * 将原程序中的 **gtk\_calendar\_new()** 替换为 **lunar\_calendar\_new()** 函数
  * 对GtkCalendar 实例设置 **GTK\_CALENDAR\_SHOW\_DETAILS** 标志。
  * 编译时使用`pkg-config --cflags --libs lunar-calendar-2.0`或`pkg-config --cflags --libs lunar-calendar-3.0`

**3. 能在日历中显示自定义的节日吗？
> 请在** /usr/share/liblunar/ **目录中找到合适的文件，将其复制为** ~/.config/liblunar/liblunar.dat **文件，并修改这个文件，在里面增加你自己的纪念日。**

**4. 怎样在非中文环境下不显示农历**
> LunarCalendar在中国大陆、中国台湾、中国香港等地区显示汉字，在其它地区的locale设置下，默认将显示拼音(这很难看，真的)。
> 设置系统环境变量 **LUNAR\_CALENDAR\_IGNORE\_NON\_CHINESE=1** ，可以将这种难看的拼音显示关闭，这样将不再显示农历。

## 截图 ##

LunarCalendar 构件

> ![http://liblunar.googlecode.com/svn/docs/lunar-calendar3/lunar-calendar.png](http://liblunar.googlecode.com/svn/docs/lunar-calendar3/lunar-calendar.png)