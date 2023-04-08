# My WebServer

a simple webserver program for C++ backend and Audio ...

**I can do all thing !!!**

&nbsp;

## 学习之路

| 博客/文档名称  | 简要描述 | 位置链接 |
| -------------- | -------- | -------- |
| WebServer 0.01 | xxx      | xxx      |

&nbsp;

## 组成部件

### 基础组件

|      名称      |                           介绍                           | 位置                |    完成情况    | 测试            | 仓库位置                                             |
| :------------: | :-------------------------------------------------------: | ------------------- | :-------------: | --------------- | ---------------------------------------------------- |
|   ThreadPool   |              a high performance thread pool              | base/ThreadPool     |    completed    | To be completed | [ThreadPool](https://github.com/lovelydayss/ThreadPool) |
|   JsonParser   | a JSON parser implemented using a regular matching method | base/JsonParser     | To be completed | To be completed | not yet                                              |
| ConnectionPool |      a connection pool that supports Redis and MySQL      | base/ConnectionPool | To be completed | To be completed | not yet                                              |
|      Net      |                            a                             |                     |                |                 |                                                      |

### webserver

| 名称           | 介绍                                                                                                                                                     | 位置               | 测试            |
| -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------ | --------------- |
| WebServer 0.01 | 参考游双《Linux 高性能服务器编程》中最后示例实现的基于线程池的 HTTP 服务器。其中线程池使用部件 ThreadPool，其余程序代码与原著中程序保持一致。           | src/webserver 0.01 | To be completed |
| WebServer 0.1  | 在 WebServer 0.01 基础上改用 C++ 11 语法完成开发，对于 HTTP 协议解析采用正则匹配方法。                                                                   | src/webserver 0.1  | To be completed |
| WebServer 0.2  | 在 WebServer 0.1 基础上支持更多请求方法，使用 JsonParser 组件库支持 Json 格式请求体传输，使用 ConnectionPool 组件库支持 Redis、MySQL 数据库连接与操作。 | src/webserver 0.2  | To be completed |
| WebServer 1.0  | 在 WebServer 0.2 基础上将对于 HTTP 协议解析交由组件库处理，基本完成对于 HTTP 协议的支持........                                                         | src/webserver 1.0  | To be completed |

&nbsp;

## 更新记录

**2023-3-25** 首次更新，在已完成高性能线程池基础上创建项目，添加 README.md   [git-commit](https://github.com/lovelydayss/webserver)

**2023-4-6** 更新，部分实现游双《Linux 高性能服务器编程》最后示例服务器程序，即组成部件 webserver 0.01。  [git-commit](https://github.com/lovelydayss/WebServer/commit/61a529fd43e28f94f23fdd7be09b5b337ea16990)

**2023-4-8** 更新，完成游双《Linux 高性能服务器编程》最后示例服务器程序，使用 gdb 调试跑通，可实现对于 template 文件目录下文件的访问。更新 Readme 中组成部件部分，规划项目下一步开发方向  [git-commit](https://github.com/lovelydayss/WebServer/commit/aa693dc7abaf539a41da01b9b282a2c87349f242)

&nbsp;
