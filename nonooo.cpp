#include<iostream>
#include<string>
// #include <coroutine>


class A
{
public:
    A(int age, std::string& name):m_age(age),m_name(name)
    {
        std::cout<< "调用A的构造函数" << std::endl;
        std::cout<< "name=" << m_name << std::endl;
        std::cout<< "age=" << m_age << std::endl;
    }

    ~A()
    {
        std::cout<< "调用A的析构函数" <<std::endl;
    }

private:
    int m_age;
    std::string m_name;
};

class B:public A
{
public:
    B(int age, std::string& name):A(age,name)
    {
        std::cout<< "调用B的构造函数" << std::endl;
    }
    ~B()
    {
        std::cout<< "调用B的析构函数" << std::endl;
    }
};


int main()
{
    int mm = 18;
    std::string nn = "张三";
    B b(mm,nn);
    return 0;

}

