#include<iostream>
int main()
{
    double a = 4.0;
    std::cout << "Type of m_val: " << typeid(a).name() << std::endl;
    return 0;

}