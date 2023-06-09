# Milo Yip 教程整理与扩展

## tutorial 1

### Cmake 使用

> Cmake 工具的使用整理详见博客  **[Cmake C++ 开发]()**

### API 设计及 Json 语法参考

> 本项目API设计及支持语法详见 **[my leptjson 项目文档](leptjson.md)**

### 单元测试

#### 常用单元测试框架(C++)

* **[Google Test（GTest）](https://github.com/google/googletest)** 由 Google 开发和维护的 C++ 单元测试框架。它提供了完善的测试框架和断言库，支持多线程测试、TDD（测试驱动开发）和测试覆盖率等功能。
* **[Catch2](https://github.com/catchorg/Catch2)** 一个轻量级的 C++ 单元测试框架，界面简洁友好，API 易于使用，可快速编写简单或复杂的测试用例。除了基本的测试断言外，它还提供了匹配器（Matchers）和 BDD（行为驱动开发）风格的语法。
* **[doctest](https://github.com/onqtam/doctest)** 一个开源的、轻量级的、支持跨平台的的 C++ 单元测试框架，采用头文件定义方法，简单易用。

#### 测试驱动开发（test-driven development, TDD）思想

测试驱动开发，它的主要循环步骤为：

1. 加入一个测试。
2. 运行所有测试，新的测试应该会失败。
3. 编写实现代码。
4. 运行所有测试，若有测试失败回到3。
5. 重构代码。
6. 回到 1。

测试驱动开发思想其基本开发思路是先写测试，再实现功能。这样可以显著提高软件质量，减少缺陷，并获得更好的代码可维护性及灵活性。但与此同时也存在过度依赖测试用例，极大加大开发时间等问题。

### c 语言宏的编写技巧

此处以下列代码中宏定义为例，在宏定义中应使用 `do {} while()` 语句包裹。
在有多行情况下使用 `\` 标识换行。

```c
#define EXPECT(c, ch)             		\
	do {                          		\
		assert(*c->json == (ch)); 	\
		c->json++;                	\
	} while (0)
```

### 断言设置

断言（assertion）是 C 语言中常用的防御式编程方式。最常用的是在函数开始的地方，检测所有参数。有时候也可以在调用函数后，检查上下文是否正确。

C 语言的标准库含有 [`assert()`](https://en.cppreference.com/w/c/error/assert) 这个宏（需 `#include <assert.h>`），提供断言功能。当程序以 release 配置编译时（定义了 `NDEBUG` 宏），`assert()` 不会做检测；而当在 debug 配置时（没定义 `NDEBUG` 宏），则会在运行时检测 `assert(cond)` 中的条件是否为真（非 0），断言失败会直接令程序崩溃。

初使用断言的同学，可能会错误地把含[副作用](https://en.wikipedia.org/wiki/Side_effect_(computer_science))的代码放在 `assert()` 中：

```c
assert(x++ == 0); /* 这是错误的! */
```

这样会导致 debug 和 release 版的行为不一样。

另一个问题是，初学者可能会难于分辨何时使用断言，何时处理运行时错误（如返回错误值或在 C++ 中抛出异常）。简单的答案是，如果那个错误是由于程序员错误编码所造成的（例如传入不合法的参数），那么应用断言；如果那个错误是程序员无法避免，而是由运行时的环境所造成的，就要处理运行时错误（例如开启文件失败）。

## tutorial 2

### 重构

在讨论解析数字之前，我们再补充 TDD 中的一个步骤──重构（refactoring）。重构指的是：

> 在不改变代码外在行为的情况下，对代码作出修改，以改进程序的内部结构。

在 TDD 的过程中，我们的目标是编写代码去通过测试。但由于这个目标的引导性太强，我们可能会忽略正确性以外的软件品质。在通过测试之后，代码的正确性得以保证，我们就应该审视现时的代码，看看有没有地方可以改进，而同时能维持测试顺利通过。我们可以安心地做各种修改，因为我们有单元测试，可以判断代码在修改后是否影响原来的行为。

> 更多的关于重构方面的知识可以参考 《重构 改善既有代码的设计》

### Json 中数字的处理

本项目中使用 `strtod()` 函数直接进行数字的转换，将所有的数字类型统一设置为使用浮点类型 `double`。

> 此处可以考虑优化，将整型及浮点类型分离，减少空间和后续计算的开销。

## tutorial 3

### 内存泄漏检测（Linux）

`apt-get install valgrind`

linux 下使用 [valgrind](https://valgrind.org/)  工具

valgrind --leak-check=full  ./xxxx

> 对于本项目中代码的内存泄露测试情况详见 **[my leptjson 项目文档](leptjson.md)**

### 性能优化参考

1. 如果整个字符串都没有转义符，我们不就是把字符复制了两次？第一次是从 `json` 到 `stack`，第二次是从 `stack` 到 `v->u.s.s`。我们可以在 `json` 扫描 `'\0'`、`'\"'` 和 `'\\'` 3 个字符（ `ch < 0x20` 还是要检查），直至它们其中一个出现，才开始用现在的解析方法。这样做的话，前半没转义的部分可以只复制一次。缺点是，代码变得复杂一些，我们也不能使用 `lept_set_string()`。
2. 对于扫描没转义部分，我们可考虑用 SIMD 加速，如 [RapidJSON 代码剖析（二）：使用 SSE4.2 优化字符串扫描](https://zhuanlan.zhihu.com/p/20037058) 的做法。这类底层优化的缺点是不跨平台，需要设置编译选项等。
3. 在 gcc/clang 上使用 `__builtin_expect()` 指令来处理低概率事件，例如需要对每个字符做 `LEPT_PARSE_INVALID_STRING_CHAR` 检测，我们可以假设出现不合法字符是低概率事件，然后用这个指令告之编译器，那么编译器可能可生成较快的代码。然而，这类做法明显是不跨编译器，甚至是某个版本后的 gcc 才支持。

## tutorial 4

### Unicode

> **Unicode** 及字符编码等相关知识整理详见 **[字符编码]()**

## tutorial 5

### Json 数组存储数据结构选择

JSON 数组存储零至多个元素，最简单就是使用 C 语言的数组。数组最大的好处是能以 $O(1)$ 用索引访问任意元素，次要好处是内存布局紧凑，省内存之余还有高缓存一致性（cache coherence）。但数组的缺点是不能快速插入元素，而且我们在解析 JSON 数组的时候，还不知道应该分配多大的数组才合适。

另一个选择是链表（linked list），它的最大优点是可快速地插入元素（开端、末端或中间），但需要以 $O(n)$ 时间去经索引取得内容。如果我们只需顺序遍历，那么是没有问题的。还有一个小缺点，就是相对数组而言，链表在存储每个元素时有额外内存开销（存储下一节点的指针），而且遍历时元素所在的内存可能不连续，令缓存不命中（cache miss）的机会上升。

> 此处项目最终选择采用动态数组进行 Json 数组存储

### 解析思路

对于 JSON 数组，采用堆栈暂存方式递归进行解析。我们只需要把每个解析好的元素压入堆栈，解析到数组结束时，再一次性把所有元素弹出，复制至新分配的内存之中。

由于 JSON 数组解析后为树形型结构，因而在逐层递归解析结束时应还原堆栈的状态。

> 详细解析过程可参考 **[原教程 tutorial05](https://github.com/miloyip/json-tutorial/blob/master/tutorial05/tutorial05.md)**

## tutorial 6

### Json 对象存储数据结构选择

要表示键值对的集合，有很多数据结构可供选择，例如：

* 动态数组（dynamic array）：可扩展容量的数组，如 C++ 的 [`std::vector`](https://en.cppreference.com/w/cpp/container/vector)。
* 有序动态数组（sorted dynamic array）：和动态数组相同，但保证元素已排序，可用二分搜寻查询成员。
* 平衡树（balanced tree）：平衡二叉树可有序地遍历成员，如红黑树和 C++ 的 [`std::map`](https://en.cppreference.com/w/cpp/container/map)（[`std::multi_map`](https://en.cppreference.com/w/cpp/container/multimap) 支持重复键）。
* 哈希表（hash table）：通过哈希函数能实现平均 O(1) 查询，如 C++11 的 [`std::unordered_map`](https://en.cppreference.com/w/cpp/container/unordered_map)（[`unordered_multimap`](https://en.cppreference.com/w/cpp/container/unordered_multimap) 支持重复键）。

设一个对象有 n 个成员，数据结构的容量是 m，n ⩽ m，那么一些常用操作的时间／空间复杂度如下：

|                 | 动态数组 | 有序动态数组 |   平衡树   |         哈希表         |
| --------------- | :-------: | :----------: | :--------: | :--------------------: |
| 有序            |    否    |      是      |     是     |           否           |
| 自定成员次序    |    可    |      否      |     否     |           否           |
| 初始化 n 个成员 |   O(n)   |  O(n log n)  | O(n log n) | 平均 O(n)、最坏 O(n^2) |
| 加入成员        | 分摊 O(1) |     O(n)     |  O(log n)  |  平均 O(1)、最坏 O(n)  |
| 移除成员        |   O(n)   |     O(n)     |  O(log n)  |  平均 O(1)、最坏 O(n)  |
| 查询成员        |   O(n)   |   O(log n)   |  O(log n)  |  平均 O(1)、最坏 O(n)  |
| 遍历成员        |   O(n)   |     O(n)     |    O(n)    |          O(m)          |
| 检测对象相等    |  O(n^2)  |     O(n)     |    O(n)    | 平均 O(n)、最坏 O(n^2) |
| 空间            |   O(m)   |     O(m)     |    O(n)    |          O(m)          |

在 ECMA-404 标准中，并没有规定对象中每个成员的键一定要唯一的，也没有规定是否需要维持成员的次序。

> 此处项目最终选择采用动态数组进行 Json 对象存储

### 解析思路

> 解析思路与 Json 数组解析基本一致，不过 Json 对象在解析 value 的同时也需要对 key 进行解析

## tutorial 7

### 生成器简介

生成器（generator）负责与解析器相反的事情，就是把树形数据结构转换成 JSON 文本。这个过程也被称为「字符串化（stringify）」。

相对于解析器，通常生成器更容易实现，而且生成器几乎不会造成运行时错误。因此，生成器的 API 设计为以下形式，直接返回 JSON 的字符串：

```c
char* lept_stringify(const lept_value* v, size_t* length);
```

`length` 参数是可选的，它会存储 JSON 的长度，传入 `NULL` 可忽略此参数。使用方需负责用 `free()` 释放内存。

为了简单起见，我们不做换行、缩进等美化（prettify）处理，因此它生成的 JSON 会是单行、无空白字符的最紧凑形式。

#### 生成与解析器关系描述

![](images/parse_stringify.png)

> 除了最简单的生成器功能，有一些 JSON 库还会提供一些美化功能，即加入缩进及换行。另外，有一些应用可能需要大量输出数字，那么就需要优化数字的输出。这方面可考虑 C++ 开源库 **[double-conversion](https://github.com/google/double-conversion)**，以及 [RapidJSON 代码剖析（四）：优化 Grisu](https://zhuanlan.zhihu.com/p/20092285)

## tutorial 8

> 从这里开始 Milo Yip 大佬停止了更新

在大佬提供的一系列接口函数的基础上，我对部分接口进行了修改，并完成了 Json 库的最后实现。这部分在 **[my leptjson 项目文档](leptjson.md)** 进行了详细描述，此处不再赘述。

因为之后会再使用 modern C++ 语法去实现一个名为 mJson 的 Json 解析器，而本项目仅用于加深对于 Json 解析思路的理解，故再开发完成后仅进行了自带的 560 例测试用例的测试和 Valgrind 检测出内存问题的修复，未继续进行优化方面的尝试。

## 感悟与收获

本项目第一行代码于 2023 年 4 月 9 日敲下，到今天 4 月 15 日刚好过去了一周。到目前为止，项目程序的开发业已完成，测试全部通过、文档和教程整理也是基本定型，我想也是时候去好好沉淀下这一周以来的收获了。

mJson 这个项目来自我对于我对大 WebServer 框架项目的构思。在我的设想中，我将会采用 moden C++ 语法、模块化实现构建一个高性能 WebServer 所需要的全部基础设施。（包括但不限于网络库、Json 解析、常用服务层及音视频编解码协议解析、后端数据库连接池（同时支持 redis 和 mysql 连接操作）、高性能线程池等等）此后，在这些基础设施的支持下，我将逐步完善和实现一个高性能 WebServer。

出于此种考虑，在我参考游双《Linux 高性能服务器编程》完成 WebServer 0.01 后，我便开始了对于 Json 库实现路径的搜索。最初，我找到了 Jsoncpp、RapidJson 等市面上常用的 Json 库，打算就此仿写一个 Json 库就算完结。可还没等我去仿照实现，我在使用和熟悉其操作时便感受到相当大的不适，似乎其接口定义并不优雅。于是我转而使用 nlohmann Json 这样一个仅有头文件，modern C++ 风格的 Json 库，与此同时也是发现了 Milo Yip 大神的这个 Json教程。

说来也巧，在我搜到 Milo Yip 大神这个教程后，我回去翻了翻我的知乎收藏夹，果不其然其也在我的收藏之中，这更是激发了我的兴趣。于是我便搜到了这个 Github 仓库，刚读完第一个 tutorial 便直呼找对了，之后更是一发不可收拾，基本按照 tutorial 的指引以每天两个tutorial 的速度完成了开发，因为内存检测工具不是特别熟，所以内存错误修复可能多花了点时间，直到今天才完成。

在我看来，在这个项目中其他的一些东西可能相对于 Json 的实现本身更为重要。首当其冲的无疑就是测试。在这个项目之前，我并没系统的进行过测试相关知识的学习。一直以来我都时以手动调试程序跑通作为最高目标。而在这个项目中，其在第一个 tutorial 中就实现了一个简单的测试框架，并提供了较多的测试用例用于验证程序执行情况。正巧这学期我选修了《高可信软件工程》，在课程中安排了足够的作业让我去尝试各种测试方法和测试工具，于是在我看来这将会是一个绝佳的机会去完成关于测试的系统性学习，以支撑我后面宏大构思中的程序代码质量。

除此之外，在调试阶段出现的 segment fault 以及 Valgrind 报出的诸多内存问题也是给我留下了很深刻的印象。从一开始的不知所措，到此后的熟练使用 gdb 进行调试，从迷迷糊糊指针绕来绕去到此后的内存模型逐渐清晰，这无疑也是一个不错的进步。而对 Vim 以及 Linux 命令行操作的进一步熟悉可能也是一个另外的收获吧，从当初感觉到相当不顺，到现在的如臂使指，终究是熟能生巧。

最后，可能对于字符编码的理解也可以算一点收获吧。经过 Json 字符串解析的实现，我熟悉了Unicode 和 UTF-8 编码，在对于字符编码的扩展整理中，我理清了常用字符编码如 ASCII、GB2312、Unicode、UTF-8/16/32 等的实现与转换，也是对于 modern C++ 的编码支持有了一些了解。

总而言之，这一周的夜以继日，还是相当值得的。
