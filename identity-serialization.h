#ifndef IDENTITY_SERIALIZATION_H
#define IDENTITY_SERIALIZATION_H

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <cinttypes>
#endif
#include "identity-service.h"
#include "service-serialization.h"

enum IdentityQueryTag {
    QUERY_IDENTITY_NONE = '\0',
    QUERY_IDENTITY_ADDR = 'a',
    QUERY_IDENTITY_EUI = 'i',
    QUERY_IDENTITY_LIST = 'l',
    QUERY_IDENTITY_COUNT = 'c',
    QUERY_IDENTITY_ASSIGN = 'p',
    QUERY_IDENTITY_RM = 'r',
    QUERY_IDENTITY_FORCE_SAVE = 's',
    QUERY_IDENTITY_CLOSE_RESOURCES = 'e'
};

class IdentityEUIRequest : public ServiceMessage {
public:
    DEVEUI eui; // 8 bytes
    IdentityEUIRequest();
    IdentityEUIRequest(char aTag, const DEVEUI &aEUI, int32_t code, uint64_t accessCode);
    IdentityEUIRequest(const unsigned char *buf, size_t sz);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
};

class IdentityAddrRequest : public ServiceMessage {
public:
    DEVADDR addr;   // 4 bytes
    IdentityAddrRequest();
    IdentityAddrRequest(const DEVADDR &addr, int32_t code, uint64_t accessCode);
    IdentityAddrRequest(const unsigned char *buf, size_t sz);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
};

class IdentityEUIAddrRequest : public ServiceMessage {
public:
    NETWORKIDENTITY identity;
    IdentityEUIAddrRequest();
    // explicit IdentityEUIAddrRequest(const DeviceIdentity &identity);
    IdentityEUIAddrRequest(char aTag, const NETWORKIDENTITY &identity, int32_t code, uint64_t accessCode);
    IdentityEUIAddrRequest(const unsigned char *buf, size_t sz);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
};

class IdentityOperationRequest : public ServiceMessage {
public:
    uint32_t offset;
    uint8_t size;
    IdentityOperationRequest();
    IdentityOperationRequest(char tag, size_t aOffset, size_t aSize, int32_t code, uint64_t accessCode);
    IdentityOperationRequest(const unsigned char *buf, size_t sz);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
};

class IdentityGetResponse : public ServiceMessage {
public:
    NETWORKIDENTITY response;

    IdentityGetResponse() = default;
    explicit IdentityGetResponse(const IdentityAddrRequest& request);
    explicit IdentityGetResponse(const IdentityEUIRequest &request);
    IdentityGetResponse(const unsigned char *buf, size_t sz);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
};

class IdentityOperationResponse : public IdentityOperationRequest {
public:
    uint32_t response;
    IdentityOperationResponse();
    IdentityOperationResponse(const IdentityOperationResponse& resp);
    IdentityOperationResponse(const unsigned char *buf, size_t sz);
    explicit IdentityOperationResponse(const IdentityEUIAddrRequest &request);
    explicit IdentityOperationResponse(const IdentityOperationRequest &request);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
};

class IdentityListResponse : public IdentityOperationResponse {
public:
    std::vector<NETWORKIDENTITY> identities;
    IdentityListResponse();
    IdentityListResponse(const IdentityListResponse& resp);
    IdentityListResponse(const unsigned char *buf, size_t sz);
    explicit IdentityListResponse(const IdentityOperationRequest &request);
    void ntoh() override;
    size_t serialize(unsigned char *retBuf) const override;
    std::string toJsonString() const override;
    size_t shortenList2Fit(size_t serializedSize);
};

class IdentitySerialization {
private:
    IdentityService *svc;
    int32_t code;
    uint64_t accessCode;
public:
    explicit IdentitySerialization(
        IdentityService *svc,
        int32_t code,
        uint64_t accessCode
    );
    /**
     * Request IdentityService and return serializred response.
     * @param retBuf buffer to return serialized response
     * @param retSize buffer size
     * @param request serialized request
     * @param sz serialized request size
     * @return IdentityService response size
     */
    size_t query(
        unsigned char *retBuf,
        size_t retSize,
        const unsigned char *request,
        size_t sz
    );
};

/**
 * Return request object or  NULL if packet is invalid
 * @param buf buffer
 * @param sz buffer size
 * @return return NULL if packet is invalid
 */
ServiceMessage* deserializeIdentity(
    const unsigned char *buf,
    size_t sz
);

/**
 * Check does it identity tag in the buffer
 * @param buffer buffer to check
 * @param size buffer size
 * @return true
 */
bool isIdentityTag(
    const unsigned char *buffer,
    size_t size
);

/**
 * Check does it serialized query in the buffer
 * @param buffer buffer to check
 * @param size buffer size
 * @return query tag
 */
enum IdentityQueryTag validateIdentityQuery(
    const unsigned char *buffer,
    size_t size
);

/**
 * Return required size for response
 * @param buffer serialized request
 * @param size buffer size
 * @return size in bytes
 */
size_t responseSizeForIdentityRequest(
    const unsigned char *buffer,
    size_t size
);

const char* identityTag2string(
    enum IdentityQueryTag value
);

const std::string &identityCommandSet();

#endif