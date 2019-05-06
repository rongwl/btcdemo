#if defined(HAVE_CONFIG_H)
#include "config/btclite-config.h"
#endif

#include "error.h"
#include "serialize.h"
#include "sync.h"
#include "util.h"
#include "utiltime.h"

#include <getopt.h>
#include <locale>
#include <stdarg.h>


volatile std::sig_atomic_t SigMonitor::received_signal_ = 0;


static void HandleAllocFail()
{
    // Rather than throwing std::bad-alloc if allocation fails, terminate
    // immediately to (try to) avoid chain corruption.
    std::set_new_handler(std::terminate);
    BTCLOG(LOG_LEVEL_ERROR) << "Critical error: out of memory. Terminating.";
    
    // The log was successful, terminate now.
    std::terminate();
}

void Args::PrintUsage() const
{
    fprintf(stdout, "Common Options:\n");
    fprintf(stdout, "  -h or -?,  --help     print this help message and exit\n");
    fprintf(stdout, "  --debug=<module>      output debugging information(default: 0)\n");
    fprintf(stdout, "                        <module> can be 1(output all debugging information),\n");
    fprintf(stdout, "                        mempool, net\n");
    fprintf(stdout, "  --loglevel=<level>    specify the type of message being printed(default: %s)\n", DEFAULT_LOG_LEVEL);
    fprintf(stdout, "                        <level> can be:\n");
    fprintf(stdout, "                            0(A fatal condition),\n");
    fprintf(stdout, "                            1(An error has occurred),\n");
    fprintf(stdout, "                            2(A warning),\n");
    fprintf(stdout, "                            3(Normal message),\n");
    fprintf(stdout, "                            4(Debug information),\n");
    fprintf(stdout, "                            5(Verbose information\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Chain selection options:\n");
    fprintf(stdout, "  --testnet             Use the test chain\n");
    fprintf(stdout, "  --regtest             Enter regression test mode, which uses a special chain\n");
    fprintf(stdout, "                        in which blocks can be solved instantly. This is intended\n");
    fprintf(stdout, "                        for regression testing tools and app development.\n");
    
    //              "                                                                                "

}

void Args::CheckArguments() const
{
    // --testnet and --regtest
    if (IsArgSet(GLOBAL_OPTION_TESTNET) && IsArgSet(GLOBAL_OPTION_REGTEST)) 
        throw Exception(ErrorCode::invalid_option, "invalid combination of --testnet and --regtest");
    
    // --debug
    if (IsArgSet(GLOBAL_OPTION_DEBUG)) {
        const std::vector<std::string> arg_values = GetArgs(GLOBAL_OPTION_DEBUG);
        if (std::none_of(arg_values.begin(), arg_values.end(),
        [](const std::string& val) { return val == "0"; })) {
            auto result = std::find_if(arg_values.begin(), arg_values.end(), 
            [](const std::string& module) { return Logging::MapIntoModule(module) == Logging::NONE; });
            if (result != arg_values.end())
                throw Exception(ErrorCode::invalid_argument, "invalid module '" + *result + "'");
        }
    }
    
    // --loglevel
    if (IsArgSet(GLOBAL_OPTION_LOGLEVEL)) {
        const std::string arg_val = GetArg(GLOBAL_OPTION_LOGLEVEL, DEFAULT_LOG_LEVEL);
        int level = -1;
        try {
            level = std::stoi(arg_val);
        }
        catch (const std::exception& e) {
            throw Exception(ErrorCode::invalid_argument, "invalid loglevel '" + arg_val + "'");
        }
        if (level < 0 || level >= LOG_LEVEL_MAX) {
            throw Exception(ErrorCode::invalid_argument, "invalid loglevel '" + arg_val + "'");
        }
    }
}

bool Args::InitParameters()
{   
    // --debug
    if (IsArgSet(GLOBAL_OPTION_DEBUG)) {
        const std::vector<std::string> arg_values = GetArgs(GLOBAL_OPTION_DEBUG);
        if (std::none_of(arg_values.begin(), arg_values.end(),
        [](const std::string& val) { return val == "0"; })) {
            for (auto module : arg_values) {
                Logging::set_logModule(Logging::MapIntoModule(module));
            }
        }
    }
    
    // --loglevel
    if (IsArgSet(GLOBAL_OPTION_LOGLEVEL)) {
        const std::string arg_val = GetArg(GLOBAL_OPTION_LOGLEVEL, DEFAULT_LOG_LEVEL);
        int level = std::stoi(arg_val);

        if (level <= LOG_LEVEL_WARNING) {
            BTCLOG(LOG_LEVEL_INFO) << "set log level: " << level;
            FLAGS_minloglevel = Logging::MapIntoGloglevel(level);
        }
        else {
            BTCLOG(LOG_LEVEL_INFO) << "set log level: " << level;
            FLAGS_minloglevel = 0;
            FLAGS_v = Logging::MapIntoGloglevel(level);
        }
    }
    else
        BTCLOG(LOG_LEVEL_INFO) << "default log level: " << DEFAULT_LOG_LEVEL;
    
    return true;
}

/* Check options that getopt_long() can not print totally */
void Args::CheckOptions(int argc, const char* const argv[])
{
    for (int i = 1; i < argc; i++) {
        std::string str(argv[i]);
        if ((str.length() > 2 && str.compare(0, 2, "--")) ||
            !str.compare("--")) {
            throw Exception(ErrorCode::invalid_option, "invalid option '" + str + "'");
        }
    }
}

std::string Args::GetArg(const std::string& arg, const std::string& arg_default) const
{
    LOCK(cs_args_);
    auto it = map_args_.find(arg);
    if (it != map_args_.end())
        return it->second;
    return arg_default;
}

bool Args::GetBoolArg(const std::string& arg, bool arg_default) const
{
    LOCK(cs_args_);
    auto it = map_args_.find(arg);
    if (it != map_args_.end()) {
        if (it->second.empty() || it->second == "0")
            return false;
        else
            return true;
    }
    
    return arg_default;
}

std::vector<std::string> Args::GetArgs(const std::string& arg) const
{
    LOCK(cs_args_);
    auto it = map_multi_args_.find(arg);
    if (it != map_multi_args_.end())
        return it->second;
    return {};
}

void Args::SetArg(const std::string& arg, const std::string& arg_val)
{
    LOCK(cs_args_);
    map_args_[arg] = arg_val;
    map_multi_args_[arg] = {arg_val};
}

void Args::SetArgs(const std::string& arg, const std::string& arg_val)
{
    LOCK(cs_args_);
    map_args_[arg] = arg_val;
    map_multi_args_[arg].push_back(arg_val);
}

bool Args::IsArgSet(const std::string& arg) const
{
    LOCK(cs_args_);
    return map_args_.count(arg);
}

bool Args::ParseFromFile(const std::string& path) const
{
    std::ifstream ifs(path);
    if (!ifs.good()) {
        return true; // No config file is OK
    }
     
    std::string line;
    while (std::getline(ifs, line)) {
     line.erase(std::remove_if(line.begin(), line.end(), 
            [](unsigned char x){return std::isspace(x);}),
         line.end());
     if (line[0] == '#' || line.empty())
      continue;
     auto pos = line.find("=");
     if (pos != std::string::npos) {
      std::string str = line.substr(0, pos);
      if (!IsArgSet(str) && str != GLOBAL_OPTION_CONF) {
       // Don't overwrite existing settings so command line settings override config file
       std::string str_val = line.substr(pos+1);
      }
     }
    }
    
    return true;
}

bool DataFiles::LockDataDir()
{
    return true;
}

void DataFiles::set_data_dir(const fs::path& path)
{
    LOCK(cs_path_);
    if (!fs::is_directory(path)) {
        BTCLOG(LOG_LEVEL_WARNING) << "Set data path \"" << path.c_str()
                                 << "\" does not exist.";
        return;
    }
    data_dir_ = path;
}

void DataFiles::set_config_file(const std::string& filename)
{
    LOCK(cs_path_);
    std::ifstream ifs(data_dir_ / filename);
    if (!ifs.good()) {
        BTCLOG(LOG_LEVEL_WARNING) << "Specified config file \"" << data_dir_.c_str() << "/" << filename
                                 << "\" does not exist.";
        return;
    }
    config_file_ = (data_dir_ / filename);
}

bool BaseExecutor::BasicSetup()
{    
    // Ignore SIGPIPE, otherwise it will bring the daemon down if the client closes unexpectedly
    std::signal(SIGPIPE, SIG_IGN);
    
    std::set_new_handler(HandleAllocFail);
    
    return true;
}

void BaseExecutor::WaitForSignal()
{
    while (!sig_int_.IsReceived() && !sig_term_.IsReceived()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void SetupEnvironment()
{
    try {
        std::locale(""); // Raises a runtime error if current locale is invalid
    } catch (const std::runtime_error&) {
        setenv("LC_ALL", "C", 1);
    }
}

