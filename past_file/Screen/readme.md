### 通过lvgl底层驱动的screen

- 通过AI辅助完成
- 本模块仅辅助测试或者展示原有项目，例如显示摄像头画面，motion执行，motion学习，obj学习，obj检测

该分支仅用来测试lvgl基础界面与信号传递模式  

该分支有关lvgl的编译部分已提前编译为静态文件  

```bash
  Screen
  build
  lvgl
    env_support
    src
    lvgl_lib
      liblvgl.a(静态编译文件)
  CMakeLists.txt
  lv_conf.h
  main.cpp
  readme.md
  Screen_ui.cpp
  Screen_ui.hpp
  ThreadSafeQueue.hpp

  