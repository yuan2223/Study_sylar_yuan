#ifndef __YUAN_CONFIG_HPP__
#define __YUAN_CONFIG_HPP__

#include <sstream>
#include <string>
#include <memory>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include<vector>
#include<map>
#include<list>
#include<set>
#include<unordered_map>
#include<unordered_set>
#include<functional>

#include "yuan_thread.hpp"
#include "yuan_log.hpp"
#include "yuan_until.hpp"

namespace yuan
{
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name), m_description(description)
        {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }
        virtual ~ConfigVarBase() {}
        const std::string &getName() const { return m_name; }
        const std::string &getDescriptioin() const { return m_description; }

        virtual std::string toString() = 0;
        virtual bool fromString(const std::string &val) = 0;
        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_description;
    };

/***********************************************************************************************************/
/***********************************************************************************************************/
//vector
    template<class F,class T>
    class MyLexicalCast
    {
    public:
        //将 F 类型的数据转化成 T 类型的数据
        T operator() (const F& v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    template <class T>
    class MyLexicalCast<std::string, std::vector<T> >
    {
    public:
        // string -> vector<T> 类型转化
        std::vector<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];  //将node[i]插入到字符串流ss中
                vec.push_back(MyLexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };
    template <class T>
    class MyLexicalCast<std::vector<T>, std::string>
    {
    public:
        // vector<T> -> string 类型转化
        std::string operator()(const std::vector<T>& v)
        {
            YAML::Node node;
            for(auto& i : v)
            {
                node.push_back(YAML::Load(MyLexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
    };

    //list
    template <class T>
    class MyLexicalCast<std::string, std::list<T> >
    {
    public:
        // string -> list<T> 类型转化
        std::list<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];  //将node[i]插入到字符串流ss中
                vec.push_back(MyLexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };
    template <class T>
    class MyLexicalCast<std::list<T>, std::string>
    {
    public:
        // list<T> -> string 类型转化
        std::string operator()(const std::list<T>& v)
        {
            YAML::Node node;
            for(auto& i : v)
            {
                node.push_back(YAML::Load(MyLexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
    };

    //set
    template <class T>
    class MyLexicalCast<std::string, std::set<T> >
    {
    public:
        // string -> set<T> 类型转化
        std::set<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];  //将node[i]插入到字符串流ss中
                vec.insert(MyLexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };
    template <class T>
    class MyLexicalCast<std::set<T>, std::string>
    {
    public:
        // set<T> -> string 类型转化
        std::string operator()(const std::set<T>& v)
        {
            YAML::Node node;
            for(auto& i : v)
            {
                node.push_back(YAML::Load(MyLexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
    };

    //unordered_set
    template <class T>
    class MyLexicalCast<std::string, std::unordered_set<T> >
    {
    public:
        // string -> unordered_set<T> 类型转化
        std::unordered_set<T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); ++i)
            {
                ss.str("");
                ss << node[i];  //将node[i]插入到字符串流ss中
                vec.insert(MyLexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };
    template <class T>
    class MyLexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        // unordered_set<T> -> string 类型转化
        std::string operator()(const std::unordered_set<T>& v)
        {
            YAML::Node node;
            for(auto& i : v)
            {
                node.push_back(YAML::Load(MyLexicalCast<T,std::string>()(i)));
            }
            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
    };

    //map
    template <class T>
    class MyLexicalCast<std::string, std::map<std::string,T> >
    {
    public:
        // string -> map<T> 类型转化
        std::map<std::string,T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string,T> vec;
            std::stringstream ss;
            for (auto it = node.begin();it != node.end();++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),MyLexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };
    template <class T>
    class MyLexicalCast<std::map<std::string,T>, std::string>
    {
    public:
        // map<T> -> string 类型转化
        std::string operator()(const std::map<std::string,T>& v)
        {
            YAML::Node node;
            for(auto& i : v)
            {
                //node.push_back(YAML::Load(MyLexicalCast<T,std::string>()(i)));
                node[i.first] = YAML::Load(MyLexicalCast<T,std::string>()(i.second));
            }
            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
    };

    //unordered_map
    template <class T>
    class MyLexicalCast<std::string, std::unordered_map<std::string,T> >
    {
    public:
        // string -> unordered_map<T> 类型转化
        std::unordered_map<std::string,T> operator()(const std::string& v)
        {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string,T> vec;
            std::stringstream ss;
            for (auto it = node.begin();it != node.end();++it)
            {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),MyLexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };
    template <class T>
    class MyLexicalCast<std::unordered_map<std::string,T>, std::string>
    {
    public:
        // unordered_map<T> -> string 类型转化
        std::string operator()(const std::unordered_map<std::string,T>& v)
        {
            YAML::Node node;
            for(auto& i : v)
            {
                //node.push_back(YAML::Load(MyLexicalCast<T,std::string>()(i)));
                node[i.first] = YAML::Load(MyLexicalCast<T,std::string>()(i.second));
            }
            std::stringstream ss;
            ss<< node;
            return ss.str();
        }
    };
/***********************************************************************************************************/
/***********************************************************************************************************/

    // FromStr T operator()(const std::string&)  将字符串类型转化成所需要的类型
    // ToStr std::string operator()(const T&)    将 T 类型的数据转化成字符串类型
    template <class T, class FromStr = MyLexicalCast<std::string, T>, class ToStr = MyLexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;
        //当一个配置更改时，知道原来的值和新的值是什么
        typedef std::function<void (const T& old_value,const T& new_value)> on_change_cb;

        ConfigVar(const std::string &name, const T &default_value, const std::string &description = "") : ConfigVarBase(name, description), m_val(default_value)
        {
        }

        std::string toString() override
        {
            try
            {
                // 将非字符串数据转化成字符串数据
                //return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            }
            catch (std::exception &e)
            {
                YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "ConfigVar::toString exception"
                                                << e.what() << "convert: " << typeid(m_val).name() << "to string";
                // typeid(m_val).name()获取m_val的类型信息
            }
            return "";
        }

        bool fromString(const std::string &val) override
        {
            try
            {
                //m_val = boost::lexical_cast<T>(val); // 将字符串类型转成成员变量的类型
                setValue(FromStr()(val)); //将string 类型的数据转化成 T ，再赋值给m_val
            }
            catch (const std::exception &e)
            {
                YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "ConfigVar::toString exception"
                                                << e.what() << "convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue() const { return m_val; }
        void setValue(const T &v)
        {
            if(v == m_val)//原值等于新的值，没有变化
            {
                return;
            }
            for(auto& i:m_cbs)
            {
                i.second(m_val,v);
            }
            m_val = v;

        }
        std::string getTypeName() const override {return typeid(T).name();}

        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }
        void delListener(uint64_t key)
        {
            m_cbs.erase(key);
        }
        on_change_cb getListener(uint64_t key)
        {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }
        void clearListener()
        {
            m_cbs.clear();
        }

    private:
        T m_val;
        //变更回调函数组,function没有比较函数，当两个key相等时，两个function是一样的,要求key是唯一的
        std::map<uint64_t,on_change_cb> m_cbs;
    };

    class Config
    {
    public:
        typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        template <class T>
        //  告诉编译器这是类型名
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value, const std::string &description = "")
        {
            //auto it = s_datas.find(name);//s_datas的数据类型std::unordered_map<std::string, ConfigVarBase::ptr>
            auto it = GetDatas().find(name);
            if(it != GetDatas().end())
            {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if(tmp)
                {
                    YUAN_LOG_INFO(YUAN_LOG_ROOT()) << "Lookup name= " << name << "exist";
                    return tmp;
                }
                else
                {
                    YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "Lookup name= " << name << " exist but type not " 
                                                    << typeid(T).name() << " real type= " << it->second->getTypeName()
                                                    << " " << it->second->toString();
                    return nullptr;
                }
            }

            //  std::string::npos 是 std::string 类的静态成员，表示未找到指定字符的特殊值
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
            {
                // 找到了不再指定字符集中的字符
                YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "Lookup name invalid" << name;
                throw std::invalid_argument(name);
            }

            //  保存在map中
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            //auto it = s_datas.find(name);
            auto it = GetDatas().find(name);
            // 没有找到该类
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            // 强制转成智能指针                       c++11以前>>会被解析成右移运算符，在模板应用的场景下>>加空格区分
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &root);
        static ConfigVarBase::ptr LookupBase(const std::string& name);
        

    private:
        static ConfigVarMap& GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        }
    };

}

#endif //__YUAN_CONFIG_HPP__