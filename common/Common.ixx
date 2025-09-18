export module Common;

// 作为公共库，把大概率要引用的库import编译一下，别的项目只要依赖此库，再次import这些库就不需要再编译了
import std;
import "sys_defs.h";