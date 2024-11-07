# PCGLayeredBiomes

PCG Layered Biomes is a plugin for Unreal Engine that enables the creation of realistic, multilayered biomes using PCG (Procedural Content Generation). The plugin includes only code and blueprints, no additional assets.

> **Note**: This project is provided as-is and may not be actively developed or maintained due to limited time. Contributions and forks are welcome for anyone interested in further developing it.

## Key Features

### Core Idea
The plugin divides the generation area into distinct biomes, each with separate logic and objects for unique biome generation. This setup allows for diverse biome environments with specific characteristics.

### Layered Generation
Biome generation is handled layer by layer. Each layer is represented by a single PCG graph that references previously generated geometry, minimizing overlap and optimizing spatial coherence within each biome.

### Flexibility
This approach enables the development of biomes with varying complexity without creating overly complex PCG graphs, providing a streamlined and efficient process for biome customization.

### PCG Knowledge Not Required
No prior knowledge of PCG is necessary to utilize this plugin. A set of ready-made layers is included, allowing users to achieve substantial results right out of the box.

### Mesh-to-Actor Replacement with State Persistence (UE 5.4 ONLY)
For Unreal Engine 5.4, the plugin supports the seamless replacement of meshes with actors, which is useful for dynamic interactions like tree chopping. This feature also includes support for saving and loading the game's last state, ensuring continuity in gameplay.

- A sample project demonstrating this feature is available.
- Learn more: [Watch the explanation](https://youtu.be/bSDhaORGlew)

> **Note**: For an advanced and flexible alternative, consider using the built-in PCG Biomes Core solution available in UE 5.4.

## Additional Resources

- **Documentation**: [PCG Layered Biomes Documentation](https://pcg-layered-biomes.snazzydocs.com)
- **Brief Overview Video**: [Watch here](https://youtu.be/--WgVOKVePA)
- **Tutorial**: [Watch the tutorial](https://youtu.be/w_Hq9T3iizU)

## Support
For assistance, please contact: lazycatsdev@gmail.com
