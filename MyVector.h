#include <iostream>
#include <initializer_list>
#include <assert.h>
#include <exception>
#include "MyException.h"
using namespace std;

/****************************************************************************************************************************************************************************************
 *                                                                          模板函数的声明和定义要放在同一个文件下
 ******************************************************************************************************************************************************************************************/

// 默认构造函数的元素个数
#define VECTOR_SIZE_DEFAULT 16


//迭代器
// template <class T>
// class Iterator
// {
// public:
//     Iterator(T* ptr) : myData(ptr) { }
//     T& operator*() const { return *myData; }
//     Iterator& operator++() { ++myData; return *this; }
//     bool operator==(const Iterator &rhs) const { return this->myData == rhs.myData; }
//     bool operator!=(const Iterator &rhs) const { return !(*this == rhs); }
// protected:
//     T* myData; 
// };


//使用模板
template<typename T>
class MyVector
{
public:
    //using iterator = Iterator<T>;
    using iterator = T*;
    using ptr_different = ptrdiff_t;

public: 

    // 构造函数
    //这里必须使用operator new来分配空间而不是new operator，因为new operator是调用operator new分配空间后再调用T的默认构造函数初始化每个对象，如果T没有定义默认构造函数，就会报错
    //因此，这里必须使用operator new来申请内存，然后搭配placement new来生成对象
    //对应地，析构函数也要进行一些判断
    
    /*  使用new operator的构造函数
    MyVector() : myData(new T[VECTOR_SIZE_DEFAULT]{}), mySize(VECTOR_SIZE_DEFAULT), myMemSize(sizeof(T) * VECTOR_SIZE_DEFAULT) { }
    MyVector(size_t count) : myData(new T[count] {}), mySize(count), myMemSize(sizeof(T) * count) { }
    MyVector(size_t count, const T& value);
    MyVector(const MyVector& source);
    MyVector(initializer_list<T> initList);
    */

    // 使用operator new和placement new的构造函数
    MyVector() : myData(operator new(sizeof(T) * VECTOR_SIZE_DEFAULT)), mySize(VECTOR_SIZE_DEFAULT), myMemSize(sizeof(T) * VECTOR_SIZE_DEFAULT) { new (myData) T(0); }
    MyVector(size_t count) : myData(operator new(sizeof(T) * count)), mySize(count), myMemSize(sizeof(T) * count) { new(myData) T(0); }
    MyVector(size_t count, const T& value) : myData(operator new(sizeof(T) * count)), mySize(count), myMemSize(sizeof(T) * count)
    {
        //placement new
        new(myData) T(0);

        while (count-- >= 0)
        {
            myData[count] = value;
        }
    } 
    MyVector(const MyVector& source) : myData(operator new(source.myMemSize)), mySize(source.mySize), myMemSize(source.myMemSize)
    {
        new(myData) T(0);
        //这里用memcpy直接拷贝对POD类型来说是比较快的，但是如果是自定义类型，特别是包含一些指针成员的时候
        //这是因为：memcpy只能进行浅拷贝，而非POD的数据通常需要深拷贝，也就是说不仅要复制对象本身，还要复制对象所引用或指向的其他对象
        //如果使用memcpy复制非POD类型的数据，那么复制后的对象将与原对象共享同一块动态分配的内存。当其中一个对象被销毁时，它会释放这块内存
        //导致另一个对象的指针变成悬空指针，如果再次访问这个悬空指针，就会发声未定义行为。
        memcpy(myData, source.myData, source.myMemSize);
    }
    MyVector(initializer_list<T> initList) : myData((T *)operator new(sizeof(T) * initList.size())), mySize(initList.size()), myMemSize(sizeof(T) * initList.size())
    {
        new(myData) T(0);
        memcpy(myData, initList.begin(), myMemSize);
    }

    // 可变参数构造函数
    template <typename... Args>
    MyVector(Args ... args) : myData(new T[sizeof... (args)] {}), mySize(0)
    {   
        RecursionFunc(args...);
        myMemSize = sizeof(T) * mySize;
    }

    // 析构函数：析构函数应该保证不会抛出异常，否则会导致资源泄漏或其他问题，所以这里用noexcept
    virtual ~MyVector() noexcept
    {
        //首先判断有没有内存
        if (myData)
        {
            myData->~T();           //先调用T的析构函数销毁对象，析构函数只销毁对象，不会释放内存
            operator delete[] (myData);   //operator new的内存空间用operator delete[] 释放
            myData = NULL;           //避免野指针
        }
    }
    
    //重载运算符
    T operator[](size_t index)
    {
        assert(index < mySize);
        return myData[index];
    }
    T operator[](size_t index) const
    {
        assert(index < mySize);
        return myData[index];
    }
    bool operator==(const MyVector& rhs) const
    {
        if (mySize != rhs.mySize)
        {
            return false;
        }
        T* ptr1 = myData;
        T* ptr2 = rhs.myData;
        for (; ptr1 != &myData[mySize]; ++ptr1, ++ptr2)
        {
            if (*ptr1 != *ptr2)
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const MyVector& rhs) const
    {
        return !(*this == rhs);
    }
   
    //迭代器函数
    iterator begin() { return &myData[0]; }
    iterator end() { return &myData[mySize]; }

    //下标索引
    T& at(size_t pos) throw(std::exception)
    { 
        if (pos >= mySize)
        {
            throw std::exception();
        }
        return myData[pos]; 
    }
    const T& at(size_t pos) const throw(std::exception)
    { 
        if (pos >= mySize)
        {
            throw std::exception();
        }
        return myData[pos]; 
    }

    //返回最后一个值
    T& back()
    {
        try {
            if (this->mySize > 0)
            {
                return myData[mySize - 1];
            }
            else
            {
                throw RangeException();
            }
        }catch (const RangeException& e)
        {
            cerr << e.what() << endl;
        }
        
    }
    
    //返回第一个值
    T& front()
    {
        return myData[0];
    }

    //清空函数，对数组中的每个值做析构函数
    void clear() 
    {
        while (--mySize != 0)       //size_t 不能判断 >= 0，因为是无符号的整形
        {
            myData[mySize].~T();
        }
        this->mySize = 0;
    }

    //返回指向向量中第一个元素的指针
    T* data() const
    { 
        if (mySize != 0)
        {
            return myData;
        }
        else
        {
            return NULL;            //TODO
        }
    }

    //判断元素个数
    size_t size() const { return mySize; }
        
    //判空函数
    bool empty() const { return mySize == 0; }

    //交换函数
    void swap(MyVector& right)
    {
        //交换头指针
        T *temp = right.myData;
        right.myData = this->myData;
        this->myData = temp;

        size_t tempSize = right.mySize;
        right.mySize = this->mySize;
        this->mySize = tempSize;

        tempSize = right.myMemSize;
        right.myMemSize = this->myMemSize;
        this->myMemSize = tempSize;
    }

    void swap(MyVector& left, MyVector& right)
    {
        left.swap(right);
    }

    //重分配空间函数
    void resize(size_t newSize)
    {
        if (newSize < mySize)
        {
            T* temp = myData + newSize;
            while (temp != myData + mySize)
            {
                temp->~T();
                //delete temp;
                temp++;
            }
        }
        else if (newSize > mySize)
        {
            T* tempData = new T[newSize] {};
            memcpy(tempData, myData, mySize * sizeof(T));
            delete[] myData;
            myData = tempData;
        }
        mySize = newSize;
        myMemSize = sizeof(T) * newSize;
    }

    // 添加元素
    void push_back(const T& value)
    {
        if ((mySize + 1) * sizeof(T) >= myMemSize)   //超内存，要申请更大的新内存
        {
            T* tempData = new T[mySize * 2] {};
            memcpy(tempData, myData, myMemSize);
            delete[] myData;
            myData = tempData;
            myMemSize *= 2;
        }
        myData[mySize] = value;
        mySize++;
    }

    //删除最后一个元素
    void pop_back()
    {
        try {
            if (this->mySize > 0)
            {
                myData[--mySize].~T(); 
            }
            else
            {
                throw RangeException();
            }
        }catch (const RangeException& e)
        {
            cerr << e.what() << endl;
        }  
    }

    //删除函数
    iterator erase(iterator first, iterator last)
    {
        assert(first >= begin() && last >= begin() && first < last && last <= end());
        ptr_different eraseSize = last - first;
        ptr_different lenOfTail = end() - last;
        for (; lenOfTail != 0; --lenOfTail)
        {
            T* temp = last - eraseSize;
            *temp = *(last++);
        }
        mySize -= eraseSize; 
        return first;
    }

    iterator erase(iterator position)
    {
        return erase(position, position + 1);
    }

    //插入函数
    iterator insert(iterator position, const size_t& n, const T& value)
    {
        size_t resMem = (myMemSize - sizeof(T) * mySize) / sizeof(T);
        size_t needCount = n;
        if (resMem >= needCount)
        {
            for (; needCount != 0; --needCount)
            {
                myData[mySize++] = value;
            }
        }
        else
        {
            T *tempData = new T[(mySize + needCount) * 2] {};
            copy(begin(), end(), tempData);
            for (; needCount != 0; --needCount)
            {
                tempData[mySize++] = value;
            }
            delete[] myData;
            myData = tempData;
            myMemSize = (mySize + needCount) * 2 * sizeof(T);
        }
        return myData;
    }

    iterator insert(iterator position,  const T& value)
    {
        return insert(position, 1, value);
    }


    
private:
    //可变参数递归取值
    template<typename... Args>
    void RecursionFunc(T first, Args ... args)
    {
        myData[mySize++] = first;
        RecursionFunc(args...);
    }
    // 递归退出函数
    void RecursionFunc() {}

private:
    T *myData;
    size_t mySize;
    size_t myMemSize;
};