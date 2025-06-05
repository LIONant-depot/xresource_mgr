#ifndef XRESOURCE_MGR_H
#define XRESOURCE_MGR_H
#pragma once

#include <unordered_map>
#include "xresource/source/xresource_guid.h"

//----------------------------------------------------------------------------------
// Example on registering a resource type
//----------------------------------------------------------------------------------
// using texture_guid = xresource::guid_def<xresource::type_guid("texture")>;  // First we define the GUID
//
// template<>
// struct xresource::loader< texture_guid.m_Type >                  // Now we specify the loader and we must fill in all the information
// {
//      //--- Expected static parameters ---
//      constexpr static inline auto         name_v      = "Custom Resource Name";      // **** This is optional *****
//      using                                data_type   = xgpu::texture;               // This is the actual data type of the runtime resource itself...
//      static data_type*                    Load   ( xresource::mgr& Mgr,                    const full_guid& GUID );
//      static void                          Destroy( xresource::mgr& Mgr, data_type&& Data,  const full_guid& GUID );
// };
// After you have define the loader type you need to register it, like this...
// inline static xresource::loader_registration<texture_guid.m_Type> UniqueName;
//
// The implementation of the above functions should be done in a cpp file
// This will minimize dependencies and help keep the code clean
//----------------------------------------------------------------------------------

namespace xresource
{
    struct mgr;
    template< type_guid TYPE_GUID_V >
    struct loader {};

    namespace details
    {
        struct registration_base
        {
            inline static registration_base* s_pHead = {nullptr};
            registration_base*               m_pNext;

            registration_base() : m_pNext { s_pHead }
            {
                s_pHead = this;
            }
                                                            registration_base   ( registration_base&& )                                             = delete;
                                                            registration_base   ( const registration_base& )                                        = delete;
                            constexpr virtual              ~registration_base   ( void )                                                            = default;
                            const registration_base&        operator =          ( const registration_base& )                                        = delete;
                            const registration_base&        operator =          ( registration_base&& )                                             = delete;
            [[nodiscard]]   constexpr virtual void*         Load                ( xresource::mgr& Mgr,              const full_guid& GUID ) const   = 0;
                            constexpr virtual void          Destroy             ( xresource::mgr& Mgr, void* pData, const full_guid& GUID ) const   = 0;
            [[nodiscard]]   constexpr virtual const char*   getName             ( void )                                                    const   = 0;
            [[nodiscard]]   constexpr virtual type_guid     getTypeGuid         ( void )                                                    const   = 0;
        };

        template< type_guid TYPE_GUID_V, typename = void > struct get_custom_name                                                                     { static inline           const char* value = []{return typeid(loader<TYPE_GUID_V>::data_type).name(); }(); };
        template< type_guid TYPE_GUID_V >                  struct get_custom_name< TYPE_GUID_V, std::void_t< typename loader<TYPE_GUID_V>::name_v > > { static inline constexpr const char* value = loader<TYPE_GUID_V>::name_v; };
    }

    //
    // Registering a loader for a resource type
    //
    template< type_guid TYPE_GUID_V >
    struct loader_registration final : details::registration_base
    {
        using loader = loader<TYPE_GUID_V>;
        using type   = typename loader::data_type;

        [[nodiscard]] constexpr void* Load(xresource::mgr& Mgr, const full_guid& GUID) const override
        {
            return loader::Load(Mgr, GUID);
        }

        constexpr void Destroy(xresource::mgr& Mgr, void* pData, const full_guid& GUID) const override
        {
            loader::Destroy(Mgr, std::move(*static_cast<type*>(pData)), GUID);
        }

        [[nodiscard]] constexpr const char* getName() const override
        {
            return details::get_custom_name<TYPE_GUID_V>::value;
        }

        [[nodiscard]] constexpr type_guid getTypeGuid() const override
        {
            return TYPE_GUID_V;
        }
    };

    //
    // RSC MANAGER
    //
    namespace details
    {
        struct instance_info
        {
            void*           m_pData     = { nullptr };
            full_guid       m_Guid      = {};
            int             m_RefCount  = { 1 };
        };

        struct universal_type
        {
            type_guid                   m_TypeGUID;
            details::registration_base* m_pRegistration;
            const char*                 m_pName;
        };
    }

    // Resource Manager
    struct mgr
    {
        //-------------------------------------------------------------------------
        void Initiallize( std::size_t MaxResource = 1000 )
        {
            m_MaxResources = MaxResource;

            m_InfoBuffer = std::make_unique<details::instance_info[]>(m_MaxResources);

            //
            // Initialize our memory manager of instance infos
            //
            for (int i = 0, end = (int)m_MaxResources - 1; i != end; ++i)
            {
                m_InfoBuffer[i].m_pData = &m_InfoBuffer[i + 1];
            }
            m_InfoBuffer[m_MaxResources - 1].m_pData = nullptr;
            m_pInfoBufferEmptyHead = m_InfoBuffer.get();


            //
            // Insert all the types into the hash table
            //
            int TotalTypes = 0;
            for ( details::registration_base* p = details::registration_base::s_pHead; p ; p = p->m_pNext )
            {
                TotalTypes++;
            }

            m_RegisteredTypes.reserve(TotalTypes);

            for (details::registration_base* p = details::registration_base::s_pHead; p; p = p->m_pNext)
            {
                m_RegisteredTypes.emplace( p->getTypeGuid(), details::universal_type{ p->getTypeGuid(), p, p->getName() } );
            }
        }

        //-------------------------------------------------------------------------

        template< auto RSC_TYPE_V >
        typename loader<RSC_TYPE_V>::data_type* getResource( def_guid<RSC_TYPE_V>& R )
        {
            using data_type = typename loader<RSC_TYPE_V>::data_type;

            // If we already have the xresource return now
            if (R.m_Instance.isPointer()) return reinterpret_cast<data_type*>(R.m_Instance.m_Pointer);

            std::uint64_t HashID = std::hash<def_guid<RSC_TYPE_V>>{}(R);
            if( auto Entry = m_ResourceInstance.find(HashID); Entry != m_ResourceInstance.end() )
            {
                auto& E = *Entry->second;
                E.m_RefCount++;
                return reinterpret_cast<data_type*>(R.m_Instance.m_Pointer = E.m_pData);
            }

            data_type* pRSC = loader<RSC_TYPE_V>::Load(*this, R);
            // If the user return nulls it must mean that it failed to load so we could return a temporary xresource of the right type
            if (pRSC == nullptr) return nullptr;

            FullInstanceInfoAlloc(pRSC, R, HashID);

            return reinterpret_cast<data_type*>(R.m_Instance.m_Pointer = pRSC);
        }

        //-------------------------------------------------------------------------

        void* getResource( full_guid& URef )
        {
            // If we already have the xresource return now
            if (URef.m_Instance.isPointer()) return URef.m_Instance.m_Pointer;

            std::uint64_t HashID = std::hash<full_guid>{}(URef);
            if( auto Entry = m_ResourceInstance.find(HashID); Entry != m_ResourceInstance.end() )
            {
                auto& E = *Entry->second;
                E.m_RefCount++;
                URef.m_Instance.m_Pointer = E.m_pData;
                return URef.m_Instance.m_Pointer;
            }

            auto UniversalType = m_RegisteredTypes.find(URef.m_Type);
            assert(UniversalType != m_RegisteredTypes.end()); // Type was not registered

            void* pRSC = UniversalType->second.m_pRegistration->Load(*this, URef );
            // If the user return nulls it must mean that it failed to load so we could return a temporary xresource of the right type
            if (pRSC == nullptr) return nullptr;

            FullInstanceInfoAlloc(pRSC, URef, HashID);

            return URef.m_Instance.m_Pointer = pRSC;
        }

        //-------------------------------------------------------------------------

        template< auto RSC_TYPE_V >
        void ReleaseRef(def_guid<RSC_TYPE_V>& Ref )
        {
            if (Ref.m_Instance.isValid() == false || false == Ref.m_Instance.isPointer() ) return;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(Ref.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            auto& R = *S->second;
            --R.m_RefCount;
            auto OriginalGuid = R.m_Guid.m_Instance;
            assert(R.m_Guid.m_Type == Ref.m_Type);
            assert(R.m_pData == Ref.m_Instance.m_Pointer );

            //
            // If this is the last reference release the xresource
            //
            if( R.m_RefCount == 0 )
            {
                loader<RSC_TYPE_V>::Destroy(*this, std::move(*reinterpret_cast<typename loader<RSC_TYPE_V>::data_type*>(Ref.m_Instance.m_Pointer)), R.m_Guid);
                FullInstanceInfoRelease(R);
            }

            Ref.m_Instance = OriginalGuid;
        }

        //-------------------------------------------------------------------------

        void ReleaseRef( full_guid& URef )
        {
            if (URef.m_Instance.isValid() == false || false == URef.m_Instance.isPointer()) return;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            auto& R = *S->second;
            R.m_RefCount--;
            auto OriginalGuid = R.m_Guid.m_Instance;
            assert(URef.m_Type == R.m_Guid.m_Type);
            assert(R.m_pData == URef.m_Instance.m_Pointer);

            //
            // If this is the last reference release the xresource
            //
            if (R.m_RefCount == 0)
            {
                auto UniversalType = m_RegisteredTypes.find(URef.m_Type);
                assert(UniversalType != m_RegisteredTypes.end()); // Type was not registered

                UniversalType->second.m_pRegistration->Destroy(*this, R.m_pData,  R.m_Guid);

                FullInstanceInfoRelease(R);
            }

            URef.m_Instance = OriginalGuid;
        }

        //-------------------------------------------------------------------------

        template< auto RSC_TYPE_V >
        full_guid getFullGuid( const def_guid<RSC_TYPE_V>& R ) const
        {
            if (R.isValid() == false || false == R.m_Instance.isPointer()) return R;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(R.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            return S->second->m_Guid;
        }

        //-------------------------------------------------------------------------

        // When serializing resources of displaying them in the editor you may want to show the GUID rather than the pointer
        // When reference holds the pointer rather than the GUID we must find the actual GUID to return to the user
        full_guid getFullGuid( const full_guid& URef ) const
        {
            if (URef.isValid() == false || false == URef.m_Instance.isPointer()) return URef;

            auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_Instance.m_Pointer));
            assert(S != m_ResourceInstanceRelease.end());

            return S->second->m_Guid;
        }

        //-------------------------------------------------------------------------

        template<auto RSC_TYPE_V >
        void CloneRef( def_guid<RSC_TYPE_V>& Dest, const def_guid<RSC_TYPE_V>& Ref ) noexcept
        {
            if( Ref.isValid() && Ref.m_Instance.isPointer() )
            {
                if (Dest.isValid() && Dest.m_Instance.isPointer())
                {
                    if( Dest.m_Instance.m_Pointer == Ref.m_Instance.m_Pointer ) return;
                    ReleaseRef(Dest);
                }

                auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(Ref.m_Instance.m_Pointer));
                assert(S != m_ResourceInstanceRelease.end());

                ++S->second->m_RefCount;
            }
            else
            {
                if (Dest.isValid() && Dest.m_Instance.isPointer()) ReleaseRef(Dest);
            }
            Dest.m_Instance = Ref.m_Instance;
        }

        //-------------------------------------------------------------------------

        void CloneRef(full_guid& Dest, const full_guid& URef ) noexcept
        {
            if(URef.m_Instance.isValid() && URef.m_Instance.isPointer() )
            {
                if (Dest.m_Instance.isValid() && Dest.m_Instance.isPointer())
                {
                    if (Dest.m_Instance.m_Pointer == URef.m_Instance.m_Pointer) return;
                    ReleaseRef(Dest);
                }

                auto S = m_ResourceInstanceRelease.find(reinterpret_cast<std::uint64_t>(URef.m_Instance.m_Pointer));
                assert(S != m_ResourceInstanceRelease.end());

                S->second->m_RefCount++;
            }
            else
            {
                if (Dest.m_Instance.isValid() && Dest.m_Instance.isPointer()) ReleaseRef(Dest);
            }

            Dest = URef;
        }

        //-------------------------------------------------------------------------

        int getResourceCount() const noexcept
        {
            assert( m_ResourceInstance.size() == m_ResourceInstanceRelease.size() );
            return static_cast<int>(m_ResourceInstance.size());
        }

    protected:

        //-------------------------------------------------------------------------

        details::instance_info& AllocRscInfo( void ) noexcept
        {
            auto pTemp = m_pInfoBufferEmptyHead;
            assert(pTemp);

            details::instance_info* pNext = reinterpret_cast<details::instance_info*>(m_pInfoBufferEmptyHead->m_pData);
            m_pInfoBufferEmptyHead = pNext;

            return *pTemp;
        }

        //-------------------------------------------------------------------------

        void ReleaseRscInfo(details::instance_info& RscInfo) noexcept
        {
            // Add this xresource info to the empty chain
            RscInfo.m_pData = m_pInfoBufferEmptyHead;
            m_pInfoBufferEmptyHead = &RscInfo;
        }

        //-------------------------------------------------------------------------

        void FullInstanceInfoAlloc(void* pRsc, const full_guid& GUID, std::uint64_t HashID) noexcept
        {
            auto& RscInfo = AllocRscInfo();

            RscInfo.m_pData     = pRsc;
            RscInfo.m_Guid      = GUID;
            RscInfo.m_RefCount  = 1;

            m_ResourceInstance.emplace( HashID, &RscInfo );
            m_ResourceInstanceRelease.emplace( reinterpret_cast<std::uint64_t>(pRsc), &RscInfo );
        }

        //-------------------------------------------------------------------------

        void FullInstanceInfoRelease( details::instance_info& RscInfo ) noexcept
        {
            // Release references in the hashs maps
            m_ResourceInstance.erase( std::hash<full_guid>{}(RscInfo.m_Guid));
            m_ResourceInstanceRelease.erase( reinterpret_cast<std::uint64_t>(RscInfo.m_pData) );

            // Add this xresource info to the empty chain
            ReleaseRscInfo(RscInfo);
        }

        //-------------------------------------------------------------------------
        //-------------------------------------------------------------------------

        std::unordered_map<type_guid, details::universal_type>      m_RegisteredTypes           = {};
        std::unordered_map<std::uint64_t, details::instance_info*>  m_ResourceInstance          = {};
        std::unordered_map<std::uint64_t, details::instance_info*>  m_ResourceInstanceRelease   = {};
        details::instance_info*                                     m_pInfoBufferEmptyHead      = { nullptr };
        std::unique_ptr<details::instance_info[]>                   m_InfoBuffer                = {};
        std::size_t                                                 m_MaxResources              = {};
    };
}
#endif
