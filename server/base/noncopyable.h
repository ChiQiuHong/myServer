/**
* @description: noncopyable.h
* @author: YQ Huang
* @brief: 自定义实现不可拷贝类
* @date: 2022/05/01 16:06:29
*/

#pragma once

namespace myserver {

/**
 * noncopyable类将拷贝构造函数和赋值运算符都指示删除，即不让编译器生成这两个函数
 * 这两个函数删除后，也就禁止了类的拷贝，所有继承了noncopyable的子类都不能被拷贝
 * Protect 意味着该类无法产生对象，而只能派生子类
 */
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;

};

}