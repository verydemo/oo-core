完整编译需参考 onlyoffice 官方仓库 build_tools ： [https://github.com/ONLYOFFICE/build_tools.git](https://github.com/ONLYOFFICE/build_tools.git)

#### core 编译

* debug模式
  ```
  make -f build_debug.makefile_linux_64 -j 8
  ```
* relase模式
  ```
  make -f build.makefile_linux_64 -j 8
  ```

#### core 代码调试

1. 使用debug模式编译出可执行文件x2t;
2. 安装gdb调试工具; gdb命令文档：[https://linux265.com/course/linux-command-gdb.html](https://linux265.com/course/linux-command-gdb.html)
3. 执行命令进入调试，加断点等等
   ```
   gdb -q --args ./x2t test1.docx test1.bin
   ```
