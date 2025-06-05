//------------------------------------------------------------------------------------
// Resource Type Registration
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------

using int_guid = xresource::def_guid< "Int" >;
template<>
struct xresource::loader<int_guid::m_Type>
{
    static inline bool s_bFailed;

    using data_type = int;

    // This is a magic value used for debugging  not required by the system
    static constexpr int magic_v = 512;

    static int* Load( xresource::mgr& Mgr, const xresource::full_guid& GUID ) noexcept;
    static void Destroy(xresource::mgr& Mgr, int&& Data, const xresource::full_guid& GUID ) noexcept;
};
inline static xresource::loader_registration<int_guid::m_Type> int_loader;

//------------------------------------------------------------------------------------

inline int* xresource::loader<int_guid::m_Type>::Load(xresource::mgr& Mgr, const xresource::full_guid& GUID) noexcept
{
    auto Texture = std::make_unique<int>(magic_v);
    return Texture.release();
}

//------------------------------------------------------------------------------------

inline void  xresource::loader<int_guid::m_Type>::Destroy(xresource::mgr& Mgr, int&& Data, const xresource::full_guid& GUID) noexcept
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

//------------------------------------------------------------------------------------

using float_guid = xresource::def_guid< "float" >;

template<>
struct xresource::loader<float_guid::m_Type>
{
    static inline bool s_bFailed;

    using data_type = float;

    // This is a nice name that we can associate with this resource... useful for editors and such
    static constexpr auto name_v = "Floating Point Numbers";

    // This is a magic value used for debugging not required by the system
    static constexpr float magic_v = 0.1234f;

    static float* Load(xresource::mgr& Mgr, xresource::full_guid GUID) noexcept
    {
        auto Texture = std::make_unique<float>(magic_v);
        return Texture.release();
    }

    static void Destroy(xresource::mgr& Mgr, float&& Data, xresource::full_guid GUID) noexcept
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
inline static xresource::loader_registration<float_guid::m_Type> float_loader;

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

            Mgr.Initiallize();

            const xresource::instance_guid Guid = xresource::guid_generator::Instance64();
            assert(Guid.isValid() && Guid.isPointer() == false);

            int_guid ResourceRef;
            ResourceRef.m_Instance = Guid;

            for( int i=0; i< 1000; ++i )
            {
                if( auto p = Mgr.getResource(ResourceRef); p == nullptr )
                {
                    printf( "ERROR: Unable to find an int resource...\n");
                    return 0;
                }
                else
                {
                    if( *p != xresource::loader<int_guid::m_Type>::magic_v )
                    {
                        printf("ERROR: fail to get the data from our resource...\n");
                        return 0;
                    }
                }

                if (Guid != Mgr.getFullGuid(ResourceRef).m_Instance)
                {
                    printf("ERROR: fail to get retive the original guid...\n");
                    return 0.4f;
                }

                // Release our resource now...
                Mgr.ReleaseRef(ResourceRef);

                if( ResourceRef.m_Instance.isPointer() )
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
            std::mt19937 Rnd;

            printf("    TestMultipleTypes... ");

            xresource::mgr Mgr;

            Mgr.Initiallize();

            int_guid    GuidInt   = {xresource::guid_generator::Instance64()};
            float_guid  GuidFloat = {xresource::guid_generator::Instance64()};

            int_guid IntRef;
            IntRef = GuidInt;

            float_guid FloatRef;
            FloatRef = GuidFloat;

            for( int i=0; i<1000; ++i )
            {
                if(Rnd() & 1)
                {
                    if( auto p = Mgr.getResource(IntRef); p == nullptr )
                    {
                        printf("ERROR: Unable to find an int resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::loader<int_guid::m_Type>::magic_v)
                        {
                            printf("ERROR: fail to get the int data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if(Rnd() & 1 )
                {
                    if (auto p = Mgr.getResource(FloatRef); p == nullptr)
                    {
                        printf("ERROR: Unable to find an float resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::loader<float_guid::m_Type>::magic_v)
                        {
                            printf("ERROR: fail to get the float data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if (GuidInt != Mgr.getFullGuid(IntRef))
                {
                    printf("ERROR: fail to get retive the original int guid...\n");
                    return 0.0f;
                }

                if (GuidFloat != Mgr.getFullGuid(FloatRef))
                {
                    printf("ERROR: fail to get retive the original float guid...\n");
                    return 0.0f;
                }

                if (Rnd() & 1)
                {
                    if( IntRef.m_Instance.isPointer() )
                    {
                        Mgr.ReleaseRef(IntRef);
                    }
                }

                if (Rnd() & 1)
                {
                    if (FloatRef.m_Instance.isPointer())
                    {
                        Mgr.ReleaseRef(FloatRef);
                    }
                }
            }

            if (IntRef.m_Instance.isPointer()) Mgr.ReleaseRef(IntRef);
            if (FloatRef.m_Instance.isPointer()) Mgr.ReleaseRef(FloatRef);

            printf("OK.\n");
            return 1;
        }

        //------------------------------------------------------------------------------------

        float TestMultipleReferences()
        {
            std::mt19937 Rnd(2274);

            printf("    TestMultipleReferences... ");

            xresource::mgr Mgr;

            Mgr.Initiallize();

            float_guid GuidFloat = {xresource::guid_generator::Instance64()};

            std::vector<int_guid>     IntRef;
            std::vector<float_guid>   FloatRef;

            for (int i = 0; i < 1000; ++i)
            {
                if (Rnd() & 1)
                {
                    int Index = Rnd()%(IntRef.size()+1);
                    if( Index == IntRef.size() || (Rnd()&1) )
                    {
                        if( (Rnd()&1) && Index != IntRef.size())
                        {
                            // Clone Reference... (The the Ref count if it is a pointer should go up!)
                            Mgr.CloneRef(IntRef[Index], IntRef[Rnd() % IntRef.size()]);
                        }
                        else
                        {
                            // We are going to create a new resource here...
                            if( Index == IntRef.size() || (Rnd() & 1) )
                            {
                                Index = static_cast<int>(IntRef.size());
                                IntRef.push_back({});
                            }
                            else
                            {
                                Mgr.ReleaseRef(IntRef[Index]);
                            }
                            
                            IntRef[Index].m_Instance = xresource::guid_generator::Instance64();
                        }
                    }                    

                    if (auto p = Mgr.getResource(IntRef[Index]); p == nullptr)
                    {
                        printf("ERROR: Unable to find an int resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::loader<int_guid::m_Type>::magic_v)
                        {
                            printf("ERROR: fail to get the int data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if (Rnd() & 1)
                {
                    int Index = Rnd() % (FloatRef.size()+1);
                    if (Index == FloatRef.size() || (Rnd() & 1))
                    {
                        if ((Rnd() & 1) && Index != FloatRef.size())
                        {
                            // Clone Reference... (The the Ref count if it is a pointer should go up!)
                            Mgr.CloneRef(FloatRef[Index], FloatRef[Rnd() % FloatRef.size()]);
                        }
                        else
                        {
                            // We are going to create a new resource here...
                            if (Index == FloatRef.size() || (Rnd() & 1))
                            {
                                Index = static_cast<int>(FloatRef.size());
                                FloatRef.push_back({});
                            }
                            else
                            {
                                Mgr.ReleaseRef(FloatRef[Index]);
                            }

                            FloatRef[Index].m_Instance = xresource::guid_generator::Instance64();
                        }
                    }

                    if (auto p = Mgr.getResource(FloatRef[Index]); p == nullptr)
                    {
                        printf("ERROR: Unable to find an float resource...\n");
                        return 0;
                    }
                    else
                    {
                        if (*p != xresource::loader<float_guid::m_Type>::magic_v)
                        {
                            printf("ERROR: fail to get the float data from our resource...\n");
                            return 0;
                        }
                    }
                }

                if ( (Rnd() & 1) && IntRef.size() )
                {
                    int Index = Rnd() % IntRef.size();
                    Mgr.ReleaseRef(IntRef[Index]);
                }

                if ( (Rnd() & 1) && FloatRef.size() )
                {
                    int Index = Rnd() % FloatRef.size();
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
        float Grade = 0;
        Grade += details::TestSimpleType();
        Grade += details::TestMultipleTypes();
        Grade += details::TestMultipleReferences();

        float Total = Grade / 3;
        printf("Section Score: %3.0f%%", Total * 100);
        return Total;
    }
}
