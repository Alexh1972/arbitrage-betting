#pragma once

#include "service/PlatformService.h"

namespace arb {

class PlatformInit {
public:
    explicit PlatformInit(PlatformService& platformService);

    void run();

private:
    PlatformService& platformService_;
};

}
