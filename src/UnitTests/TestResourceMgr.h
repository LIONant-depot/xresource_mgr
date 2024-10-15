//------------------------------------------------------------------------------------
// Resource Type Registration
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

template<>
struct xresource::type<int>
{
    static inline bool s_bFailed;

    // Converts from a string to a GUID (Does not need to be a string... but you should never change it!)
    static constexpr auto guid_v = xresource::type_guid("int");

    // This is a magic value used for debugging 
    static constexpr int magic_v = 512;

    static int* Load( xresource::mgr& Mgr, xresource::guid GUID ) noexcept
    {
        auto Texture = std::make_unique<int>(magic_v);
        return Texture.release();
    }

    static void Destroy( int& Data, xresource::mgr& Mgr, xresource::guid GUID ) noexcept
    {
        if( Data != magic_v)
        {
            printf( "ERROR: Failed to get the original value...\n");
            s_bFailed = true;
        }
        else
        {
            s_bFailed = false;
            delete &Data;
        }
    }
};

//------------------------------------------------------------------------------------

template<>
struct xresource::type<float>
{
    static inline bool s_bFailed;

    // This is a nice name that we can associate with this resource... useful for editors and such
    static constexpr auto name_v = "Floating Point Numbers";

    // Converts from a string to a GUID (Does not need to be a string... but you should never change it!)
    static constexpr auto guid_v = xresource::type_guid("float");

    // This is a magic value used for debugging 
    static constexpr float magic_v = 0.1234f;

    static float* Load(xresource::mgr& Mgr, xresource::guid GUID) noexcept
    {
        auto Texture = std::make_unique<float>(magic_v);
        return Texture.release();
    }

    static void Destroy(float& Data, xresource::mgr& Mgr, xresource::guid GUID) noexcept
    {
        if (Data != magic_v)
        {
            printf("ERROR: Failed to get the original value...\n");
            s_bFailed = true;
        }
        else
        {
            s_bFailed = false;
            delete& Data;
        }
    }
};

//------------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------------
namespace xresource::unitest::resource_type_registration
{
    namespace details
    {
        //------------------------------------------------------------------------------------
        float TestSimpleType()
        {
            printf("    TestSimpleType... ");

            xresource::mgr Mgr;

            Mgr.RegisterTypes< int >();

            xresource::guid Guid = xresource::CreateUniqueGuid();

            xresource::ref<int> ResourceRef;
            ResourceRef.m_PRef.m_GUID = Guid;

            for( int i=0; i< 1000; ++i )
            {
                if( auto p = Mgr.getResource(ResourceRef); p == nullptr )
                {
                    printf( "ERROR: Unable to find an int resource...\n");
                    return 0;
                }
                else
                {
                    if( *p != xresource::type<int>::magic_v )
                    {
                        printf("ERROR: fail to get the data from our resource...\n");
                        return 0;
                    }
                }

                if (Guid != Mgr.getInstanceGuid(ResourceRef))
                {
                    printf("ERROR: fail to get retive the original guid...\n");
                    return 0.4f;
                }

                // Release our resource now...
                Mgr.ReleaseRef(ResourceRef);

                if( ResourceRef.m_PRef.isPointer() )
                {
                    printf("ERROR: At reference was still a pointer even when the resource was already released...\n");
                    return 0.4f;
                }
            }

            printf("OK.\n");
            return 1;
        }

        //------------------------------------------------------------------------------------

        float TestMultipleTypes()
        {
            xcore::random::small_generator Rnd;

            printf("    TestMultipleTypes... ");

            xresource::mgr Mgr;

            Mgr.RegisterTypes< int, float >();

            xresource::guid GuidInt   = xresource::CreateUniqueGuid();
            xresource::guid GuidFloat = xresource::CreateUniqueGuid();

            xresource::ref<int> IntRef;
            IntRef.m_PRef.m_GUID = GuidInt;

            xresource::ref<float> FloatRef;
            FloatRef.m_PRef.m_GUID = GuidFloat;

            for( int i=0; i<1000; ++i )
            {
                if( Rnd.RandU32()&1 )
                {
                    if( auto p = Mgr.getResource(IntRef); p == nullptr )
                    {
                        printf("ERROR: Unable to find an int resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::type<int>::magic_v)
                        {
                            printf("ERROR: fail to get the int data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if( Rnd.RandU32() & 1 )
                {
                    if (auto p = Mgr.getResource(FloatRef); p == nullptr)
                    {
                        printf("ERROR: Unable to find an float resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::type<float>::magic_v)
                        {
                            printf("ERROR: fail to get the float data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if (GuidInt != Mgr.getInstanceGuid(IntRef))
                {
                    printf("ERROR: fail to get retive the original int guid...\n");
                    return 0.0f;
                }

                if (GuidFloat != Mgr.getInstanceGuid(FloatRef))
                {
                    printf("ERROR: fail to get retive the original float guid...\n");
                    return 0.0f;
                }

                if (Rnd.RandU32() & 1)
                {
                    if( IntRef.m_PRef.isPointer() )
                    {
                        Mgr.ReleaseRef(IntRef);
                    }
                }

                if (Rnd.RandU32() & 1)
                {
                    if (FloatRef.m_PRef.isPointer())
                    {
                        Mgr.ReleaseRef(FloatRef);
                    }
                }
            }

            if (IntRef.m_PRef.isPointer()) Mgr.ReleaseRef(IntRef);
            if (FloatRef.m_PRef.isPointer()) Mgr.ReleaseRef(FloatRef);

            printf("OK.\n");
            return 1;
        }

        //------------------------------------------------------------------------------------

        float TestMultipleReferences()
        {
            xcore::random::small_generator Rnd;

            printf("    TestMultipleReferences... ");

            xresource::mgr Mgr;

            Mgr.RegisterTypes< int, float >();

            xresource::guid GuidFloat = xresource::CreateUniqueGuid();

            std::vector<xresource::ref<int>>     IntRef;
            std::vector<xresource::ref<float>>   FloatRef;

            for (int i = 0; i < 1000; ++i)
            {
                if (Rnd.RandU32() & 1)
                {
                    int Index = Rnd.RandU32()%(IntRef.size()+1);
                    if( Index == IntRef.size() || (Rnd.RandU32()&1) ) 
                    {
                        if( (Rnd.RandU32()&1) && Index != IntRef.size())
                        {
                            // Clone Reference... (The the Ref count if it is a pointer should go up!)
                            Mgr.CloneRef(IntRef[Index], IntRef[Rnd.RandU32() % IntRef.size()]);
                        }
                        else
                        {
                            // We are going to create a new resource here...
                            if( Index == IntRef.size() || (Rnd.RandU32() & 1) )
                            {
                                Index = static_cast<int>(IntRef.size());
                                IntRef.push_back({});
                            }
                            else
                            {
                                Mgr.ReleaseRef(IntRef[Index]);
                            }
                            
                            IntRef[Index].m_PRef.m_GUID = xresource::CreateUniqueGuid();
                        }
                    }                    

                    if (auto p = Mgr.getResource(IntRef[Index]); p == nullptr)
                    {
                        printf("ERROR: Unable to find an int resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::type<int>::magic_v)
                        {
                            printf("ERROR: fail to get the int data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if (Rnd.RandU32() & 1)
                {
                    int Index = Rnd.RandU32() % (FloatRef.size()+1);
                    if (Index == FloatRef.size() || (Rnd.RandU32() & 1))
                    {
                        if ((Rnd.RandU32() & 1) && Index != FloatRef.size())
                        {
                            // Clone Reference... (The the Ref count if it is a pointer should go up!)
                            Mgr.CloneRef(FloatRef[Index], FloatRef[Rnd.RandU32() % FloatRef.size()]);
                        }
                        else
                        {
                            // We are going to create a new resource here...
                            if (Index == FloatRef.size() || (Rnd.RandU32() & 1))
                            {
                                Index = static_cast<int>(FloatRef.size());
                                FloatRef.push_back({});
                            }
                            else
                            {
                                Mgr.ReleaseRef(FloatRef[Index]);
                            }

                            FloatRef[Index].m_PRef.m_GUID = xresource::CreateUniqueGuid();
                        }
                    }

                    if (auto p = Mgr.getResource(FloatRef[Index]); p == nullptr)
                    {
                        printf("ERROR: Unable to find an float resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::type<float>::magic_v)
                        {
                            printf("ERROR: fail to get the float data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if ( (Rnd.RandU32() & 1) && IntRef.size() )
                {
                    int Index = Rnd.RandU32() % IntRef.size();
                    Mgr.ReleaseRef(IntRef[Index]);
                }

                if ( (Rnd.RandU32() & 1) && FloatRef.size() )
                {
                    int Index = Rnd.RandU32() % FloatRef.size();
                    Mgr.ReleaseRef(FloatRef[Index]);
                }
            }

            //
            // Release everything...
            //
            for (auto& E : IntRef)
            {
                Mgr.ReleaseRef(E);
            }

            for (auto& E : FloatRef)
            {
                Mgr.ReleaseRef(E);
            }

            if (Mgr.getResourceCount())
            {
                printf("ERROR: fail to get the float data from our resource...\n");
                return 0.5f;
            }

            printf("OK.\n");
            
            return 1;
        }
    }

    //--------------------------------------------------------------------------------

    float Evaluate()
    {
        printf("\n\nEvaluating Resource Manager...\n");
        xcore::vector2 Grade(0, 0);
        Grade += xcore::vector2(details::TestSimpleType(), 1);
        Grade += xcore::vector2(details::TestMultipleTypes(), 1);
        Grade += xcore::vector2(details::TestMultipleReferences(), 1);

        float Total = Grade.m_X / Grade.m_Y;
        printf("Section Score: %3.0f%%", Total * 100);
        return Total;
    }
}
