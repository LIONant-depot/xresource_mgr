
#include "xresource_mgr_unit_test_example01.h"

//--------------------------------------------------------------------------

xgpu::texture* xresource::loader< xrsc::texture_type_guid_v >::Load(xresource::mgr& Mgr, const full_guid& GUID)
{
    auto FullPathToResource = Mgr.getResourcePath(GUID, type_name_v);

    //
    // Now we should be able to call our loader
    // Example: LoadTexture(FullPathToResource);
    //

    //
    // After the data is loaded we usually need to register with a system... such opengl.
    // We can do that now...
    //

    //
    // Typically once we register the texture with opengl... we just need an int
    // But sometimes it is good to keep extra-data such Width and Height, etc...
    // Add that data would be part of the xgpu::texture (in this example does not have that)
    // But now we should be ready to return the actual texture structure


    // I will create this as a pretend load/creation example...
    auto OurData = std::make_unique<xgpu::texture>(22);

    // Give the data to the resource manager
    return OurData.release();
}

//--------------------------------------------------------------------------
// This function should release the memory
void xresource::loader< xrsc::texture_type_guid_v >::Destroy(xresource::mgr& Mgr, data_type&& Data, const full_guid& GUID)
{
    // OK The resource manager thinks it does not need the data any more...
    // So we can release it here...
    // In openGL we would need to unregister our data and such before actually deleting our structure.
    delete &Data;
}
