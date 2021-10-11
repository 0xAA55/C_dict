
# C dictionary

A collection written in C.

## Intro

Using a dictionary based on a hash table written in C language, the query time complexity is O(1). When the amount of data to be stored is large, the query performance can significantly exceed the array.

## 介绍

使用C语言编写的基于哈希表实现的字典，查询的时间复杂度是O(1)，需要存储的数据量很大的时候，查询的性能就可以明显超过数组。

## Usage

Just read the comments in `dict.h` , it describes the interface of this library.
The dictionary uses `key` and `value` to organize data.
The `key` type is `void*`, you can use strings, integers, or anything (for example, a `struct`) as the `key`.
By default, the `key` is strings. But if you want to use other types, you need to provide the following:
* The function that calculates the `hash` for your `key` should be able to avoid collisions and be fast, but it does not need to strictly avoid collisions. Speed comes first.
* Comparison function, used to compare whether two `keys` are exactly the same.
* Function to copy `key`. By default, the `key` is a string, and the behavior of this function by default (if not specified) is to allocate memory to store the contents of the string, then copy the string into the newly allocated memory.
* Delete `key` function, used to free memory. By default, the `key` is a string stored in allocated memory, and so the behavior of this function by default (if not specified) is `free()`.
* Delete `value` function, used to free memory. By default, if not specified this function, `free()` will be used to free `value`.

## 用法

直接阅读`dict.h`里的注释即可。字典使用`key`和`value`组织数据。
`key`的类型是`void*`，你可以使用结构体来做`key`，也可以用整数类型或者字符串。
默认情况下，`key`被视作字符串类型。但如果你使用了别的类型，你需要提供以下内容：
* 针对你的`key`计算哈希的函数，要能避免碰撞，并且要快，但并不需要严格避免碰撞，速度第一。
* 比较函数，用于对比两个`key`是否完全相同。
* 复制`key`的函数。默认情况下，`key`是字符串，这个函数的行为是调用`malloc()`分配内存并调用`memcpy()`来复制字符串内容。
* 删除`key`的函数，用于释放内存。默认情况下，`key`是专门分配了内存的字符串，然后这个函数的行为是调用`free()`来释放字符串内存。
* 删除`value`的函数，用于释放内存。默认情况下，`value`被视作使用`malloc()`分配的内存，所以默认的函数行为相当于调用`free()`。
