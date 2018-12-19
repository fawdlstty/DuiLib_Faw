# **DuiLib_Faw**

## 描述

此项目是DuiLib的一个个人维护版本，使用C++17重写了整个项目。

大众所熟知的DuiLib主分支处于较少更新的状态，几年前的激进更新派DuiLib_Ultimate、DuiLib_Redrain等也渐渐变成了保守派，也降低了更新速度，然而这个优秀的库也渐渐的无法满足当今的需要。

此仓库建立的目的是，对DuiLib做出较激进的更新优化，使得它能更好的满足当今开发的需要。

此项目的更新方式为，一方面项目自身激进的更新，另一方面原分支仓库有更新时，根据需要将更新内容同步至此仓库。

项目使用示例：[易大师网络工具箱](https://github.com/fawdlstty/NetToolbox)

此项目目前只支持vs2017环境下编译，如果使用clang、gcc或其他版本编译器需要自行建立项目文件。

## 使用

### 第一步：链接dll

#### 动态链接

使用动态链接的程序需在程序目录保留DuiLib*.dll，否则程序无法运行。由于动态链接有诸多不利因素（更大的软件体积、更多的文件引用等），所以在发行版中不推荐这种方式。

用法：主程序内引用如下代码

```C++
#if (defined _UNICODE) && (defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_64d.lib")
#elif (defined _UNICODE) && (defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_d.lib")
#elif (defined _UNICODE) && (!defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_64.lib")
#elif (defined _UNICODE) && (!defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib.lib")
#elif (!defined _UNICODE) && (defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_64d.lib")
#elif (!defined _UNICODE) && (defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_d.lib")
#elif (!defined _UNICODE) && (!defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_64.lib")
#elif (!defined _UNICODE) && (!defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA.lib")
#endif
```

#### 静态链接

静态链接具有诸多好处：依赖关系更少、程序体积更小等。极端的追求程序体积可以使用这种链接方式，使用VC_LTL优化项目引用并缩小程序体积，最后使用upx对程序进行压缩。

用法：主程序内引用如下代码

```C++
#if (defined _UNICODE) && (defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_64sd.lib")
#elif (defined _UNICODE) && (defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_sd.lib")
#elif (defined _UNICODE) && (!defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_64s.lib")
#elif (defined _UNICODE) && (!defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLib_s.lib")
#elif (!defined _UNICODE) && (defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_64sd.lib")
#elif (!defined _UNICODE) && (defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_sd.lib")
#elif (!defined _UNICODE) && (!defined _DEBUG) && (defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_64s.lib")
#elif (!defined _UNICODE) && (!defined _DEBUG) && (!defined _WIN64)
#	pragma comment (lib, "../lib/DuiLibA_s.lib")
#endif
```

看需求加入预处理器定义UILIB_STATIC，然后将编译选项分别设置为/MT、/MTd即可。

### 第二步：增加变量绑定

需将所有的这种代码：
```C++
// InitWindow里面
CEditUI *ctrl = dynamic_cast<CEditUI*> (parent->find_control (_T ("ctrl_name")));
```
替换为：
```C++
// 任意位置
BindEditUI ctrl { _T ("ctrl_name") };
```
需注意：
1. 绑定对象的创建位置不限，既可以在程序运行时绑定，也可以在InitWindow执行完后再绑定
2. 绑定对象在窗口及PaintManager创建前是无法使用的，只有在两者创建完成后才能使用
3. PaintManager对象不要设置名称，如果必须设置的场合，需要在Bind/BindCtrls.hpp代码中同步修改
4. BindXxxxxUI对象的使用方式与CXxxxxUI相同，BindXxxxxUI不能new，使用时直接使用->访问CXxxxxUI里的属性或方法

### 第三步：自定义控件绑定

假设XML节点为&lt;UserCtrl name="ctrl_name" /&gt;，那么类名必须风格统一，为CUserCtrlUI；然后在头文件中加入以下代码：
```C++
#ifdef DEF_BINDCTRL
DEF_BINDCTRL (UserCtrl);
#endif //DEF_BINDCTRL
```
大功告成，此时可以用以下代码实现自定义控件绑定
```C++
BindUserCtrlUI ctrl { _T ("ctrl_name") };
```

## 已更新内容

1. 完善链接方式，针对是否为Unicode、Debug、64位、动态或静态，4种条件16种链接方式做出完善的链接选项
2. GroupBox边框绘制时自动擦除文字位置，使得不用设置背景色，文字依旧不被边框遮盖
3. 修复容器子元素手动右对齐，方法为子控件加入代码：floatalign="right"
4. 修复Combo控件的SetText功能
5. 新增图片的dest可为负数，当为负数时，位置固定在目标控件的右边。比如Combo控件固定到右边的下拉箭头
6. 修改CDuiString实现方式的底层为std::string，使得兼容性更强
7. Label控件新增autocalcheight，效果类似于autocalcwidth
8. 使用std::variant代替STRINGorID
9. 使用std::string_view代替了几乎所有的LPCTSTR
10. 移除了CDuiPoint、CDuiRect、CDuiSize
11. 整理出公共类FawTools，将大部分重复的代码合并
12. 实现控件绑定（经研究发现，因为语言的问题，值绑定的效果不好，所以这儿暂时不考虑，如果有好的建议可以提）
13. 修复Edit控件nativetextcolor属性无效的问题
14. 新增绘图引用，使得指定的背景图等可以动态改变
15. 修复设置背景色时移除图片资源的bug
16. 新增Dynamic属性，避免因caption覆盖内容时，可拖动边框失效的问题

## 待研究或待添加

1. 实现虚拟列表效果
2. 实现文字阴影效果
3. 图片没有autocalcwidth、autocalcheight
4. 容器不支持根据子元素大小来计算大小
5. 考虑新增Ribbon、Docker、MVC功能

## 许可证：MIT+原始许可
