#include "yuan_config.hpp"
// #include " yuan_log.cpp"
#include <yaml-cpp/yaml.h>

// yuan::ConfigVar<int>::ptr g_int_value_config = yuan::Config::Lookup("system.port", (int)8080, "system port");
// yuan::ConfigVar<float>::ptr g_int_valuex_config = yuan::Config::Lookup("system.port", (float)8080, "system port");
// yuan::ConfigVar<float>::ptr g_float_value_config = yuan::Config::Lookup("system.value", (float)10.2f, "system value");
// yuan::ConfigVar<std::vector<int> >::ptr g_intvec_value_config = yuan::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int_vec");
// yuan::ConfigVar<std::list<int> >::ptr g_intlist_value_config = yuan::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int_list");
// yuan::ConfigVar<std::set<int> >::ptr g_intset_value_config = yuan::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int_set");
// yuan::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = yuan::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int_uset");
// yuan::ConfigVar<std::map<std::string,int> >::ptr g_str_int_map_value_config = yuan::Config::Lookup("system.str_int_map", std::map<std::string,int>{{"key",1}}, "system str int map");
// yuan::ConfigVar<std::unordered_map<std::string,int> >::ptr g_str_int_umap_value_config = yuan::Config::Lookup("system.str_int_umap", std::unordered_map<std::string,int>{{"key",1}}, "system str int umap");

// void print_yaml(const YAML::Node &node, int level)
// {
//     if (node.IsScalar())
//     {
//         YUAN_LOG_INFO(YUAN_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
//     }
//     else if (node.IsNull())
//     {
//         YUAN_LOG_INFO(YUAN_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
//     }
//     else if (node.IsMap())
//     {
//         for (auto it = node.begin(); it != node.end(); ++it)
//         {
//             YUAN_LOG_INFO(YUAN_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
//             print_yaml(it->second, level + 1);
//         }
//     }
//     else if (node.IsSequence())
//     {
//         for (size_t i = 0; i < node.size(); ++i)
//         {
//             YUAN_LOG_INFO(YUAN_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
//             print_yaml(node[i], level + 1);
//         }
//     }
// }

// void test_yaml()
// {
//     YAML::Node root = YAML::LoadFile("/home/bob/桌面/c++_code/yuan_fiber/test11.yaml");
//     print_yaml(root,0);

//     YUAN_LOG_INFO(YUAN_LOG_ROOT()) << root.Scalar();
// }

// void test_config()
// {
//     YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "before: " << g_int_value_config->getValue();// getValue返回T类型的常量
//     YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "before: " << g_float_value_config->toString();


// //顺序容器
// #define XX(g_var, name, prefix)\
//     { \
//         auto& v = g_var->getValue();\
//         for (auto &i : v)\
//         {\
//             YUAN_LOG_INFO(YUAN_LOG_ROOT()) << #prefix " " #name ": "<< i; \
//         } \
//         YUAN_LOG_INFO(YUAN_LOG_ROOT()) << #prefix " " #name " yaml: "<< g_var->toString();\
//     }

// #define XX_M(g_var, name, prefix)\
//     { \
//         auto& v = g_var->getValue();\
//         for (auto &i : v)\
//         {\
//             YUAN_LOG_INFO(YUAN_LOG_ROOT()) << #prefix " " #name ": {" << i.first << " - " << i.second<< "}";\
//         } \
//         YUAN_LOG_INFO(YUAN_LOG_ROOT()) << #prefix " " #name " yaml: "<< g_var->toString();\
//     }

//     XX(g_intvec_value_config,int_vec,before);
//     XX(g_intlist_value_config,int_list,before);
//     XX(g_intset_value_config,int_set,before);
//     XX(g_int_uset_value_config,int_uset,before);
//     XX_M(g_str_int_map_value_config,str_int_map,before);
//     XX_M(g_str_int_umap_value_config,str_int_umap,before);

// /****************************************** after *******************************************/
//     YAML::Node root = YAML::LoadFile("/home/bob/桌面/c++_code/study_sylar/test11.yaml");
//     yuan::Config::LoadFromYaml(root);

//     YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
//     YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "after: " << g_float_value_config->toString(); 

//     XX(g_intvec_value_config,int_vec,after);
//     XX(g_intlist_value_config,int_list,after);
//     XX(g_intset_value_config,int_set,after);
//     XX(g_int_uset_value_config,int_uset,after);
//     XX_M(g_str_int_map_value_config,str_int_map,after);
//     XX_M(g_str_int_umap_value_config,str_int_umap,after);
    
// }
// 从yaml文件中设置

class Person
{
public:
    Person(){}
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name << " age=" << m_age << " sex=" << m_sex << "]";
        return ss.str();
    }
    bool operator==(const Person& p) const
        {
            return p.m_age == m_age && p.m_name == m_name && p.m_sex == m_age;
        }
};

namespace yuan
{
    template <>
    class MyLexicalCast<std::string, Person>
    {
    public:
        // string -> Person类型转化
        Person operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };
    template <>
    class MyLexicalCast<Person, std::string>
    {
    public:
        // Person -> string 类型转化
        std::string operator()(const Person& p)
        {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            node["sex"] = p.m_age;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}

yuan::ConfigVar<Person>::ptr g_person = yuan::Config::Lookup("class.person", Person(), "system person");
yuan::ConfigVar<std::map<std::string,Person>>::ptr g_person_map = yuan::Config::Lookup("class.map", std::map<std::string,Person>(), "system person");
yuan::ConfigVar<std::map<std::string,std::vector<Person>>>::ptr g_person_vec_map = yuan::Config::Lookup("class.vec_map", std::map<std::string,std::vector<Person>>(), "system person");
void test_class()
{
    YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_PM(g_var,prefix)\
{\
    auto m = g_var->getValue();\
    for(auto i : m)\
    {\
        YUAN_LOG_INFO(YUAN_LOG_ROOT()) << #prefix << ": " << i.first << " - " << i.second.toString();\
    }\
    YUAN_LOG_INFO(YUAN_LOG_ROOT()) << #prefix << ": size= " << m.size();\
}

    g_person->addListener(10,[](const Person& old_value,const Person& new_value)
    {
        YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "old_value= " << old_value.toString() << " new_value= " << new_value.toString();
    });

    XX_PM(g_person_map,"class.map before");
    YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/bob/桌面/c++_code/study_sylar/test11.yaml");
    yuan::Config::LoadFromYaml(root);

    YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();  
    YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "after: " << g_person_vec_map->toString();

    XX_PM(g_person_map,"class.map after");
}
int main()
{
    // g++ test_config.cpp -lyaml-cpp -o test_config

    // YUAN_LOG_INFO(YUAN_LOG_ROOT()) << g_int_value_config->getValue();
    // YUAN_LOG_INFO(YUAN_LOG_ROOT()) << g_int_value_config->toString();
    // YUAN_LOG_INFO(YUAN_LOG_ROOT()) << g_float_value_config->getValue();
    // YUAN_LOG_INFO(YUAN_LOG_ROOT()) << g_float_value_config->toString();
    //test_config();
    test_class();
    return 0;
}