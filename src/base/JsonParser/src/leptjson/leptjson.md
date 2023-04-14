# Milo Yip 《从零开始的 JSON 库教程》整理与思考

## 文档

### Json 语法


### 接口设计

### 测试

segment fault

### 使用示例

## Miloyip 大神教程整理与思考

### tutorial 3

#### 内存泄漏检测

`apt-get install valgrind`

linux 下使用 [valgrind](https://valgrind.org/)  工具

valgrind --leak-check=full  ./leptjson_test

#### 性能优化参考

1. 如果整个字符串都没有转义符，我们不就是把字符复制了两次？第一次是从 `json` 到 `stack`，第二次是从 `stack` 到 `v->u.s.s`。我们可以在 `json` 扫描 `'\0'`、`'\"'` 和 `'\\'` 3 个字符（ `ch < 0x20` 还是要检查），直至它们其中一个出现，才开始用现在的解析方法。这样做的话，前半没转义的部分可以只复制一次。缺点是，代码变得复杂一些，我们也不能使用 `lept_set_string()`。
2. 对于扫描没转义部分，我们可考虑用 SIMD 加速，如 [RapidJSON 代码剖析（二）：使用 SSE4.2 优化字符串扫描](https://zhuanlan.zhihu.com/p/20037058) 的做法。这类底层优化的缺点是不跨平台，需要设置编译选项等。
3. 在 gcc/clang 上使用 `__builtin_expect()` 指令来处理低概率事件，例如需要对每个字符做 `LEPT_PARSE_INVALID_STRING_CHAR` 检测，我们可以假设出现不合法字符是低概率事件，然后用这个指令告之编译器，那么编译器可能可生成较快的代码。然而，这类做法明显是不跨编译器，甚至是某个版本后的 gcc 才支持。

### tutorial 4

#### unicode 编码

### tutorial 5

动态数组实现数组

### tutorial 6

采用 map 实现对象

### tutorial 7

Json 生成器

### tutorial 8

这里好像大佬没有更新

看了看大佬在 tutorial 中提供的一系列接口函数

感觉就一片混乱

于是我就斗胆修改了下接口定义，并按照我的思路进行了实现

因为最后会使用 modern C++ 语言去实现一个 mJson 的 Json解析器，本项目仅用于对Json 解析思路进行理解，故最后未继续进行优化尝试

此外简单编写了部分测试用例

## 感悟与收获

aFASDGDFASHGBFS
