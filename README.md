<<<<<<< HEAD
## 概述

该程序利用 CUDD 库处理二进制决策图（BDD）以表示和操作布尔表达式。它从 JSON 文件中读取变量信息和表达式，基于这些表达式构建 BDD，并从 BDD 中采样值。
服务器不会用

## 目录结构

```
eda230906/
|-- test.c
|-- Cjson.c
|-- Cjson.h
|-- build.sh
|-- run.sh
|-- README.md
```

- **build.sh**：编译程序的脚本。
- **README.md**：本文件，提供说明和信息。
=======
# CVcuddtest
该程序使用CUDD3.0.0库和cjson库
测试时请将输入文件输入到主函数 const char* jsonText = "..."; // 测试文件地址 中...处
>>>>>>> a835fc59e44876d5bec7cad2ac71b8c75dcdf601
