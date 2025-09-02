
#include "xresource_mgr_unit_test_example01.h"



int main()
{
    std::array<xrsc::texture, 10>   ListOfComponentsBackup;
    std::array<xrsc::texture,10>    ListOfComponents;
    xresource::mgr Mgr;

    // Initialize the resource manager
    Mgr.Initiallize();

    // Generate a bunch of GUIDs
    // Normally these would come from resources
    // We're also going back them up to later test them...
    for ( auto i=0u; i<ListOfComponents.size(); ++i )
    {
        auto& E = ListOfComponents[i];
        ListOfComponentsBackup[i].m_Instance = E.m_Instance.GenerateGUID();
    }

    // We are going to fake call this function pretending a frame has pass 
    Mgr.OnEndFrameDelegate();

    //
    // Get the resources 1 time
    // 
    for ( auto& E : ListOfComponents )
    {
        if ( auto pTexture = Mgr.getResource(E); pTexture == nullptr )
        {
            // It fail to get the resource...
            assert(false);
        }
        else
        {
            // Make sure that the resource is valid...
            // 22 is the stupid number we set in the loader to make sure everything is ok
            assert(pTexture->m_X == 22);
        }
    }

    // We are going to fake call this function pretending a frame has pass 
    Mgr.OnEndFrameDelegate();

    //
    // Get the resources 2nd time
    // 
    for (auto& E : ListOfComponents)
    {
        if (auto pTexture = Mgr.getResource(E); pTexture == nullptr)
        {
            // It fail to get the resource...
            assert(false);
        }
        else
        {
            // Make sure that the resource is valid...
            // 22 is the stupid number we set in the loader to make sure everything is ok
            assert(pTexture->m_X == 22);
        }
    }

    // We are going to fake call this function pretending a frame has pass 
    Mgr.OnEndFrameDelegate();

    //
    // Check if we can get their GUIDs
    //
    for (auto i = 0u; i < ListOfComponents.size(); ++i)
    {
        assert(ListOfComponents[i] != ListOfComponentsBackup[i]);
        assert(Mgr.getFullGuid(ListOfComponents[i]) == ListOfComponentsBackup[i]);
    }

    // We are going to fake call this function pretending a frame has pass 
    Mgr.OnEndFrameDelegate();

    //
    // Let us release the resources
    // 
    for (auto i = 0u; i < ListOfComponents.size(); ++i)
    {
        Mgr.ReleaseRef(ListOfComponents[i]);
        assert(ListOfComponents[i] == ListOfComponentsBackup[i]);
    }

    // We are going to fake call this function pretending a frame has pass 
    Mgr.OnEndFrameDelegate();

    return 0;
}