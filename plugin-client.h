#ifndef PLUGIN_CLIENT_H_
#define PLUGIN_CLIENT_H_	1

#include <string>

#include "direct-client.h"

#ifdef _MSC_VER
#include <Windows.h>
#else
typedef void * HINSTANCE;
#endif


class PluginClient : public DirectClient {
private:
    HINSTANCE handleSvc;
    int load(
        const std::string &fileName,
        const std::string &classIdentityName,
        const std::string &classGatewayName
    );
    void unload();
public:
    explicit PluginClient(
        const std::string &fileName,
        const std::string &classIdentityName,
        const std::string &classGatewayName
    );
    virtual ~PluginClient();
};

#endif
