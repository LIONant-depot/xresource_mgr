#include <map>
namespace xresource::unitest::guids
{
    static_assert( sizeof(xresource::guid) == 8, "Resource GUIDs for now should be 64bits!");

    namespace details
    {
        static constexpr int max_iterations_v = 10000;

        //------------------------------------------------------------------------------------

        float TestUniqueness()
        {
            printf("    TestUniqueness... ");

            //
            // Lets create a bunch of guids...
            //
            std::unordered_map<std::uint64_t, int>  m_ListOfGeneratedGuids;
            
            for (int i = 0; i < max_iterations_v; i++)
            {
                auto GUID = xresource::CreateUniqueGuid();
                auto GU64 = *reinterpret_cast<std::uint64_t*>(&GUID);
                
                if( auto f = m_ListOfGeneratedGuids.find(GU64); f != m_ListOfGeneratedGuids.end() )
                {
                    printf("ERROR: Your GUID generation is creating GUIDs that are colliding...\n");
                    return 0;
                }
            }
            
            printf("OK.\n");
            return 1;
        }

        //------------------------------------------------------------------------------------

        float TestFirstBit()
        {
            printf("    TestFirstBit... ");
            for (int i = 0; i < max_iterations_v; i++)
            {
                auto GUID = xresource::CreateUniqueGuid();
                auto GU64 = *reinterpret_cast<std::uint64_t*>(&GUID);

                if( (GU64&1) == 0 )
                {
                    printf("ERROR: Resource GUIDs should always have the first bit set to 1...\n");
                    return 0;
                }
            }
            printf("OK.\n");
            return 1;
        }

        //------------------------------------------------------------------------------------

        float TestTypeGuids()
        {
            printf("    TestTypeGuids... ");

            std::map<std::uint64_t, int>  m_ListOfGeneratedGuids;

            m_ListOfGeneratedGuids.emplace(xresource::type_guid("Int").m_Value,   0);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("int").m_Value,   1);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("a").m_Value,     2);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("asd").m_Value,   3);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("tasxx").m_Value, 4);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("tasxz").m_Value, 5);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("t4sxx").m_Value, 6);
            m_ListOfGeneratedGuids.emplace(xresource::type_guid("tadxx").m_Value, 7);

            for (int i = 0; i < 8; i++)
            {
                bool bFound = false;
                for (auto e = m_ListOfGeneratedGuids.begin(); e != m_ListOfGeneratedGuids.end(); e++ )
                {
                    if (e->second == i)
                    {
                        bFound = true;
                        break;
                    }
                }

                if( bFound == false )
                {
                    printf("ERROR: Resource Type GUIDs some how are failing because they are generating collisions...\n");
                    return 0;
                }
            }
            printf("OK.\n");
            return 1;
        }
    }

    //--------------------------------------------------------------------------------

    float Evaluate()
    {
        printf("\n\nEvaluating Resource GUIDs...\n");
        xcore::vector2 Grade( 0,0 );
        Grade += xcore::vector2( details::TestUniqueness(), 1 );
        Grade += xcore::vector2( details::TestFirstBit(),   1 );
        Grade += xcore::vector2( details::TestTypeGuids(),  1 );

        float Total = Grade.m_X / Grade.m_Y;
        printf("Section Score: %3.0f%%", Total * 100);
        return Total;
    }
}
