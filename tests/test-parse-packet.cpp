#include <cassert>
#include "lorawan/lorawan-string.h"

void testIdentityProp()
{
    NETWORK_IDENTITY_PROPERTY p = string2NETWORK_IDENTITY_PROPERTY("version");
    assert(p == NIP_LORAWAN_VERSION);
    p = string2NETWORK_IDENTITY_PROPERTY("ver");
    assert(p == NIP_NONE);
    p = string2NETWORK_IDENTITY_PROPERTY("name");
    assert(p == NIP_DEVICENAME);
}

int main() {
    testIdentityProp();
    return 0;
}
