#include <ul/man/ui/ui_MainApplication.hpp>
#include <ul/sf/sf_PublicService.hpp>

ul::man::ui::MainApplication::Ref g_MainApplication;

namespace {

    bool g_IsAvailable;
    ul::Version g_GotVersion;

    void Initialize() {
        UL_RC_ASSERT(nsInitialize());
        
        g_IsAvailable = ul::sf::IsAvailable();
        if(g_IsAvailable) {
            UL_RC_ASSERT(ul::sf::Initialize());
            UL_RC_ASSERT(ul::sf::GetVersion(&g_GotVersion));
        }
    }

    void Finalize() {
        ul::sf::Finalize();
        nsExit();
    }

}

int main() {
    ul::InitializeLogging("uManager");
    Initialize();

    auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
    renderer_opts.UseImage(pu::ui::render::IMGAllFlags);
    renderer_opts.AddDefaultAllSharedFonts();
    renderer_opts.AddExtraDefaultFontSize(35);
    renderer_opts.UseRomfs();
    auto renderer = pu::ui::render::Renderer::New(renderer_opts);

    g_MainApplication = ul::man::ui::MainApplication::New(renderer);
    g_MainApplication->Set(g_IsAvailable, g_GotVersion.Equals(ul::CurrentVersion), g_GotVersion);
    g_MainApplication->Prepare();
    g_MainApplication->ShowWithFadeIn();

    Finalize();
    return 0;
}