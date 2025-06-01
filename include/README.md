# Zerialize

```mermaid

graph TB
    %% Main entry point
    Z[zerialize.hpp] --> E[errors.hpp]
    Z --> C[concepts.hpp]
    Z --> VT[value_type.hpp]
    Z --> D[deserializer.hpp]
    Z --> S[serializer.hpp]
    Z --> A[api.hpp]
    Z --> DU[debug_utils.hpp]

    %% Core dependencies
    VT --> C
    D --> C
    D --> E
    S --> C
    S --> E
    S --> AD[any_dispatch.hpp]
    A --> S
    A --> ZB[zbuffer.hpp]
    DU --> C
    DU --> VT
    AD --> C
    AD --> E

    %% Protocol implementations (optional)
    PM[protocols/msgpack.hpp] -.-> Z
    PJ[protocols/json.hpp] -.-> Z
    PF[protocols/flex.hpp] -.-> Z
    PMC[protocols/msgpack_c.hpp] -.-> Z
    PMO[protocols/msgpack_old.hpp] -.-> Z

    %% Tensor integrations (optional)
    TE[tensor/eigen.hpp] -.-> Z
    TX[tensor/xtensor.hpp] -.-> Z
    TU[tensor/utils.hpp] -.-> Z

    %% Utility files (standalone)
    TU2[testing_utils.hpp]

    %% Styling
    classDef coreFile fill:#e1f5fe,stroke:#01579b,stroke-width:2px
    classDef protocolFile fill:#f3e5f5,stroke:#4a148c,stroke-width:2px
    classDef tensorFile fill:#e8f5e8,stroke:#1b5e20,stroke-width:2px
    classDef utilFile fill:#fff3e0,stroke:#e65100,stroke-width:2px
    classDef mainFile fill:#ffebee,stroke:#c62828,stroke-width:3px

    class Z mainFile
    class E,C,VT,D,S,A,DU,AD,ZB coreFile
    class PM,PJ,PF,PMC,PMO protocolFile
    class TE,TX,TU tensorFile
    class TU2 utilFile
```

## File Organization

### Core Library Structure

The library is organized into a clean, modular structure:

- **Main Header**: `zerialize.hpp` - Single include for most users
- **Core Components**: Essential building blocks (concepts, base classes, errors)
- **API Layer**: User-facing serialization functions and convenience helpers
- **Utilities**: Debug tools and testing utilities

### Dependency Layers

1. **Foundation Layer**: `errors.hpp`, `concepts.hpp` (no internal dependencies)
2. **Core Layer**: `value_type.hpp`, `deserializer.hpp`, `any_dispatch.hpp` (depend on foundation)
3. **Serialization Layer**: `serializer.hpp` (depends on core + any_dispatch)
4. **API Layer**: `api.hpp` (depends on serializer + zbuffer)
5. **Utility Layer**: `debug_utils.hpp` (depends on concepts + value_type)

### Optional Extensions

- **Protocols**: Format-specific implementations (MessagePack, JSON, Flex)
- **Tensor Libraries**: Integration with Eigen, XTensor
- **Testing**: Utilities for testing and validation

### Usage Patterns

**Simple Usage**: Just include `zerialize.hpp`
```cpp
#include <zerialize/zerialize.hpp>
```

**Advanced Usage**: Include specific components for faster compilation
```cpp
#include <zerialize/serializer.hpp>  // Serialization only
#include <zerialize/protocols/json.hpp>  // JSON protocol
```
