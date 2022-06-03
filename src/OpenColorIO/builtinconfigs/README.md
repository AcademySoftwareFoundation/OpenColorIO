# Built-in configuration

Built-in configurations functionality provides built-in configurations to be used by a developer or end-user. 

The goal is to offer some configurations that can be used without any external files.

## File structure

The configuration files (.ocio) go into the `configs` folder.

Depending on the type of configuration, it has to be added to the suitable C++ files.

As of now, there are only CG (Computer Graphic) configs available. 
The following **three steps** must be followed to add a new CG configuration :

1. The configuration file (.ocio) must be added to the `configs` folder.
2. A new line must be added to `CG.cpp.in` :

   ```C++
   constexpr char NAME_OF_THE_OCIO_FILES[] = { @original_configuration_filename@ };
   ```
    ### Example A
    Let's say that the configuration `cg-config-v0.20.0_aces-v1.3_ocio-v2.1.1.ocio` has to be added. The following line must be added to `CG.cpp.in`.
    ```C++
    constexpr char CG_CONFIG_V0200_ACES_V13_OCIO_V211[] = { @cg-config-v0.20.0_aces-v1.3_ocio-v2.1.1@ };
    ```
3. A new line must be added to the `Register function` in `CGConfig.cpp`.
    ### Follow up from Example A
    ```C++
    namespace CGCONFIG
    {
        // Register CG configs.
        void Register(BuiltinConfigRegistryImpl & registry) noexcept
        {
            [... other addBuiltin lines ... ]
            
            // Below is the new line.
            registry.addBuiltin(
                "cg-config-v0.20.0_aces-v1.3_ocio-v2.1.1",
                CG_CONFIG_V0200_ACES_V13_OCIO_V211,
                true
            );
        }

    } // namespace CGCONFIG
    ```

    The variable `CG_CONFIG_V0200_ACES_V13_OCIO_V211` is from **step 2**, and the first parameter of the `addBuiltin` method is the name of the configuration (same as the filename).

    The third parameter tells us that the configuration is still recommended (not deprecated).