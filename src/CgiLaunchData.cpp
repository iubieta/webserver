#include "../inc/CgiLaunchData.hpp"

CgiLaunchData::CgiLaunchData()
    : environmentBuilder_(), execArguments_(),
      environmentError_(CgiEnvironmentBuilder::CGI_ENV_OK),
      argumentsError_(CgiExecArguments::CGI_ARGS_OK) {}
CgiLaunchData::~CgiLaunchData() {}

// Invalidate both pointer arrays and discard any previous request data.
void CgiLaunchData::clear() {
    execArguments_.clear();
    environmentBuilder_.clear();
    environmentError_ = CgiEnvironmentBuilder::CGI_ENV_OK;
    argumentsError_ = CgiExecArguments::CGI_ARGS_OK;
}

// Construct trusted CGI metadata first, then prepare the interpreter arrays.
// On either failure, do not expose an incomplete argv/envp pair.
CgiLaunchData::Error CgiLaunchData::build( const Request& request, const Decision& decision,
    const CgiEnvironmentBuilder::ConnectionInfo& connection) {
    clear();
    environmentError_ = environmentBuilder_.build(request, decision, connection);
    if (environmentError_ != CgiEnvironmentBuilder::CGI_ENV_OK) {
        environmentBuilder_.clear();
        return CGI_LAUNCH_ENV_ERROR;
    }

    argumentsError_ = execArguments_.build(decision.getCgiInterpreter(),
        decision.getDiskPath(), request.getMethod(), request.getQuery(),
        environmentBuilder_.getVariables());
    if (argumentsError_ != CgiExecArguments::CGI_ARGS_OK) {
        execArguments_.clear();
        environmentBuilder_.clear();
        return CGI_LAUNCH_ARGS_ERROR;
    }
    return CGI_LAUNCH_OK;
}

char* const* CgiLaunchData::argv() const { 
    return execArguments_.argv(); 
}

char* const* CgiLaunchData::envp() const { 
    return execArguments_.envp(); 
}

const std::vector<std::string>& CgiLaunchData::arguments() const {
    return execArguments_.arguments();
}

const std::vector<std::string>& CgiLaunchData::environment() const {
    return environmentBuilder_.getVariables();
}

CgiEnvironmentBuilder::Error CgiLaunchData::environmentError() const {
    return environmentError_;
}

CgiExecArguments::Error CgiLaunchData::argumentsError() const {
    return argumentsError_;
}
