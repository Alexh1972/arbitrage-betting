#include "config/PlatformInit.h"

namespace arb {

PlatformInit::PlatformInit(PlatformService& platformService) : platformService_(platformService) {}

void PlatformInit::run() {
    platformService_.save(Platform("Polymarket"));
    platformService_.save(Platform("Kalshi"));
}

}
