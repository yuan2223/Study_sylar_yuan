#include <iostream>
#include "yaml-cpp/yaml.h"

int main(int argc, const char *argv[])
{
     YAML::Node config = YAML::LoadFile("/home/bob/桌面/c++_code/study_sylar/log2.yaml");
     std::cout << config << std::endl;
}