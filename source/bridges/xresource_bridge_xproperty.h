
#include "dependencies/xproperty/source/xcore_properties.h"

namespace xresource
{
    template< type_guid T_TYPE_GUID_V >
    struct property_ref_friend
    {
        using type = xresource::def_guid<T_TYPE_GUID_V>;

        XPROPERTY_DEF
        ("RscRef", type
        , obj_member
            < "InstanceGuid"
            , +[](type& O){ return O.m_Instance.m_Value }
            , member_help<"This is the instance GUID"
            >>
        , obj_member
            < "TypeGuid"
            , +[](type& O) { return O.m_Instance.m_Value }
            , member_help<"This is the type GUID"
            >>
        )
    };

    template<type_guid T_TYPE_GUID_V>
    inline const decltype(propperty_ref_friend<T_TYPE_GUID_V>::PropertiesDefinition()) g_PropertyRegistration_v{};
}
