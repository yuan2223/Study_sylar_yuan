#include "yuan_config.hpp"
#include "yuan_until.hpp"


namespace yuan
{
    //Config::ConfigVarMap Config::s_datas;

    // s_datas 的数据类型 std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    static yuan::Logger::ptr g_logger = YUAN_LOG_NAME("system");
    ConfigVarBase::ptr Config::LookupBase(const std::string& name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }
/************************************************************************************************************************/
/************************************************************************************************************************/
/************************************************************************************************************************/

    static void ListAllMember(const std::string &prefix, const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos)
        {
            YUAN_LOG_ERROR(YUAN_LOG_ROOT()) << "Config invalid name" << prefix << ":" << node;
            return;
        }
        // 有效的
        output.push_back(std::make_pair(prefix, node));
        if (node.IsMap())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                // 递归调用
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node &root)
    {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

        for (auto &i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }
            // 转变成小写
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr val = LookupBase(key);
            //找到了
            if(val)
            {
                if(i.second.IsScalar())
                {
                    val->fromString(i.second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss<<i.second;
                    val->fromString(ss.str());
                }
            }
        }
    }
    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb)
    {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap& m = GetDatas();
        for(auto it = m.begin();it != m.end();++it)
        {
            cb(it->second);
        }
    }
}