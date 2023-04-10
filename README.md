# My WebServer

a simple webserver program for C++ backend and Audio ...

**I can do all thing !!!**

&nbsp;

## 学习之路

| 博客/文档名称  |                                             简要描述                                             | 完成情况        | 位置链接 |
| -------------- | :-----------------------------------------------------------------------------------------------: | --------------- | -------- |
| WebServer 0.01 | 对于游双《Linux 高性能服务器编程》书籍的阅读总结，包括对于其最后示例即项目 WebServer 0.01 的整理 | To be completed | xxxxx    |

&nbsp;

## 组成部件

### 基础组件

|      名称      | 介绍                                                                         |        位置        |     完成情况     |      测试      |                       仓库位置                       |
| :------------: | :--------------------------------------------------------------------------- | :-----------------: | :---------------: | :-------------: | :--------------------------------------------------: |
|   ThreadPool   | A high performance thread pool                                               |   base/ThreadPool   |     Completed     | To be completed | [ThreadPool](https://github.com/lovelydayss/ThreadPool) |
|   JsonParser   | A JSON parser implemented using a regular matching method                    |   base/JsonParser   | Under development | To be completed |                       not yet                       |
| ConnectionPool | A connection pool that supports Redis and MySQL                              | base/ConnectionPool |  To be completed  | To be completed |                       not yet                       |
|      Net      | A network framework library implemented using the remaining basic components |       ......       |  To be completed  | To be completed |                       not yet                       |

&nbsp;

### WebServer

| 名称           | 介绍                                                                                                                                           | 位置               |      测试      |
| -------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- | ------------------ | :-------------: |
| WebServer 0.01 | 参考游双《Linux 高性能服务器编程》中最后示例实现的基于线程池的 HTTP 服务器。其中线程池使用部件 ThreadPool，其余程序代码与原著中程序保持一致。 | src/webserver 0.01 | To be completed |
| WebServer 0.1  | 在 WebServer 0.01 基础上改用 Modern C++ 语法开发，对于 HTTP 协议解析由状态机改用正则匹配方法，支持更多样请求方法。                             | src/webserver 0.1  | To be completed |
| WebServer 0.2  | 在 WebServer 0.1 基础上使用 JsonParser 组件库支持 Json 格式请求体传输，使用 ConnectionPool 组件库支持 Redis、MySQL 数据库连接与操作。         | src/webserver 0.2  | To be completed |
| WebServer 1.0  | 在 WebServer 0.2 基础上将对于 HTTP 协议解析交由组件库处理，基本完成对于 HTTP 协议的支持........                                               | src/webserver 1.0  | To be completed |

&nbsp;

## 更新记录

**2023-3-25** 首次更新，在已完成高性能线程池基础上创建项目，添加 README.md   [git-commit](https://github.com/lovelydayss/webserver)

**2023-4-6** 更新，部分实现游双《Linux 高性能服务器编程》最后示例服务器程序，即组成部件 webserver 0.01。  [git-commit](https://github.com/lovelydayss/WebServer/commit/61a529fd43e28f94f23fdd7be09b5b337ea16990)

**2023-4-8** 更新，完成游双《Linux 高性能服务器编程》最后示例服务器程序，使用 gdb 调试跑通，可实现对于 template 文件目录下文件的访问。更新 Readme 中组成部件部分，规划项目下一步开发方向  [git-commit](https://github.com/lovelydayss/WebServer/commit/aa693dc7abaf539a41da01b9b282a2c87349f242)

**2023-4-9** 更新，确定 MyJson 开发思路。即在 nlohmann Json 的接口规范下完成自己的 tiny Json 开发，保证可以使用 nlohmann Json 对自己 tiny Json 进行无缝替代 [git-commit](https://github.com/lovelydayss/WebServer/commit/c65e08ed281660e47da46dd1400594310ab07d79)

**2023-4-10** 更新，开始写 miloyip 大佬的《从零开始的 JSON 库教程》，写完了前两个 tutorial，即 JsonParser 的 null，true，false，number 等类型解析。之后几天的更新也将会集中在JsonParser 库的实现上 [git-commit](https://github.com/lovelydayss/WebServer/commit/c9c64d7d9ac5a154769f6c22279f8295aec79f58)

&nbsp;
