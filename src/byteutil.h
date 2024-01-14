#pragma once

#include <qglobal.h>
#include <qbytearray.h>

namespace ByteUtil {

    /**
     * @brief 字节序交换
     * @tparam T 数值类型
     * @param data 转换目标数值
     * @param size 字节序交换大小
    */
    template<typename T>
    static void swapBits(T& data, size_t size = sizeof(T)) {
        for (size_t i = 0; i < size / 2; i++) {
            char* pl = (char*)&data + i;
            char* pr = (char*)&data + (size - i - 1);
            if (*pl != *pr) {
                *pl ^= *pr;
                *pr ^= *pl;
                *pl ^= *pr;
            }
        }
    }

    /**
     * @brief 字节序交换
     * @tparam T 数值类型
     * @param data 转换目标数值
     * @param size 字节序交换大小
    */
    template<typename T>
    static T swapBits2(T&& data, size_t size = sizeof(T)) {
        for (size_t i = 0; i < size / 2; i++) {
            char* pl = (char*)&data + i;
            char* pr = (char*)&data + (size - i - 1);
            if (*pl != *pr) {
                *pl ^= *pr;
                *pr ^= *pl;
                *pl ^= *pr;
            }
        }
        return data;
    }

    /**
     * @brief char*转指定数值类型（大端序）
     * @tparam T 数值类型
     * @param data 转换目标数值
     * @param src 原字节数组
     * @param srcSize 原字节数组大小
    */
    template<typename T>
    static void bitConvert(T& data, const void* src, int srcSize = sizeof(T)) {
        memcpy(&data, src, srcSize);
        swapBits(data, srcSize);
    }

    /**
     * @brief char*转指定数值类型（小端序）
     * @tparam T 数值类型
     * @param data 转换目标数值
     * @param src 原字节数组
     * @param srcSize 原字节数组大小
    */
    template<typename T>
    static void bitConvertL(T& data, const void* src, int srcSize = sizeof(T)) {
        memset(&data, 0, sizeof(T));
        memcpy(&data, src, srcSize);
    }

    /**
     * @brief 初始化QByteArray字节数组
     * @param a 字节数组列表
     * @return QByteArray
    */
    template<size_t N>
    QByteArray makeBytes(const uchar(&a)[N]) {
        return { (const char*)a, N };
    }

}
