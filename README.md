# xresource_mgr: Advanced Resource Management in C++

`xresource_mgr` is a robust C++ library for managing resources with unique identifiers, built on top of the `xresource` GUID library. It provides a flexible and type-safe resource management system, enabling efficient loading, tracking, and cleanup of resources like textures, models, or other assets in performance-critical applications.

## Key Features

- **Type-Safe Resource Management**: Uses templated GUIDs to ensure compile-time type safety for resources.
- **Reference Counting**: Automatically tracks resource usage with reference counting for efficient memory management.
- **Death March Support**: Optional delayed resource deletion to prevent frame-bound issues in real-time applications.
- **Customizable Loaders**: Define custom resource loaders for different types, with minimal dependencies via header-only declarations.
- **Dynamic Resource Paths**: Constructs resource paths based on GUIDs and type names for organized asset storage.
- **Modern C++20**: Utilizes modern C++ features for clean, maintainable code.
- **No External Dependencies**: Relies only on the C++ Standard Library and `xresource_guid`.
- **MIT License**: Free and open for all use cases.

## Dependencies

- [xresource_guid](https://github.com/LIONant-depot/xresource_guid): Required for GUID generation and management.

## Code Example

Below is an example demonstrating how to define and use a texture resource with `xresource_mgr`.

```cpp
#include "xresource_mgr.h"

// Define a texture resource type
namespace xrsc {
    inline static constexpr auto texture_type_guid_v = xresource::type_guid(xresource::guid_generator::Instance64FromString("texture"));
    using texture = xresource::def_guid<texture_type_guid_v>;
}

// Define the loader for texture
template<>
struct xresource::loader<xrsc::texture_type_guid_v> {
    constexpr static inline auto type_name_v = L"Texture";
    using data_type = xgpu::texture;
    constexpr static inline auto use_death_march_v = false;

    static data_type* Load(xresource::mgr& Mgr, const full_guid& GUID) {
        auto path = Mgr.getResourcePath(GUID, type_name_v);
        auto data = std::make_unique<xgpu::texture>();
        data->m_X = 22; // Example initialization
        return data.release();
    }

    static void Destroy(xresource::mgr& Mgr, data_type&& Data, const full_guid& GUID) {
        delete &Data;
    }
};

// Register the loader
inline static xresource::loader_registration<xrsc::texture_type_guid_v> texture_loader;

int main() {
    xresource::mgr mgr;
    mgr.Initiallize();

    xrsc::texture tex;
    tex.m_Instance = xresource::instance_guid::GenerateGUID();

    // Load resource
    auto* texture = mgr.getResource(tex);
    if (texture) {
        assert(texture->m_X == 22); // Verify loaded data
    }

    // Release resource
    mgr.ReleaseRef(tex);
    mgr.OnEndFrameDelegate(); // Process cleanup

    return 0;
}
```

## Usage

1. **Define Resource Types**: Create GUIDs for resource types using `xresource::type_guid` and `xresource::def_guid`.
2. **Implement Loaders**: Define `xresource::loader` for each resource type, specifying `Load` and `Destroy` functions.
3. **Register Loaders**: Use `xresource::loader_registration` to register loaders with the manager.
4. **Manage Resources**: Initialize `xresource::mgr`, load resources with `getResource`, and release them with `ReleaseRef`.
5. **Handle Frame Updates**: Call `OnEndFrameDelegate` to process delayed deletions (death march).

## Installation

1. Include `xresource_guid.h` and `xresource_mgr.h` in your project.
2. Implement loader logic in a `.cpp` file to minimize dependencies.
3. Link against the C++ Standard Library (C++20).

## Contributing

Star, fork, and contribute to `xresource_mgr` on GitHub! 🚀 Submit issues or PRs to enhance functionality or documentation.