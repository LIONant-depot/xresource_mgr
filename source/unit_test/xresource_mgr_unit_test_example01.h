#include "source/xresource_mgr.h"

//
// Put all the resource types here...
//

// This is just an example of the actual resource structure... normally you could forward declare them.
struct xgpu
{
    struct texture
    {
        int m_X;
    };
};

//
// define a texture resource
//  We will declare the following:
//      * xrsc::type_guid_v - which is how we will uniquely identify the fact that it is a "texture"
//      * xrsc::texture     - which will be the handle to the resource
//      * xresource::loader< xrsc::texture_type_guid_v > - We register our loader this way.
//

// Declare a namespace which it will be used in our tool...
namespace xrsc
{
    // A resource type such a texture in the xresource_pipeline it also represents the GUID of the plugin instance ("Texture" plugin)
    // Since the plugin instance is also a resource... its guid must be generated from an Instance GUID. However, if your engine
    // does not care about plugins and such you do not need to follow this constraint. 
    inline static constexpr auto    texture_type_guid_v = xresource::type_guid(xresource::guid_generator::Instance64FromString("texture"));

    // Now we define the actual handle for our textures... we will reference all our textures using this handle
    using                           texture             = xresource::def_guid<texture_type_guid_v>;
}

// We define our loader here...
template<>
struct xresource::loader< xrsc::texture_type_guid_v >
{
    //--- Expected static parameters ---
    constexpr static inline auto        type_name_v         = L"Texture";                               // This name is used to construct the path to the resource
    using                               data_type           = xgpu::texture;                            // Associate the loader with the actual data that we will use
    constexpr static inline auto        use_death_march_v   = false;                                    // If this is true it will ask the resource manager to wait 1 frame to delete the resource

    // We follow the requirements of the resource manager by providing these two functions
    // Note that we can place the actual loading inside a cpp to minimize dependencies
    static data_type*                   Load        (xresource::mgr& Mgr, const full_guid& GUID);   
    static void                         Destroy     (xresource::mgr& Mgr, data_type&& Data, const full_guid& GUID);
};

// Officially register the loader like this...
inline static xresource::loader_registration<xrsc::texture_type_guid_v> texture_loader;

