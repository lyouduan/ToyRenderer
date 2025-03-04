# Toy Renderer on DirectX 12

A toy  render written in C++ using DirectX 12.



## Basis

- D3D12 Memory  Management
  - Buddy Allocator
- D3D12 Descriptor Management
- Dear ImGUI
- Assimp



## Feature

- PBR and  IBL

![IBL](./screenshot/IBL.gif)

- Shadows

![Shadow](./screenshot/Shadow.jpg)

- Cascaded Shadow Map

![CSM](./screenshot/CSMM.jpg)

- Tile Based Deferred Rendering

![TBDR](./screenshot/TBDR.png)

- Forward Plus

![TBDR](./screenshot/ForwardPlus.png)

- HBAO

![TBDR](./screenshot/HBAO.jpg)

- Temporal AA、FXAA

  ![TBDR](./screenshot/AA.jpg)

- SH

![TBDR](./screenshot/SH.jpg)


## TODO

- [x] PBR and  IBL
- [x] Shadows	
  - [x] PCF、PCSS、VSM、VSSM、ESM、EVSM
- [x] Cascaded Shadow Map
- [x] Tile Based Deferred Rendering
- [x] Forward Plus
- [x] SSAO\HBAO
- [x] Temporal AA
- [x] FXAA
- [x] Spherical harmonic
- [ ] SSR
- [ ] Bloom
