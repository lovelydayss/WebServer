# My WebServer

a simple webserver program for C++ backend and Audio ...

**I can do all thing !!!**

&nbsp;

## 学习之路

### 文档

|              文档名称              | 简要描述                                                                                          |    完成情况    |                         位置链接                         |
| :---------------------------------: | ------------------------------------------------------------------------------------------------- | :-------------: | :------------------------------------------------------: |
|         WebServer 0.01 文档         | 对于游双《Linux 高性能服务器编程》书籍的阅读总结，包括对于其最后示例即项目 WebServer 0.01 的整理 | To be completed |                    WebServer 0.01/doc                    |
| Doc of My Json library ( leptjson ) | 参考 Milo Yip 《从零开始的 JSON 库教程》实现 Json 库的文档，包含支持语法，接口定义，测试报告等    |    Complete    | [leptjson.md](src/base/JsonParser/leptjson/doc/leptjson.md) |

### 博客

|        博客名称        | 简要描述                                                                      | 完成情况 |                          位置链接                          |
| :---------------------: | ----------------------------------------------------------------------------- | :------: | :--------------------------------------------------------: |
| Milo Yip 教程整理与扩展 | Milo Yip 《从零开始的 JSON 库教程》各 tutorial 整理、相关知识扩展、思考总结等 | Complete | [tutorials.md](src/base/JsonParser/leptjson/doc/tutorials.md) |
|   字符编码与 C++ 处理   | 常用字符集、字符编码规范及 C++字符编码支持整理                                | Complete |        [unicode.md](mind/character-coding/unicode.md)        |

&nbsp;

## 组成部件

### 基础组件

|      名称      | 介绍                                                                         |        位置        |     完成情况     |       测试       |                       仓库位置                       |
| :------------: | ---------------------------------------------------------------------------- | :-----------------: | :---------------: | :---------------: | :--------------------------------------------------: |
|   ThreadPool   | A high performance thread pool                                               |   base/ThreadPool   |     Complete     |  To be completed  | [ThreadPool](https://github.com/lovelydayss/ThreadPool) |
|   JsonParser   | A moden C++ style Json parser                                              |   base/JsonParser   | Under development | Under development |                       not yet                       |
| ConnectionPool | A connection pool that supports Redis and MySQL                              | base/ConnectionPool |  To be completed  |  To be completed  |                       not yet                       |
|      Net      | A network framework library implemented using the remaining basic components |       ......       |  To be completed  |  To be completed  |                       not yet                       |

&nbsp;

### WebServer

| 版本名称 | 介绍                                                                                                                                           | 位置           |     完成情况     |      测试      |
| :------: | ---------------------------------------------------------------------------------------------------------------------------------------------- | -------------- | :---------------: | :-------------: |
|  v0.01  | 参考游双《Linux 高性能服务器编程》中最后示例实现的基于线程池的 HTTP 服务器。其中线程池使用部件 ThreadPool，其余程序代码与原著中程序保持一致。 | WebServer 0.01 |     Complete     |        -        |
|   v0.1   | 在 WebServer 0.01 基础上改用 Modern C++ 语法开发，对于 HTTP 协议解析由状态机改用正则匹配方法，支持更多样请求方法。                             | WebServer 0.1  | Under development | To be completed |
|   v1.0   | 在 WebServer 0.1 基础上使用 JsonParser 组件库支持 Json 格式请求体传输，使用 ConnectionPool 组件库支持 Redis、MySQL 数据库连接与操作。         | WebServer 1.0  |  To be completed  | To be completed |
|   v1.1   | 在 WebServer 0.2 基础上将对于 HTTP 协议解析交由组件库处理，基本完成对于 HTTP 协议的支持，支持更多服务层协议 ........                          | WebServer 1.1  |  To be completed  | To be completed |

&nbsp;

## 更新记录

**2023-3-25** 首次更新，在已完成高性能线程池基础上创建项目，添加 README.md   [git-commit](https://github.com/lovelydayss/webserver)

**2023-4-6** 更新，部分实现游双《Linux 高性能服务器编程》最后示例服务器程序，即组成部件 webserver 0.01。  [git-commit](https://github.com/lovelydayss/WebServer/commit/61a529fd43e28f94f23fdd7be09b5b337ea16990)

**2023-4-8** 更新，完成游双《Linux 高性能服务器编程》最后示例服务器程序，使用 gdb 调试跑通，可实现对于 template 文件目录下文件的访问。更新 Readme 中组成部件部分，规划项目下一步开发方向  [git-commit](https://github.com/lovelydayss/WebServer/commit/aa693dc7abaf539a41da01b9b282a2c87349f242)

**2023-4-9** 更新，确定 My Json 库的开发思路。即在 nlohmann Json 库的接口规范下完成自己的 tiny Json 库开发，保证可以使用 nlohmann Json 对自己 tiny Json 库进行无缝替代 [git-commit](https://github.com/lovelydayss/WebServer/commit/c65e08ed281660e47da46dd1400594310ab07d79)

**2023-4-10** 更新，开始写 Milo Yip 大佬的《从零开始的 JSON 库教程》，写完了前两个 tutorial，即 JsonParser 的 null，true，false，number 等类型解析。之后几天的更新也将会集中在 JsonParser 库的实现上 [git-commit](https://github.com/lovelydayss/WebServer/commit/c9c64d7d9ac5a154769f6c22279f8295aec79f58)

**2023-4-14** 更新，完成《从零开始的 JSON 库教程》的全部 tutorial（包括教程中未实现的 tutorial8 及其扩展），并跑通所有测试用例。下一步将进行文档的整理，整理完成后将进行满足  nlohmann Json 接口规范的 modern C++ Json 库开发。 [git-commit](https://github.com/lovelydayss/WebServer/commit/ea3ec4e560c0c118842196876bcbdacefdb56253)

**2023-4-15** 更新，完成 My leptjson 文档的整理，使用 Valgrind 工具进行内存测试，修正部分内存问题。参考 Milo Yip《从零开始的 JSON 库教程》实现 Json 库的开发告一段落，接下来将补齐对于该教程的扩展整理（即博客 Milo Yip 教程整理与扩展） [git-commit](https://github.com/lovelydayss/WebServer/commit/96c7f8de39b2466ecb5b95dbec12b2310e7a0438)

**2023-4-16** 更新，重构部分代码，修正全部内存问题。完成教程的扩展整理（即博客 Milo Yip 教程整理与扩展），接下来将进行 mJson （即满足  nlohmann Json 接口规范的 modern C++ Json 库）的开发 [git-commit](https://github.com/lovelydayss/WebServer/commit/a6c4489eb93bc526537774b0650c019491e00535)

&nbsp;
