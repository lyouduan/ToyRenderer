# 1 GPU显存管理

## 1.1 GPU资源

GPU资源是渲染管线所需要的资源，比如vertex buffer\index buffer\texture等等，在DX12中统一表示为ID3D12Resource。

但这些资源并不是直接绑定到渲染管线上，而是通过描述符这个中间对象进行间接管理；实质上是为了解耦资源和渲染流水线，提高资源管理的效率，在渲染管线切换资源开销比切换描述符更大。

这些资源都存放在GPU显存中，表示为ID3D12Heap。这也就涉及到了如何CPU内存和GPU显存之间的数据传输问题，即CPU如何把资源上传GPU中，以及GPU如何把资源回写到CPU；

根据堆的不同读写限制，常用有三种：

1）上传堆Upload Heap：CPU和GPU都可访问，把CPU提交的资源上传到GPU；

2）默认堆Default Heap：只能GPU访问，若需要与CPU传输数据，只能通过上传堆Upload Heap实现；

3）回读堆Readback Heap：从GPU回传资源，CPU可以从中读取资源；**（暂时没用到，不了解）**



相比于过去的gfx api，DX12提供了更底层的创建显存接口提高性能：

1）Committed Resource

ID3D12Device::CreateCommittedResource根据资源描述信息直接创建一个ID3D12Resource，实际上隐式创建了一个ID3D12Heap（由硬件负责管理），再将将对应的Resource映射到堆中；

2）Placed Resource

ID3D12Device::CreateHeap，先创建一个ID3D12Heap，再通过CreatePlacedResource在该Heap上分配给Resource;

3）ReservedResource**(暂时没用到，不了解)**

类似虚拟内存，使用的时候才将虚拟地址映射为heap上；



## 1.2 显存管理

首先，考虑把顶点数据上传到GPU显存中的情况，在没有显存管理的情况下：

1）创建上传堆，把顶点数据通过映射地址上传；

2）创建默认堆，将上传堆的顶点数据通过CopyBufferRegion拷贝到默认堆

3）安全释放上传堆（可能需要驻留，假设顶点数据不变）；

即动态分配显存，需要的时候才申请显存，对性能影响比较大，每次都需要申请创建上传堆和默认堆，以及拷贝的开销；同时可能使得GPU出现内存碎片问题；

因此，在追求性能优化时，显存管理是节省开销的关键一环。

加入显存管理后，只需要在初始化或者显存分配不足时进行创建，需要时从内存池中取出，在渲染管线不再需要的时候返还即可；



**显存管理方式**

**1）Circular Buffer**

环状缓冲区，以先进先出的方式分配和释放显存，使用于一次性资源（一帧的生命周期），即渲染管线使用过后不再需要，通过fence value进行释放，确保相应的帧完成渲染；

适用于Upload Buffer的分配；

**2）Segregated Free Lists**

分离队列，将不同大小块的链表放置在不同的链表中，适用于分配固定大小的内存，但容易内部碎片化；

适用于PlacedResource;

**3）Buddy System**

伙伴分配算法，使用二叉树思想，分配的时候，将大块的内存不断二分，直到小于请求大小为止；

而合并的时候，需要检测兄弟节点是否空闲，进行合并处理，不断向上合并；

适用于PlacedResource，创建一个ID3D12Heap进行不断地划分；



**分配方案**

根据Resource和渲染资源的关系，可以将显存进行划分：

**1）StandAlone**

每次需要创建渲染资源的时候，直接调用CreateCommittedResource创建ID3D12Resource;

缺陷：即前面提到的，相当于没有内存管理，每次都需要动态分配显存，开销较大；

**2）SubAllocation**

在初始的时候，一次申请较大的显存空间，采用子分配的方式进行管理，可以分为两种策略：

（1）PlacedResource策略，先申请一块显存ID3D12Heap，再通过CreatePlacedResource方式创建不同的资源ID3D12Resource，允许资源的状态不一样（在CreatePlacedResource阶段填写相应的资源描述信息）；

（2）ManualSubAllocation策略，通过CreateCommittedResource方式创建一个ID3D12Resource，手动将ID3D12Resource的地址GPUVirtualAddress进行划分，需要注意**内存对齐**的问题。由于是共用一个ID3D12Resource，所以相应的渲染资源状态需要保持一致；



**ID3D12Resource封装**

按照资源数据类型区分，主要分为Buffer（顶点缓冲区，索引缓冲区，常量缓冲区，结构化缓冲区，无序访问缓冲区）和Texture（纹理，渲染目标，深度模版缓冲区）。

在MiniEngine中，ID3D12Resource通过Descriptor Heap区分不同的资源和访问方式。

**资源视图**

资源视图（即描述符）用于定义和管理GPU资源的访问方式；



将分配器作为封装的一部分，申请内存交由分配器管理，封装部分管理相关的Handle和View；
