// MarkUpdated: a resource compiled again while references hold it. Each reference moves to the new data on its next getResource;
// the old data goes with its last reference; a resource that holds a reference to it (a material instance and its material) gets
// the new one through that reference; an unloaded resource is not touched.
#include "source/xresource_mgr.h"
#include <cstdio>

struct update_test_shader   { int m_Version; };
struct update_test_instance { int m_X; };

namespace update_test
{
    inline int g_Version   = 0;     // the version the next load gets
    inline int g_Destroyed = 0;

    inline static constexpr auto shader_type_guid_v   = xresource::type_guid(xresource::guid_generator::Instance64FromString("update_test_shader"));
    inline static constexpr auto instance_type_guid_v = xresource::type_guid(xresource::guid_generator::Instance64FromString("update_test_instance"));
    using shader_ref   = xresource::def_guid<shader_type_guid_v>;
    using instance_ref = xresource::def_guid<instance_type_guid_v>;
}

template<>
struct xresource::loader< update_test::shader_type_guid_v >
{
    constexpr static inline auto        type_name_v         = L"UpdateTestShader";
    using                               data_type           = update_test_shader;
    constexpr static inline auto        use_death_march_v   = false;
    static data_type* Load(xresource::mgr&, const full_guid&) { return new data_type{ ++update_test::g_Version }; }
    static void       Destroy(xresource::mgr&, data_type&& Data, const full_guid&) { ++update_test::g_Destroyed; delete &Data; }
};
inline static xresource::loader_registration<update_test::shader_type_guid_v> update_test_shader_loader;

template<>
struct xresource::loader< update_test::instance_type_guid_v >
{
    constexpr static inline auto        type_name_v         = L"UpdateTestInstance";
    using                               data_type           = update_test_instance;
    constexpr static inline auto        use_death_march_v   = false;
    static data_type* Load(xresource::mgr&, const full_guid&) { return new data_type{ 7 }; }
    static void       Destroy(xresource::mgr&, data_type&& Data, const full_guid&) { delete &Data; }
};
inline static xresource::loader_registration<update_test::instance_type_guid_v> update_test_instance_loader;

#define UCHECK(X, ...) do { if (!(X)) { std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); return 1; } } while (0)

int RunUpdateTests()
{
    using namespace update_test;
    xresource::mgr Mgr;
    Mgr.Initiallize();

    shader_ref A, B;
    A.m_Instance = A.m_Instance.GenerateGUID();
    B = A;
    const xresource::full_guid Guid = A;

    // Two references on version 1
    const auto* p1 = Mgr.getResource(A);
    UCHECK(p1 && p1->m_Version == 1 && Mgr.getResource(B) == p1, "the first loads");
    UCHECK(Mgr.getResourceCount() == 1, "one resource loaded");

    // Unloaded: nothing happens
    {
        xresource::full_guid Other = Guid; Other.m_Instance = Other.m_Instance.GenerateGUID();
        Mgr.MarkUpdated(Other);
        UCHECK(Mgr.getResourceCount() == 1 && !Mgr.isStale(A), "marking an unloaded resource changed something");
    }

    // Updated: A moves on its next get, B stays on the old one until its own
    Mgr.MarkUpdated(Guid);
    UCHECK(Mgr.isStale(A) && Mgr.isStale(B), "both references are on the old data");
    const auto* p2 = Mgr.getResource(A);
    UCHECK(p2 && p2 != p1 && p2->m_Version == 2, "the reference did not move to the new data (version %d)", p2 ? p2->m_Version : -1);
    UCHECK(!Mgr.isStale(A) && Mgr.isStale(B) && g_Destroyed == 0, "the old data went while a reference still held it");
    UCHECK(Mgr.getResourceCount() == 2, "old and new both alive (%d)", Mgr.getResourceCount());
    UCHECK(Mgr.getFullGuid(B) == Guid && Mgr.getFullGuid(A) == Guid, "the references lost their GUID");

    // A new reference by GUID gets the new data, no third load
    shader_ref C; C.m_Instance = Guid.m_Instance;
    UCHECK(Mgr.getResource(C) == p2 && g_Version == 2, "a get by GUID loaded again or got the old data");

    // B moves: the old data goes with its last reference
    UCHECK(Mgr.getResource(B) == p2 && g_Destroyed == 1 && !Mgr.isStale(B), "the old data stayed after its last reference moved");
    UCHECK(Mgr.getResourceCount() == 1, "one resource loaded after the move (%d)", Mgr.getResourceCount());

    // A resource holding a reference (a material instance and its material): through that reference the new one
    instance_ref I; I.m_Instance = I.m_Instance.GenerateGUID();
    auto* pI = Mgr.getResource(I);
    struct holder { shader_ref m_Shader; } H;          // as the instance's data would hold it
    H.m_Shader.m_Instance = Guid.m_Instance;
    UCHECK(pI && Mgr.getResource(H.m_Shader) == p2, "the held reference resolves");
    Mgr.MarkUpdated(Guid);
    const auto* p3 = Mgr.getResource(H.m_Shader);
    UCHECK(p3 && p3 != p2 && p3->m_Version == 3 && Mgr.getResource(I) == pI, "the held reference did not move, or the holder was reloaded");

    // Marked twice before anyone asks: one more load, not two
    Mgr.MarkUpdated(Guid);
    Mgr.MarkUpdated(Guid);
    const int Before = g_Version;
    UCHECK(Mgr.getResource(A) != nullptr && g_Version == Before + 1, "marking twice loaded twice (%d -> %d)", Before, g_Version);

    // Everything released: nothing left
    Mgr.ReleaseRef(A); Mgr.ReleaseRef(B); Mgr.ReleaseRef(C); Mgr.ReleaseRef(H.m_Shader); Mgr.ReleaseRef(I);
    UCHECK(Mgr.getResourceCount() == 0, "%d resources left after releasing every reference", Mgr.getResourceCount());
    UCHECK(g_Destroyed == g_Version, "loaded %d, destroyed %d", g_Version, g_Destroyed);

    std::printf("update tests: PASS\n");
    return 0;
}
