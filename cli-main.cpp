#include <string>
#include <iostream>
#include <csignal>
#include <climits>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <direct.h>
#include <sstream>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

#include "argtable3/argtable3.h"

#ifdef ENABLE_LIBUV
#include "lorawan/storage/listener/uv-listener.h"
#define DAEMONIZE_CLOSE_FILE_DESCRIPTORS_AFTER_FORK false
#else
#include "lorawan/storage/listener/udp-listener.h"
#define DAEMONIZE_CLOSE_FILE_DESCRIPTORS_AFTER_FORK true
#endif

#ifdef ENABLE_HTTP
#include "lorawan/storage/listener/http-listener.h"
#include "lorawan/storage/serialization/identity-text-json-serialization.h"
#endif

#define DEF_DB_GATEWAY_JSON  "gateway.json"

#ifdef ENABLE_SQLITE
#include "lorawan/storage/service/identity-service-sqlite.h"
#include "lorawan/storage/service/gateway-service-sqlite.h"
#define DEF_DB  "lorawan.db"
#endif

#ifdef ENABLE_GEN
#include "lorawan/storage/service/identity-service-gen.h"
#define DEF_DB  "gen"
#endif

#ifdef ENABLE_JSON
#include "lorawan/storage/service/identity-service-json.h"
#include "lorawan/storage/service/gateway-service-json.h"
#define DEF_DB  "identity.json"
#else
#include "lorawan/storage/service/identity-service-mem.h"
#include "lorawan/storage/service/gateway-service-mem.h"
#endif

#ifndef DEF_DB
#define DEF_DB  "none"
#endif

#include "lorawan/lorawan-error.h"
#include "lorawan/lorawan-msg.h"
#include "log.h"
#include "daemonize.h"
#include "lorawan/helper/ip-address.h"
#include "lorawan/storage/serialization/identity-binary-serialization.h"

// i18n
// #include <libintl.h>
// #define _(String) gettext (String)
#define _(String) (String)

const char *programName = "lorawan-storage";
#define DEF_PASSPHRASE  "masterkey"

enum IP_PROTO {
    PROTO_UDP,
    PROTO_TCP,
    PROTO_UDP_N_TCP
};

static std::string IP_PROTO2string(
    enum IP_PROTO value
)
{
    switch(value) {
        case PROTO_UDP:
            return "UDP";
        case PROTO_TCP:
            return "TCP";
        default:
            return "TCP, UDP";
    }
}

// global parameters and descriptors
class CliServiceDescriptorNParams : public Log
{
public:
    StorageListener *server;
    enum IP_PROTO proto;
    std::string intf;
    uint16_t port;
#ifdef ENABLE_HTTP
    StorageListener *httpServer;
    std::string httpIntf;
    uint16_t httpPort;
#endif
    int32_t code;
    uint64_t accessCode;
    bool runAsDaemon;
    std::string pidfile;
    int verbose;
    std::string db;
    std::string dbGatewayJson;
    int32_t retCode;
#ifdef ENABLE_GEN
    std::string passPhrase;
    NETID netid;
#endif
    CliServiceDescriptorNParams()
        : server(nullptr), proto(PROTO_UDP), port(4244),
#ifdef ENABLE_HTTP
        httpServer(nullptr), httpPort(4246),
#endif
        code(0), accessCode(0), verbose(0), retCode(0),
        runAsDaemon(false)
#ifdef ENABLE_GEN
        , netid(0, 0)
#endif
    {

    }

    std::string toString() const {
        std::stringstream ss;
        ss
            << _("Service: ") << intf << ":" << port << " " << IP_PROTO2string(proto) << ".\n"
            << _("Code: ") << std::hex << code << _(", access code: ")  << accessCode << " " << "\n";
        if (!db.empty())
            ss << _("database file name: ") << db << "\n";
#ifdef ENABLE_JSON
        if (!dbGatewayJson.empty())
            ss << _("gateway database file name: ") << dbGatewayJson << "\n";
#endif
        return ss.str();
    }

    std::ostream& strm(
        int level
    ) override {
        return std::cerr;
    }

    void flush() override {
        std::cerr << std::endl;
    }
};

CliServiceDescriptorNParams svc;

static void done() {
    if (svc.server) {
        svc.server->stop();
        svc.server->identitySerialization->svc->flush();
        delete svc.server;
        svc.server = nullptr;
        std::cerr << MSG_GRACEFULLY_STOPPED << std::endl;
        exit(svc.retCode);
    }
}

static void stop() {
	if (svc.server) {
        svc.server->stop();
	}
}

void signalHandler(int signal)
{
	if (signal == SIGINT) {
		std::cerr << MSG_INTERRUPTED << std::endl;
        done();
	}
}

void setSignalHandler()
{
#if defined(_MSC_VER) || defined(__MINGW32__)
#else
	struct sigaction action = {};
	action.sa_handler = &signalHandler;
	sigaction(SIGINT, &action, nullptr);
	sigaction(SIGHUP, &action, nullptr);
#endif
}

void run() {
    auto identityService =
#ifdef ENABLE_SQLITE
        new SqliteIdentityService;
    identityService->init(svc.db, nullptr);
#endif
#ifdef ENABLE_GEN
        new GenIdentityService;
    identityService->init(svc.passPhrase, &svc.netid);
#endif
#ifdef ENABLE_JSON
        new JsonIdentityService;
    identityService->init(svc.db, nullptr);
#else
        new ClientUDPIdentityService;
    identityService->init("", nullptr);
#endif

    auto gatewayService =
#ifdef ENABLE_SQLITE
        new SqliteGatewayService;
    gatewayService->init(svc.db, nullptr);
#else
#ifdef ENABLE_JSON
        new JsonGatewayService;
    gatewayService->init(svc.dbGatewayJson, nullptr);
#else
        new MemoryGatewayService;
    gatewayService->init("", nullptr);
#endif
#endif

    auto identitySerialization = new IdentityBinarySerialization(identityService, svc.code, svc.accessCode);
    auto gatewaySerialization = new GatewaySerialization(gatewayService, svc.code, svc.accessCode);
#ifdef ENABLE_LIBUV
    svc.server = new UVListener(identitySerialization, gatewaySerialization);
#else
    svc.server = new UDPListener(identitySerialization, gatewaySerialization);
#endif
    svc.server->setAddress(svc.intf, svc.port);
    svc.server->setLog(svc.verbose, &svc);

#ifdef ENABLE_HTTP
    auto identitySerializationJSON = new IdentityTextJSONSerialization(identityService, svc.code, svc.accessCode);
    svc.httpServer = new HTTPListener(identitySerializationJSON, gatewaySerialization);
    svc.httpServer->setAddress(svc.httpIntf, svc.httpPort);
    svc.httpServer->setLog(svc.verbose, &svc);
#endif

    if (svc.verbose)
        std::cout << _("Identities: ") << svc.server->identitySerialization->svc->size() << std::endl;

    svc.retCode = svc.server->run();
    if (svc.retCode)
        std::cerr << ERR_MESSAGE << svc.retCode << ": " << std::endl;
}

int main(int argc, char **argv) {
	struct arg_str *a_interface_n_port = arg_str0(nullptr, nullptr, _("IP addr:port"), _("Default *:4244"));

#ifdef ENABLE_HTTP
    struct arg_str *a_http_interface_n_port = arg_str0("h", "http", _("IP addr:port"), _("Default *:4246"));
#endif

    struct arg_str *a_db = arg_str0("f", "db", _("<database file>"), _("database file name. Default " DEF_DB));
#ifdef ENABLE_JSON
    struct arg_str *a_gateway_json_db = arg_str0("g", "gateway-db", _("<database file>"), _("database file name. Default " DEF_DB_GATEWAY_JSON));
#endif
    struct arg_int *a_code = arg_int0("c", "code", _("<number>"), _("Default 42. 0x - hex number prefix"));
#ifdef ENABLE_GEN
    struct arg_str *a_pass_phrase = arg_str0("m", _("master-key"), _("<pass-phrase>"), _("Default " DEF_PASSPHRASE));
    struct arg_str *a_net_id = arg_str0("n", "network-id", _("<hex|hex:hex>"), _("Hexadecimal <network-id> or <net-type>:<net-id>. Default 0"));
#endif

    struct arg_str *a_access_code = arg_str0("a", "access", _("<hex>"), _("Default 2a (42 decimal)"));
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", _("run daemon"));
    struct arg_str *a_pidfile = arg_str0("p", "pidfile", _("<file>"), _("Check whether a process has created the file pidfile"));
    struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 2, _("-v - verbose, -vv - debug"));
	struct arg_lit *a_help = arg_lit0("h", "help", _("Show this help"));
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
		a_interface_n_port,
#ifdef ENABLE_HTTP
        a_http_interface_n_port,
#endif
#ifdef ENABLE_GEN
        a_pass_phrase, a_net_id,
#endif
#if defined ENABLE_SQLITE || defined ENABLE_JSON
        a_db,
#endif
#ifdef ENABLE_JSON
        a_gateway_json_db,
#endif
        a_code, a_access_code, a_verbose, a_daemonize, a_pidfile,
		a_help, a_end 
	};

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0) {
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return ERR_CODE_COMMAND_LINE;
	}
	// Parse the command line as defined by argtable[]
	int nerrors = arg_parse(argc, argv, argtable);

    svc.runAsDaemon = a_daemonize->count > 0;
    if (a_pidfile->count)
        svc.pidfile = *a_pidfile->sval;
    else
        svc.pidfile = "";

    svc.verbose = a_verbose->count;

	if (a_interface_n_port->count) {
        splitAddress(svc.intf, svc.port, std::string(*a_interface_n_port->sval));
    } else {
        svc.intf = "*";
        svc.port = 4244;
    }

#ifdef ENABLE_HTTP
    if (a_http_interface_n_port->count) {
        splitAddress(svc.httpIntf, svc.httpPort, std::string(*a_http_interface_n_port->sval));
    } else {
        svc.httpIntf = "*";
        svc.httpPort = 4246;
    }
#endif

#ifdef ENABLE_GEN
    if (a_pass_phrase->count)
        svc.passPhrase = *a_pass_phrase->sval;
    else
        svc.passPhrase = DEF_PASSPHRASE;
    if (a_net_id)
        readNetId(svc.netid, *a_net_id->sval);
#endif
    if (a_db->count)
        svc.db = *a_db->sval;
    else
        svc.db = DEF_DB;
#ifdef ENABLE_JSON
    if (a_gateway_json_db->count)
        svc.dbGatewayJson = *a_gateway_json_db->sval;
    else
        svc.dbGatewayJson = DEF_DB_GATEWAY_JSON;
#endif

    if (a_code->count)
        svc.code = *a_code->ival;
    else
        svc.code = 42;
    if (a_access_code->count)
        svc.accessCode = strtoull(*a_access_code->sval, nullptr, 16);
    else
        svc.accessCode = 42;

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors) {
		if (nerrors)
			arg_print_errors(stderr, a_end, programName);
		std::cerr << _("Usage: ") << programName << std::endl;
		arg_print_syntax(stderr, argtable, "\n");
		std::cerr << _("LoRaWAN gateway storage service") << std::endl;
		arg_print_glossary(stderr, argtable, "  %-27s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return ERR_CODE_COMMAND_LINE;
	}
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

#if defined(_MSC_VER) || defined(__MINGW32__)
    WSADATA wsaData;
    int r = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (r) {
        std::cerr << ERR_SOCKET_CREATE << std::endl;
        exit(r);
    }
#endif

    if (svc.runAsDaemon) {
		char workDir[PATH_MAX];
		std::string programPath = getcwd(workDir, PATH_MAX);
		if (svc.verbose)
			std::cerr << MSG_LISTENER_DAEMON_RUN
              << "(" << programPath << "/" << programName << "). "
              << MSG_CHECK_SYSLOG << std::endl;
		OPEN_SYSLOG(programName)
        Daemonize daemon(programName, programPath, run, stop, done, 0, svc.pidfile, DAEMONIZE_CLOSE_FILE_DESCRIPTORS_AFTER_FORK);
		// CLOSESYSLOG()
	} else {
		setSignalHandler();
        if (svc.verbose > 1)
            std::cerr << svc.toString();
		run();
		done();
	}
}
