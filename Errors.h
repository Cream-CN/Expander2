#pragma once
//用于提升所有静态分析器给出的警告为错误
//By Cream-CN 20261017
//静态检出的UB
#pragma warning(error: 6011)//取消引用 NULL 指针“pointer-name”。
#pragma warning(error: 6029)//调用“函数名”时可能出现缓冲区溢出
#pragma warning(error: 6503)
#pragma warning(error: 6507)
#pragma warning(error: 28182)//新版本的空指针解引用
#pragma warning(error: 26822)
#pragma warning(error: 26823)
#pragma warning(error: 26430)
#pragma warning(error: 26404)
//静态检出的，可能引发UB的编码习惯问题
#pragma warning(warning: 26429)
#pragma warning(warning: 26431)
