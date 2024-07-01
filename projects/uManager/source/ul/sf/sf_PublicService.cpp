#include <ul/sf/sf_PublicService.hpp>
#include <ul/sf/sf_Base.hpp>

namespace ul::sf {

    namespace {

        Service g_PublicService;

    }

    bool IsAvailable() {
        auto has = false;
        const auto name = smEncodeName(sf::PublicServiceName);
        tipcDispatchInOut(smGetServiceSessionTipc(), 65100, name, has);
        return has;
    }

    Result Initialize() {
        if(serviceIsActive(&g_PublicService)) {
            return ResultSuccess;
        }

        UL_RC_TRY(smGetService(&g_PublicService, sf::PublicServiceName));

        return ResultSuccess;
    }

    void Finalize() {
        serviceClose(&g_PublicService);
    }

    Result GetVersion(Version *out_ver) {
        return serviceDispatchOut(&g_PublicService, 0, *out_ver);
    }

}
