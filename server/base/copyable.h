/**
* @description: copyable.h
* @author: YQ Huang
* @brief: 自定义实现可拷贝类
* @date: 2022/05/02 10:02:21
*/

#pragma once

namespace myserver {

/**
 * 所有继承copyable的子类都应该是值类型，也就是可以安全拷贝的
 * Protect 意味着该类无法产生对象，而只能派生子类
 */
class copyable {
protected:
    copyable() = default;
    ~copyable() = default;
};

}