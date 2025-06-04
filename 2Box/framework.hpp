#pragma once
// 在模块接口单元里import "framework.h" 只能在当前文件看到windows.h定义的宏
// 这样在分离的实现文件里就看不到宏了。因此模块实现文件里需要再次import "framework.h"才行
// 但是当前IntelliSense智能感知还是有bug，再次import还是看不到宏，因为它可能认为framework.h已经导入过了，不需要再次处理
// 因此特意用这个名字不一样的文件伺候IntelliSense
// 使用时，需要在实现单元中这样写:
// #ifndef _FRAMEWORK_H_
// import "framework.hpp";
// #endif
// 对编译器来说，因为已经import "framework.h"过了，所以能看到_FRAMEWORK_H_宏，不会再次导入framework.hpp
#include "framework.h"
