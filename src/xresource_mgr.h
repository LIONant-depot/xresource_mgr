#include "xcore/src/xcore.h"

// This is an example of a xresource system
namespace xresource
{
    struct mgr;

    // The actual GUID
    // All resources GUIDs has the very first bit turn on
    // This bit servers as a flag to know if a reference is an actual GUID or a pointer
    using guid      = xcore::guid::unit<64, struct resource_guid_tag>;
    static inline guid CreateUniqueGuid( void ) noexcept { auto Guid = guid{xcore::not_null}; Guid.m_Value |= 1; return Guid; }

    // This is the type of the xresource described as a simple GUID
    using type_guid = xcore::guid::unit<64, struct resource_type_guid_tag>;

    // Resource type
    template< typename T_RESOURCE_STRUCT_TYPE >
    struct type
    {
        // Expected static parameters
        // constexpr static inline auto                 name_v = "Custom Resource Name";  // **** This is opcional *****
        // constexpr static inline xresource::type_guid  guid_v = { "ResourceName" };        
        // static T_RESOURCE_STRUCT_TYPE*  Load   (                               xresource::mgr& Mgr, xresource::guid GUID );
        // static void                     Destroy( T_RESOURCE_STRUCT_TYPE& Data, xresource::mgr& Mgr, xresource::guid GUID );
    };

    template< typename T, typename = void > struct get_custom_name                                               { static inline           const char* value = []{return typeid(T).name();}();  };
    template< typename T >                  struct get_custom_name< T, std::void_t< typename type<T>::name_v > > { static inline constexpr const char* value = type<T>::name_v;   };

    // this simple structure has the actual reference to the xresource
    // but it needs to know if the actual reference is a pointer or
    // is a GUID
    union partial_ref
    {
        guid    m_GUID;                 // This is 64 bits
        void*   m_Pointer;              // This is 64 bits

        constexpr bool isPointer() const { return (m_GUID.m_Value & 1) == 0; }
    };

    // This structure is able to hold a reference to any kind of xresource
    // This is also a full reference rather than partial
    struct universal_ref
    {
        partial_ref   m_PRef;
        type_guid     m_TypeGUID;

        // No copy allowed... 
                                universal_ref   ()                     = default;
                                universal_ref   (const universal_ref&) = delete;
        const universal_ref&    operator =      (const universal_ref&) = delete;

        // moves are ok
        universal_ref(universal_ref&& A)
        {
            m_PRef                  = A.m_PRef;
            m_TypeGUID              = A.m_TypeGUID;
            A.m_PRef.m_GUID.m_Value = A.m_TypeGUID.m_Value = 0;
        }

        universal_ref& operator = (universal_ref&& A)
        {
            m_PRef                  = A.m_PRef;
            m_TypeGUID              = A.m_TypeGUID;
            A.m_PRef.m_GUID.m_Value = A.m_TypeGUID.m_Value = 0;
            return *this;
        }

        // Destruction can not happen carelessly 
        ~universal_ref()
        {
            // No references can die with pointers they must be kill by the user properly 
            // Since we need to update the ref counts
            assert(m_PRef.isPointer() == false || m_PRef.m_GUID.m_Value == 0);
        }
    };

    // This structure is able to hold a reference to a particular xresource type
    // The type for T_TYPE_V could be:
    // xresource::type<texture>, xresource::type<sound>, etc...
    // This structure is also a full reference rather than partial
    template < typename T_RESOURCE_TYPE >
    struct ref
    {
        using                           type = T_RESOURCE_TYPE;
        partial_ref                     m_PRef;
        static inline constexpr auto&   m_TypeGUID = xresource::type<T_RESOURCE_TYPE>::guid_v;

        // No copy allowed... 
                                        ref             () = default;
                                        ref             (const ref&) = delete;
        const ref&                      operator =      (const ref&) = delete;

        // moves are ok
        ref(ref&& A)
        {
            m_PRef                   = A.m_PRef;
            A.m_PRef.m_GUID.m_Value  = 0;
        }

        ref& operator = (ref&& A)
        {
            m_PRef                   = A.m_PRef;
            A.m_PRef.m_GUID.m_Value  = 0;
            return *this;
        }

        // Destruction can not happen carelessly 
        ~ref()
        {
            // No references can die with pointers they must be kill by the user properly 
            // Since we need to update the ref counts
            assert( m_PRef.isPointer() == false || m_PRef.m_GUID.m_Value == 0 );
        }
    };

    //
    // RSC MANAGER
    //
    namespace details
    {
        struct instance_info
        {
            void* m_pData{ nullptr };
            xresource::guid  m_GUID;
            type_guid       m_TypeGUID;
            int             m_RefCount{ 1 };
        };

        struct universal_type
        {
            using load_fun      = void* ( xresource::mgr& Mgr, guid GUID );
            using destroy_fun   = void  ( void* pData, xresource::mgr& Mgr, xresource::guid GUID );

            type_guid       m_GUID;
            load_fun*       m_pLoadFunc;
            destroy_fun*    m_pDestroyFunc;
            const char*     m_pName;
        };
    }

    // Resource Manager
    struct mgr
    {
        //-------------------------------------------------------------------------
        mgr()
        {
            //
            // Initialize our memory manager of instance infos
            //
            for (int i = 0, end = (int)m_InfoBuffer.size() - 1; i != end; ++i)
            {
                m_InfoBuffer[i].m_pData = &m_InfoBuffer[i + 1];
            }
            m_InfoBuffer[m_InfoBuffer.size() - 1].m_pData = nullptr;
            m_pInfoBufferEmptyHead = m_InfoBuffer.data();
        }

        //-------------------------------------------------------------------------
        template< typename...T_ARGS >
        void RegisterTypes()
        {
            //
            // Insert all the types into the hash table
            //
            (   [&]< typename T >(T*)
                {
                    m_RegisteredTypes.emplace
                    ( type<T>::guid_v.m_Value
                    , details::universal_type
                        { {type<T>::guid_v}
                        , [](xresource::mgr& Mgr, guid GUID) constexpr -> void*
                            { return type<T>::Load(Mgr,GUID); }
                        , [](void* pData, xresource::mgr& Mgr, guid GUID) constexpr
                            { type<T>::Destroy(*reinterpret_cast<T*>(pData),Mgr,GUID); }
                        , get_custom_name<T>::value
                        }
                    );
                }(reinterpret_cast<T_ARGS*>(0))
                , ...
            );
        }

        //-------------------------------------------------------------------------
        template< typename T >
        T* getResource( xresource::ref<T>& R )
        {
            // If we already have the xresource return now
            if (R.m_PRef.isPointer()) return reinterpret_cast<T*>(R.m_PRef.m_Pointer);

            std::uint64_t HashID = R.m_PRef.m_GUID.m_Value ^ R.m_TypeGUID.m_Value;
            if( auto Entry = m_ResourceInstance.find(HashID); Entry != m_ResourceInstance.end() )
            {
                auto& E = *Entry->second;
                E.m_RefCount++;
                return reinterpret_cast<T*>(R.m_PRef.m_Pointer = E.m_pData);
            }

            T* pRSC = type<T>::Load(*this, R.m_PRef.m_GUID);
            // If the user return nulls it must mean that it failed to load so we could return a temporary xresource of the right type
            if (pRSC == nullptr) return nullptr;

            FullInstanceInfoAlloc(pRSC, R.m_PRef.m_GUID, R.m_TypeGUID);

            return reinterpret_cast<T*>(R.m_PRef.m_Pointer = pRSC);
        }

        //-------------------------------------------------------------------------
        void* getResource( universal_ref& URef )
        {
            // If we already have the xresource return now
            if (URef.m_PRef.isPointer()) return URef.m_PRef.m_Pointer;

            std::uint64_t HashID = URef.m_PRef.m_GUID.m_Value ^ URef.m_TypeGUID.m_Value;
            if( auto Entry = m_ResourceInstance.find(HashID); Entry != m_ResourceInstance.end() )
            {
                auto& E = *Entry->second;
                E.m_RefCount++;
                URef.m_PRef.m_Pointer = E.m_pData;
                return URef.m_PRef.m_Pointer;
            }

            auto UniversalType = m_RegisteredTypes.find(URef.m_TypeGUID.m_Value);
            assert(UniversalType != m_RegisteredTypes.end()); // Type was not registered

            void* pRSC = UniversalType->second.m_pLoadFunc(*this, URef.m_PRef.m_GUID );
            // If the user return nulls it must mean that it failed to load so we could return a temporary xresource of the right type
            if (pRSC == nullptr) return nullptr;

            FullInstanceInfoAlloc(pRSC, URef.m_PRef.m_GUID, URef.m_TypeGUID);

            return URef.m_PRef.m_Pointer = pRSC;
        }

        //-------------------------------------------------------------------------
        template< typename T >
        void ReleaseRef(xresource::ref<T>& Ref )
        {
            if (false == Ref.m_PRef.isPointer() || Ref.m_PRef.m_GUID.m_Value == 0 ) return;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(Ref.m_PRef.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            auto& R = *S->second;
            R.m_RefCount--;
            auto OriginalGuid = R.m_GUID;
            assert(R.m_TypeGUID == Ref.m_TypeGUID);
            assert(R.m_pData == Ref.m_PRef.m_Pointer );

            //
            // If this is the last reference release the xresource
            //
            if( R.m_RefCount == 0 )
            {
                type<T>::Destroy(*reinterpret_cast<T*>(Ref.m_PRef.m_Pointer), *this, R.m_GUID);
                FullInstanceInfoRelease(R);
            }

            Ref.m_PRef.m_GUID = OriginalGuid;
        }

        //-------------------------------------------------------------------------
        void ReleaseRef( universal_ref& URef )
        {
            if( false == URef.m_PRef.isPointer() || URef.m_PRef.m_GUID.m_Value == 0 ) return;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_PRef.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            auto& R = *S->second;
            R.m_RefCount--;
            auto OriginalGuid = R.m_GUID;
            assert(URef.m_TypeGUID == R.m_TypeGUID);
            assert(R.m_pData == URef.m_PRef.m_Pointer);

            //
            // If this is the last reference release the xresource
            //
            if (R.m_RefCount == 0)
            {
                auto UniversalType = m_RegisteredTypes.find(URef.m_TypeGUID.m_Value);
                assert(UniversalType != m_RegisteredTypes.end()); // Type was not registered

                UniversalType->second.m_pDestroyFunc(URef.m_PRef.m_Pointer, *this, R.m_GUID);

                FullInstanceInfoRelease(R);
            }

            URef.m_PRef.m_GUID = OriginalGuid;
        }

        template< typename T >
        guid getInstanceGuid( const xresource::ref<T>& R ) const
        {
            if (false == R.m_PRef.isPointer()) return R.m_PRef.m_GUID;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(R.m_PRef.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            return S->second->m_GUID;
        }

        // When serializing resources of displaying them in the editor you may want to show the GUID rather than the pointer
        // When reference holds the pointer rather than the GUID we must find the actual GUID to return to the user
        guid getInstanceGuid( const universal_ref& URef ) const
        {
            if (false == URef.m_PRef.isPointer()) return URef.m_PRef.m_GUID;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_PRef.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            return S->second->m_GUID;
        }

        template< typename T >
        void CloneRef( ref<T>& Dest, const ref<T>& Ref ) noexcept
        {
            if( Ref.m_PRef.isPointer() )
            {
                if (Dest.m_PRef.isPointer())
                {
                    if( Dest.m_PRef.m_Pointer == Ref.m_PRef.m_Pointer ) return;
                    ReleaseRef(Dest);
                }

                auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(Ref.m_PRef.m_Pointer));
                assert(S != m_ResourceInstanceRelease.end());

                S->second->m_RefCount++;
            }
            else
            {
                if (Dest.m_PRef.isPointer()) ReleaseRef(Dest);
            }
            Dest.m_PRef = Ref.m_PRef;
        }

        void CloneRef(universal_ref& Dest, const universal_ref& URef ) noexcept
        {
            if( URef.m_PRef.isPointer() )
            {
                if (Dest.m_PRef.isPointer())
                {
                    if (Dest.m_PRef.m_Pointer == URef.m_PRef.m_Pointer) return;
                    ReleaseRef(Dest);
                }

                auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_PRef.m_Pointer));
                assert(S != m_ResourceInstanceRelease.end());

                S->second->m_RefCount++;
            }
            else
            {
                if (Dest.m_PRef.isPointer()) ReleaseRef(Dest);
            }

            Dest.m_PRef     = URef.m_PRef;
            Dest.m_TypeGUID = URef.m_TypeGUID;
        }

        int getResourceCount()
        {
            assert( m_ResourceInstance.size() == m_ResourceInstanceRelease.size() );
            return static_cast<int>(m_ResourceInstance.size());
        }

    protected:

        details::instance_info& AllocRscInfo( void )
        {
            auto pTemp = m_pInfoBufferEmptyHead;
            assert(pTemp);

            details::instance_info* pNext = reinterpret_cast<details::instance_info*>(m_pInfoBufferEmptyHead->m_pData);
            m_pInfoBufferEmptyHead = pNext;

            return *pTemp;
        }

        void ReleaseRscInfo(details::instance_info& RscInfo)
        {
            // Add this xresource info to the empty chain
            RscInfo.m_pData = m_pInfoBufferEmptyHead;
            m_pInfoBufferEmptyHead = &RscInfo;
        }

        void FullInstanceInfoAlloc(void* pRsc, xresource::guid RscGUID, xresource::type_guid TypeGUID)
        {
            auto& RscInfo = AllocRscInfo();

            RscInfo.m_pData     = pRsc;
            RscInfo.m_GUID      = RscGUID;
            RscInfo.m_TypeGUID  = TypeGUID;
            RscInfo.m_RefCount  = 1;

            std::uint64_t HashID = RscGUID.m_Value ^ TypeGUID.m_Value;
            m_ResourceInstance.emplace( HashID, &RscInfo );
            m_ResourceInstanceRelease.emplace( reinterpret_cast<std::uint64_t>(pRsc), &RscInfo );
        }

        void FullInstanceInfoRelease( details::instance_info& RscInfo )
        {
            // Release references in the hashs maps
            m_ResourceInstance.erase(RscInfo.m_GUID.m_Value ^ RscInfo.m_TypeGUID.m_Value);
            m_ResourceInstanceRelease.erase( reinterpret_cast<std::uint64_t>(RscInfo.m_pData) );

            // Add this xresource info to the empty chain
            ReleaseRscInfo(RscInfo);
        }

        constexpr static auto max_resources_v = 1024;

        std::unordered_map<std::uint64_t, details::universal_type>  m_RegisteredTypes;
        std::unordered_map<std::uint64_t, details::instance_info*>  m_ResourceInstance;
        std::unordered_map<std::uint64_t, details::instance_info*>  m_ResourceInstanceRelease;
        details::instance_info*                                     m_pInfoBufferEmptyHead{ nullptr };
        std::array<details::instance_info, max_resources_v>         m_InfoBuffer;
    };
}

