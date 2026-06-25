// auto handles dependencies, buffering, resource management
namespace Arawn {

    enum Queue { GRAPHICS, COMPUTE, TRANSFER, PRESENT };

    template<typename T> struct Meta;
    template<typename T> struct Instance;
    template<typename T> struct Handle { Meta<T>* meta; Instance<T>* instance; };
    template<typename T> struct CreateInfo;

    struct Offset3D { int32_t x, y, z; };
    struct Extent3D { uint32_t x, y, z; };
    struct Rect3D { Offset3D offset; Extent3D extent; };

    struct Offset2D { int32_t x, y; };
    struct Extent2D { uint32_t x, y; };
    struct Rect2D { Offset2D offset; Extent2D extent; };

    struct Cmd {
        enum Type {
            // TRANSFER
            CLEAR_COLOR_IMAGE,
            CLEAR_DEPTH_IMAGE,
            RESOLVE_IMAGE,
            COPY_BUFFER,
            UPDATE_BUFFER,
            FILL_BUFFER,
            COPY_IMAGE_TO_BUFFER,
            COPY_BUFFER_TO_IMAGE,
            COPY_IMAGE,
            
            // GRAPHICS
            BIND_GRAPHICS_PIPELINE, 
            BIND_DESCRIPTOR_SETS,
            PUSH_CONSTANTS,
            BIND_BUFFER,
            DRAW,
            DRAW_INDEXED,
            DRAW_INDIRECT,
            DRAW_INDEXED_INDIRECT,
            BLIT_IMAGE,
            CLEAR_ATTACHMENTS,
            
            // COMPUTE
            BIND_COMPUTE_PIPELINE,
            DISPATCH,
            DISPATCH_INDIRECT,
        } type;
    };
    
    struct Domain : Handle<Domain> { };
    template<> struct CreateInfo<Domain> { uint32_t index, buffering, frequency, phase; };
    struct Buffer : Handle<Buffer> {
        enum Binding {
            INDEX,
            VERTEX_0,
            VERTEX_1,
            VERTEX_2,
            VERTEX_3,
        };
        struct Copy;
        struct ImageCopy;
    };
    template<> struct CreateInfo<Buffer> { 
        Handle<Domain> domain;
    };

    struct Image : Handle<Image> {
        enum Aspect {
            ASPECT_COLOR_BIT = 0x00000001,
            ASPECT_DEPTH_BIT = 0x00000002,
            ASPECT_STENCIL_BIT = 0x00000004,
            ASPECT_METADATA_BIT = 0x00000008,
            ASPECT_PLANE_0_BIT = 0x00000010,
            ASPECT_PLANE_1_BIT = 0x00000020,
            ASPECT_PLANE_2_BIT = 0x00000040,
            ASPECT_NONE = 0,
            ASPECT_MEMORY_PLANE_0_BIT_EXT = 0x00000080,
            ASPECT_MEMORY_PLANE_1_BIT_EXT = 0x00000100,
            ASPECT_MEMORY_PLANE_2_BIT_EXT = 0x00000200,
            ASPECT_MEMORY_PLANE_3_BIT_EXT = 0x00000400,
            ASPECT_PLANE_0_BIT_KHR = ASPECT_PLANE_0_BIT,
            ASPECT_PLANE_1_BIT_KHR = ASPECT_PLANE_1_BIT,
            ASPECT_PLANE_2_BIT_KHR = ASPECT_PLANE_2_BIT,
            ASPECT_NONE_KHR = ASPECT_NONE,
            ASPECT_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
        };
        
        enum Layout {
            LAYOUT_UNDEFINED = 0,
            LAYOUT_GENERAL = 1,
            LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
            LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
            LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
            LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
            LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
            LAYOUT_TRANSFER_DST_OPTIMAL = 7,
            LAYOUT_PREINITIALIZED = 8,
            LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
            LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
            LAYOUT_DEPTH_ATTACHMENT_OPTIMAL = 1000241000,
            LAYOUT_DEPTH_READ_ONLY_OPTIMAL = 1000241001,
            LAYOUT_STENCIL_ATTACHMENT_OPTIMAL = 1000241002,
            LAYOUT_STENCIL_READ_ONLY_OPTIMAL = 1000241003,
            LAYOUT_READ_ONLY_OPTIMAL = 1000314000,
            LAYOUT_ATTACHMENT_OPTIMAL = 1000314001,
            LAYOUT_RENDERING_LOCAL_READ = 1000232000,
            LAYOUT_PRESENT_SRC_KHR = 1000001002,
            LAYOUT_VIDEO_DECODE_DST_KHR = 1000024000,
            LAYOUT_VIDEO_DECODE_SRC_KHR = 1000024001,
            LAYOUT_VIDEO_DECODE_DPB_KHR = 1000024002,
            LAYOUT_SHARED_PRESENT_KHR = 1000111000,
            LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT = 1000218000,
            LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR = 1000164003,
            LAYOUT_VIDEO_ENCODE_DST_KHR = 1000299000,
            LAYOUT_VIDEO_ENCODE_SRC_KHR = 1000299001,
            LAYOUT_VIDEO_ENCODE_DPB_KHR = 1000299002,
            LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT = 1000339000,
            LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR = 1000553000,
            LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR = LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
            LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR = LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
            LAYOUT_SHADING_RATE_OPTIMAL_NV = LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
            LAYOUT_RENDERING_LOCAL_READ_KHR = LAYOUT_RENDERING_LOCAL_READ,
            LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR = LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR = LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
            LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR = LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
            LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR = LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
            LAYOUT_READ_ONLY_OPTIMAL_KHR = LAYOUT_READ_ONLY_OPTIMAL,
            LAYOUT_ATTACHMENT_OPTIMAL_KHR = LAYOUT_ATTACHMENT_OPTIMAL,
            LAYOUT_MAX_ENUM = 0x7FFFFFFF
        };

        enum Filter {
            VK_FILTER_NEAREST = 0,
            VK_FILTER_LINEAR = 1,
            VK_FILTER_CUBIC_EXT = 1000015000,
            VK_FILTER_CUBIC_IMG = VK_FILTER_CUBIC_EXT,
            VK_FILTER_MAX_ENUM = 0x7FFFFFFF
        };

        enum LoadOp {
            LOAD_OP_LOAD = 0,
            LOAD_OP_CLEAR = 1,
            LOAD_OP_DONT_CARE = 2,
            LOAD_OP_NONE = 1000400000,
        };
        
        enum StoreOp {
            STORE_OP_STORE = 0,
            STORE_OP_DONT_CARE = 1,
            STORE_OP_NONE = 1000301000,
        };

        enum ResolveMode {
            RESOLVE_MODE_NONE = 0,
            RESOLVE_MODE_SAMPLE_ZERO_BIT = 0x00000001,
            RESOLVE_MODE_AVERAGE_BIT = 0x00000002,
            RESOLVE_MODE_MIN_BIT = 0x00000004,
            RESOLVE_MODE_MAX_BIT = 0x00000008,
            RESOLVE_MODE_EXTERNAL_FORMAT_DOWNSAMPLE_ANDROID = 0x00000010,
        };

        struct Meta {
            uint32_t width, height, depth;
            Layout layout;
        };

        struct SubresourceLayers;
        struct SubresourceRange;

        struct Copy;
        struct BufferCopy;
        struct Blit;
        struct Resolve;
        struct ClearDepth;
        union ClearColor;
        union ClearValue;
        struct ClearRect;
        struct ClearAttachment;

        struct Attachment;
    };

    template<> struct CreateInfo<Image> { 
        Handle<Domain> domain;
        VK_TYPE(VkImage)* external;
    };

    struct Pipeline : Handle<Pipeline> { 
        enum BindPoint {
            VK_PIPELINE_BIND_POINT_GRAPHICS = 0,
            VK_PIPELINE_BIND_POINT_COMPUTE = 1,
        };

        enum Stage {
            VERTEX = 0x00000001,
            GEOMETRY = 0x00000008,
            FRAGMENT = 0x00000010,
        };
    };

    template<> struct CreateInfo<Pipeline> {

    };

    struct DescriptorSet :  Handle<DescriptorSet> {

    };
    template<> struct CreateInfo<DescriptorSet> {
        Handle<Domain> domain;
    };
    
    union Image::ClearColor {
        ClearColor(float r, float g, float b, float a);
        ClearColor(int32_t r, int32_t g, int32_t b, int32_t a);
        ClearColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a);


        float       f32[4];
        int32_t     i32[4];
        uint32_t    u32[4];
    };

    struct Image::ClearDepth {
        float       depth;
        uint32_t    stencil;
    };

    union Image::ClearValue {
        ClearColor color;
        ClearDepth depth;
    };

    struct Image::ClearAttachment {
        uint32_t   aspectMask;
        uint32_t   colorAttachment;
        ClearValue clearValue;
    };
    
    struct Image::SubresourceLayers {
        uint32_t aspectMask = 0;
        uint32_t mipLevel = 0;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
    };
    

    struct Image::SubresourceRange {
        uint32_t aspectMask;
        uint32_t baseMipLevel;
        uint32_t levelCount;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };

    struct Image::Copy {
        SubresourceLayers srcSubresource;
        Offset3D srcOffset;
        SubresourceLayers dstSubresource;
        Offset3D dstOffset;
        Extent3D extent;
    };

    struct Image::Blit {
        SubresourceLayers srcSubresource;
        Offset3D srcOffsets[2];
        SubresourceLayers dstSubresource;
        Offset3D dstOffsets[2];
    };

    struct Image::ClearRect {
        Rect2D      rect;
        uint32_t    baseArrayLayer;
        uint32_t    layerCount;
    };

    struct Image::BufferCopy {
        std::size_t bufferOffset;
        uint32_t bufferRowLength;
        uint32_t bufferImageHeight;
        SubresourceLayers imageSubresource;
        Offset3D imageOffset;
        Extent3D imageExtent;
    };

    struct Image::Resolve {
        SubresourceLayers srcSubresource;
        Offset3D srcOffset;
        SubresourceLayers dstSubresource;
        Offset3D dstOffset;
        Extent3D extent;
    };

    struct Image::Attachment {
        Handle<Image>      image;
        Image::ResolveMode resolveMode;
        Handle<Image>      resolve;            
        Image::LoadOp      loadOp;
        Image::StoreOp     storeOp;            
        Image::ClearValue  clearValue;
    };

    struct Buffer::Copy {
        std::size_t srcOffset, dstOffset, size;
    };
    struct Buffer::ImageCopy {
        std::size_t bufferOffset;
        uint32_t                    bufferRowLength;
        uint32_t                    bufferImageHeight;
        Image::SubresourceLayers    imageSubresource;
        Offset3D                    imageOffset;
        Extent3D                    imageExtent;
    };
    



    struct CmdCopyImage : Cmd {
        Handle<Image> src;
        Handle<Image> dst;
        std::span<const Image::Copy> regions;
    };
    struct CmdCopyBufferToImage : Cmd { 
        Handle<Buffer> src;
        Handle<Image> dst;
        std::span<const Image::BufferCopy> regions;
    };
    struct CmdBlitImage : Cmd {
        Handle<Image> src;
        Handle<Image> dst;
        std::span<const Image::Blit> regions;
        Image::Filter filter;
    };
    struct CmdResolveImage : Cmd { 
        Handle<Image> src;
        Handle<Image> dst;
        std::span<const Image::Resolve> regions;
    };
    struct CmdClearColorImage : Cmd {
        Handle<Image> img;
        Image::ClearColor color;
        std::span<const Image::SubresourceRange> ranges;

    };
    struct CmdClearDepthImage : Cmd { 
        Handle<Image> img;
        Image::ClearDepth pDepth;
        std::span<const Image::SubresourceRange> ranges;
    };

    struct CmdCopyBuffer : Cmd { 
        Handle<Buffer> src;
        Handle<Buffer> dst;
        std::span<Buffer::Copy> regions;
    };
    struct CmdCopyImageToBuffer : Cmd { 
        Handle<Image> src;
        Handle<Buffer> dst;
        std::span<Buffer::ImageCopy> regions;
    };
    struct CmdUpdateBuffer : Cmd { 
        Handle<Buffer> buf;
        std::size_t offset;
        std::span<const std::byte> pData;
    };
    struct CmdFillBuffer : Cmd { 
        Handle<Buffer> buf;
        std::size_t offset;
        std::size_t size;
        uint32_t data;
    };
    struct CmdBindBuffer : Cmd { 
        Handle<Buffer> buf;
        Buffer::Binding binding;
        std::size_t offset, size, stride;
    };
    struct CmdBindPipeline : Cmd {
        Handle<Pipeline> pipeline;
    };
    struct CmdBindDescriptorSets : Cmd {
        std::span<Handle<DescriptorSet>> sets;
    };
    struct CmdPushConstants : Cmd {
        std::span<const std::byte> data;
    };
    struct CmdDraw : Cmd {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };
    struct CmdDrawIndirect : Cmd { 
        Handle<Buffer> buffer; // buffer<CmdDraw>
        std::size_t offset;
        uint32_t drawCount;
        uint32_t stride;
    };
    struct CmdDrawIndexed : Cmd { 
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t firstInstance;
    };
    struct CmdDrawIndexedIndirect : Cmd { 
        Handle<Buffer> buffer;
        std::size_t offset;
        uint32_t drawCount;
        uint32_t stride;
    };
    struct CmdDispatch : Cmd { 
        uint32_t groupCountX;
        uint32_t groupCountY;
        uint32_t groupCountZ;
    };
    struct CmdDispatchIndirect : Cmd { 
        Handle<Buffer> buffer;
        std::size_t offset;
    };
    
    struct CmdClearAttachments : Cmd { };
    // vkCmdPipelineBarrier

    struct Pass {
        
        // transfer cmds
        void copy(Handle<Image> src, Handle<Image> dst, const std::vector<Image::Copy>& regions) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
            
            Image::Copy* cachedRegions = static_cast<Image::Copy*>(cache.allocate(sizeof(Image::Copy) * regions.size(), alignof(Image::Copy)));
            std::copy(regions.data(), regions.data() + regions.size(), cachedRegions);
            
            CmdCopyImage* cmd = static_cast<CmdCopyImage*>(cache.allocate(sizeof(CmdCopyImage), alignof(CmdCopyImage)));
            std::construct_at(cmd, Cmd{ .type = Cmd::COPY_IMAGE },  src, dst, std::span{ cachedRegions, regions.size() });

            cmds->push_back(cmd);
        }
        void copy(Handle<Buffer> src, Handle<Image> dst, const std::vector<Image::BufferCopy>& regions) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Image::BufferCopy* cachedRegions = static_cast<Image::BufferCopy*>(cache.allocate(sizeof(Image::BufferCopy) * regions.size(), alignof(Image::BufferCopy)));
            std::copy(regions.data(), regions.data() + regions.size(), cachedRegions);
            
            CmdCopyBufferToImage* cmd = static_cast<CmdCopyBufferToImage*>(cache.allocate(sizeof(CmdCopyBufferToImage), alignof(CmdCopyBufferToImage)));
            std::construct_at(cmd, Cmd{ .type = Cmd::COPY_BUFFER_TO_IMAGE },  src, dst, std::span{ cachedRegions, regions.size() });

            cmds->push_back(cmd);
        }
        void blit(Handle<Image> src, Handle<Image> dst, const std::vector<Image::Blit>& regions, Image::Filter filter) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Image::Blit* cachedRegions = static_cast<Image::Blit*>(cache.allocate(sizeof(Image::Blit) * regions.size(), alignof(Image::Blit)));
            std::copy(regions.data(), regions.data() + regions.size(), cachedRegions);
            
            CmdBlitImage* cmd = static_cast<CmdBlitImage*>(cache.allocate(sizeof(CmdBlitImage), alignof(CmdBlitImage)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BLIT_IMAGE },  src, dst, std::span{ cachedRegions, regions.size() }, filter);

            cmds->push_back(cmd);
        }
        void resolve(Handle<Image> src, Handle<Image> dst, const std::vector<Image::Resolve>& regions) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Image::Resolve* cachedRegions = static_cast<Image::Resolve*>(cache.allocate(sizeof(Image::Resolve) * regions.size(), alignof(Image::Resolve)));
            std::copy(regions.data(), regions.data() + regions.size(), cachedRegions);
            
            CmdResolveImage* cmd = static_cast<CmdResolveImage*>(cache.allocate(sizeof(CmdResolveImage), alignof(CmdResolveImage)));
            std::construct_at(cmd, Cmd{ .type = Cmd::RESOLVE_IMAGE },  src, dst, std::span{ cachedRegions, regions.size() });

            cmds->push_back(cmd);
        }
        void clear(Handle<Image> img, const Image::ClearColor& color, const std::vector<Image::SubresourceRange>& ranges) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Image::SubresourceRange* cachedRanges = static_cast<Image::SubresourceRange*>(cache.allocate(sizeof(Image::SubresourceRange) * ranges.size(), alignof(Image::SubresourceRange)));
            std::copy(ranges.data(), ranges.data() + ranges.size(), cachedRanges);
            
            CmdClearColorImage* cmd = static_cast<CmdClearColorImage*>(cache.allocate(sizeof(CmdClearColorImage), alignof(CmdClearColorImage)));
            std::construct_at(cmd, Cmd{ .type = Cmd::CLEAR_COLOR_IMAGE }, img, color, std::span{ cachedRanges, ranges.size() });

            cmds->push_back(cmd);
        }
        void clear(Handle<Image> img, const Image::ClearDepth& depth, const std::vector<Image::SubresourceRange>& ranges) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Image::SubresourceRange* cachedRanges = static_cast<Image::SubresourceRange*>(cache.allocate(sizeof(Image::SubresourceRange) * ranges.size(), alignof(Image::SubresourceRange)));
            std::copy(ranges.data(), ranges.data() + ranges.size(), cachedRanges);
            
            CmdClearDepthImage* cmd = static_cast<CmdClearDepthImage*>(cache.allocate(sizeof(CmdClearDepthImage), alignof(CmdClearDepthImage)));
            std::construct_at(cmd, Cmd{ .type = Cmd::CLEAR_DEPTH_IMAGE }, img, depth, std::span{ cachedRanges, ranges.size() });

            cmds->push_back(cmd);
        }
        // buffer
        void copy(Handle<Buffer> src, Handle<Buffer> dst, const std::vector<Buffer::Copy>& regions) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
                        
            Buffer::Copy* cachedRegions = static_cast<Buffer::Copy*>(cache.allocate(sizeof(Buffer::Copy) * regions.size(), alignof(Buffer::Copy)));
            std::copy(regions.data(), regions.data() + regions.size(), cachedRegions);

            CmdCopyBuffer* cmd = static_cast<CmdCopyBuffer*>(cache.allocate(sizeof(CmdCopyBuffer), alignof(CmdCopyBuffer)));
            std::construct_at(cmd, Cmd{ .type = Cmd::COPY_BUFFER },  src, dst, std::span { cachedRegions, regions.size() });

            cmds->push_back(cmd);
        }
        void copy(Handle<Image> src, Handle<Buffer> dst, const std::vector<Buffer::ImageCopy>& regions) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Buffer::ImageCopy* cachedRegions = static_cast<Buffer::ImageCopy*>(cache.allocate(sizeof(Buffer::ImageCopy) * regions.size(), alignof(Buffer::ImageCopy)));
            std::copy(regions.data(), regions.data() + regions.size(), cachedRegions);

            CmdCopyImageToBuffer* cmd = static_cast<CmdCopyImageToBuffer*>(cache.allocate(sizeof(CmdCopyImageToBuffer), alignof(CmdCopyImageToBuffer)));
            std::construct_at(cmd, Cmd{ .type = Cmd::COPY_IMAGE_TO_BUFFER },  src, dst, std::span{ cachedRegions, regions.size() });

            cmds->push_back(cmd);
        }
        void update(Handle<Buffer> buf, std::size_t offset, std::span<std::byte> data) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
            
            std::byte* cachedData = static_cast<std::byte*>(cache.allocate(data.size()));
            std::copy(data.data(), data.data() + data.size(), cachedData);

            CmdUpdateBuffer* cmd = static_cast<CmdUpdateBuffer*>(cache.allocate(sizeof(CmdUpdateBuffer), alignof(CmdUpdateBuffer)));
            std::construct_at(cmd, Cmd{ .type = Cmd::UPDATE_BUFFER }, buf, offset, std::span{ cachedData, data.size() });

            cmds->push_back(cmd);
        }
        void fill(Handle<Buffer> buf, std::size_t offset, std::size_t size, uint32_t data) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            CmdFillBuffer* cmd = static_cast<CmdFillBuffer*>(cache.allocate(sizeof(CmdFillBuffer), alignof(CmdFillBuffer)));
            std::construct_at(cmd, Cmd{ .type = Cmd::FILL_BUFFER }, buf, offset, size, data);

            cmds->push_back(cmd);
        }

        // graphics cmds
        void bind(Handle<Pipeline> pipeline) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            CmdBindPipeline* cmd = static_cast<CmdBindPipeline*>(cache.allocate(sizeof(CmdBindPipeline), alignof(CmdBindPipeline)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_GRAPHICS_PIPELINE }, pipeline);
        }
        
        void bind(const std::vector<Handle<DescriptorSet>>& sets, uint32_t offset = 0) {
            // TODO: resource dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores

            Handle<DescriptorSet>* cachedSets = static_cast<Handle<DescriptorSet>*>(cache.allocate(sizeof(Handle<DescriptorSet>) * sets.size(), alignof(Handle<DescriptorSet>)));
            CmdBindDescriptorSets* cmd = static_cast<CmdBindDescriptorSets*>(cache.allocate(sizeof(CmdBindDescriptorSets), alignof(CmdBindDescriptorSets)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_DESCRIPTOR_SETS }, std::span{ cachedSets, sets.size() });
        }

        template<typename T>
        void push(const T& data) {
            std::span<std::byte> constants = { static_cast<std::byte*>(cache.allocate(sizeof(T), alignof(T))), sizeof(T) };
            CmdPushConstants* cmd = static_cast<CmdPushConstants*>(cache.allocate(sizeof(CmdPushConstants), alignof(CmdPushConstants)));
            std::construct_at(cmd, Cmd{ .type = Cmd::PUSH_CONSTANTS }, constants);
        }



        void bind(Handle<Buffer> buf, Buffer::Binding binding, std::size_t offset, std::size_t size) {
            // TODO: graph dependencies dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
            
            CmdBindBuffer* cmd = static_cast<CmdBindBuffer*>(cache.allocate(sizeof(CmdBindBuffer), alignof(CmdBindBuffer)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_BUFFER }, buf, binding, offset, size);
        }
        
        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) {
            
            CmdDraw* cmd = static_cast<CmdDraw*>(cache.allocate(sizeof(CmdDraw), alignof(CmdDraw)));
            std::construct_at(cmd, Cmd{ .type = Cmd::DRAW }, vertexCount, instanceCount, firstVertex, firstInstance);
        }
        void draw(Handle<Buffer> indirect, std::size_t offset, uint32_t drawCount) { // indirect 
            // TODO: graph dependencies dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
            
            CmdDrawIndirect* cmd = static_cast<CmdDrawIndirect*>(cache.allocate(sizeof(CmdDrawIndirect), alignof(CmdDrawIndirect)));
            std::construct_at(cmd, Cmd{ .type = Cmd::DRAW_INDIRECT }, indirect, offset, drawCount);
        }

        void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
            CmdDrawIndexed* cmd = static_cast<CmdDrawIndexed*>(cache.allocate(sizeof(CmdDrawIndexed), alignof(CmdDrawIndexed)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_BUFFER }, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }
        void drawIndexed(Handle<Buffer> indirect, std::size_t offset, uint32_t drawCount) { // indirect
            // TODO: graph dependencies dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
            
            CmdDrawIndexedIndirect* cmd = static_cast<CmdDrawIndexedIndirect*>(cache.allocate(sizeof(CmdDrawIndexedIndirect), alignof(CmdDrawIndexedIndirect)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_BUFFER }, indirect, offset, drawCount);
        }

        void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
            
            CmdDispatch* cmd = static_cast<CmdDispatch*>(cache.allocate(sizeof(CmdDispatch), alignof(CmdDispatch)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_BUFFER }, groupCountX, groupCountY, groupCountZ);
        }
        void dispatch(Handle<Buffer> indirect, std::size_t offset) { // indirect
            // TODO: graph dependencies dependencies and insert barriers
            // TODO: acquire pass dependencies to insert semaphores
            
            CmdDispatchIndirect* cmd = static_cast<CmdDispatchIndirect*>(cache.allocate(sizeof(CmdDispatchIndirect), alignof(CmdDispatchIndirect)));
            std::construct_at(cmd, Cmd{ .type = Cmd::BIND_BUFFER }, indirect, offset);
        }
        Queue queue;
        
        std::pmr::monotonic_buffer_resource cache;
        std::vector<const Cmd*>* cmds;

        
    };

    struct TransferPass {
        void copy(Handle<Image> src, Handle<Image> dst, const std::vector<Image::Copy>& regions) { pass->copy(src, dst, regions); }
        void copy(Handle<Buffer> src, Handle<Image> dst, const std::vector<Image::BufferCopy>& regions) { pass->copy(src, dst, regions); }
        void blit(Handle<Image> src, Handle<Image> dst, const std::vector<Image::Blit>& regions, Image::Filter filter) { pass->blit(src, dst, regions, filter); }
        void resolve(Handle<Image> src, Handle<Image> dst, const std::vector<Image::Resolve>& regions) { pass->resolve(src, dst, regions); }
        void clear(Handle<Image> img, const Image::ClearColor& color, const std::vector<Image::SubresourceRange>& ranges) { pass->clear(img, color, ranges); }
        void clear(Handle<Image> img, const Image::ClearDepth& depth, const std::vector<Image::SubresourceRange>& ranges) { pass->clear(img, depth, ranges); }
        void copy(Handle<Buffer> src, Handle<Buffer> dst, const std::vector<Buffer::Copy>& regions) { pass->copy(src, dst, regions); }
        void copy(Handle<Image> src, Handle<Buffer> dst, const std::vector<Buffer::ImageCopy>& regions) { pass->copy(src, dst, regions); }
        void update(Handle<Buffer> buf, std::size_t offset, std::span<std::byte> data) { pass->update(buf, offset, data); }
        void fill(Handle<Buffer> buf, std::size_t offset, std::size_t size, uint32_t data) { pass->fill(buf, offset, size, data); }

    protected:
        Pass* pass;

    };    
    struct GraphicsPass : TransferPass { 
        void bind(Handle<Pipeline> pipeline) { pass->bind(pipeline); }
        void bind(const std::vector<Handle<DescriptorSet>>& bindings, uint32_t descSetOffset = 0) { pass->bind(bindings, descSetOffset); }
        template<typename T> void push(const T& push_constants = {}) { pass->template push<T>(push_constants); }
        void bind(Handle<Buffer> buf, Buffer::Binding binding, std::size_t offset, std::size_t size) { pass->bind(buf, binding, offset, size); }
        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) { pass->draw(vertexCount, instanceCount, firstVertex, firstInstance); }
        void draw(Handle<Buffer> indirect, std::size_t offset, uint32_t drawCount) { pass->draw(indirect, offset, drawCount); }
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) { pass->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance); }
        void drawIndexed(Handle<Buffer> indirect, std::size_t offset, uint32_t drawCount) { pass->drawIndexed(indirect, offset, drawCount); }
    
    private:
        Pass* pass;        
    };   
    struct ComputePass : TransferPass {
        void bind(Handle<Pipeline> pipeline) { pass->bind(pipeline); }
        void bind(const std::vector<Handle<DescriptorSet>>& bindings, uint32_t descSetOffset = 0) { pass->bind(bindings, descSetOffset); }
        template<typename T> void push(const T& push_constants = {}) { pass->template push<T>(push_constants); }
        void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) { pass->dispatch(groupCountX, groupCountY, groupCountZ); }
        void dispatch(Handle<Buffer> indirect, std::size_t offset) { pass->dispatch(indirect, offset); }
    };

    template<> struct CreateInfo<TransferPass> { };
    template<> struct CreateInfo<GraphicsPass> { std::vector<Image::Attachment> colorAttachments;
        std::optional<Image::Attachment> depthAttachment;
        std::optional<Image::Attachment> resolveAttachment;
    };
    template<> struct CreateInfo<ComputePass> { };

    struct Engine {
        template<typename T>
        [[nodiscard]] T create(CreateInfo<T>&& info);
        
        template<typename T>
        void destroy(Handle<T> hnd);
        
    };

    

    struct Vertex { };
    struct CullingInfo { } cullingInfo;
    void example() {
        Engine engine;
        auto static_domain = engine.create<Domain>({ .buffering = 1 });
        auto swap_domain = engine.create<Domain>({ .buffering = 3 });
        auto frame_domain = engine.create<Domain>({ .buffering = 2 });
        auto asnc_domain = engine.create<Domain>({ .buffering = 2 });

        auto ebo = engine.create<Buffer>({ .domain = static_domain });
        auto vbo = engine.create<Buffer>({ .domain = static_domain });
        
        auto ubo = engine.create<Buffer>({ .domain = frame_domain });
        auto ibo = engine.create<Buffer>({ .domain = frame_domain }); // indirect buffer object
        auto tbo = engine.create<Image>({ .domain = static_domain });
        
        auto color_attachment = engine.create<Image>({ .domain = frame_domain });
        auto depth_attachment = engine.create<Image>({ .domain = frame_domain });
        auto present_attachment = engine.create<Image>({ .domain = frame_domain });

        VK_TYPE(VkImage)* swap_imgs;
        auto swapchain_img = engine.create<Image>({ .domain = swap_domain, .external = swap_imgs });

        auto set_0 = engine.create<DescriptorSet>({ .domain = swap_domain });
        auto set_1 = engine.create<DescriptorSet>({ .domain = frame_domain });
        auto pipeline = engine.create<Pipeline>({ });
        
        ComputePass hierarchy_pass = engine.create<ComputePass>({ });
        hierarchy_pass.bind(pipeline); // culling pipeline
        hierarchy_pass.bind({ set_0, set_1 });
        hierarchy_pass.dispatch(2048, 1, 1);
        
        ComputePass transform_pass = engine.create<ComputePass>({ });
        transform_pass.bind(pipeline); // culling pipeline
        transform_pass.bind({ set_0, set_1 });
        transform_pass.dispatch(2048, 1, 1);
        
        

        GraphicsPass depth_pass = engine.create<GraphicsPass>({ 
            .colorAttachments = {},
            .depthAttachment = { 
                Image::Attachment{ 
                    .image = depth_attachment,
                    .loadOp = Image::LOAD_OP_CLEAR,
                    .storeOp = Image::STORE_OP_STORE,
                    .clearValue = { .depth = { 1000.0f } }
                }
            }
        });
        depth_pass.bind(pipeline);
        depth_pass.bind(ebo, Buffer::INDEX, 0, sizeof(int32_t)); // index buffer
        depth_pass.bind(vbo, Buffer::VERTEX_0, 0, sizeof(Vertex)); // vertex buffer    
        depth_pass.draw(ibo, 0, 1);
        
        ComputePass culling_pass = engine.create<ComputePass>({ });
        culling_pass.bind(pipeline); // culling pipeline
        culling_pass.bind({ set_0, set_1 });
        culling_pass.push(cullingInfo);
        culling_pass.dispatch(2048, 1, 1);

        // TODO: derive domain usage from resources
        auto forward_pass = engine.create<GraphicsPass>({ 
            .colorAttachments = { 
                { 
                    .image = color_attachment, 
                    .loadOp = Image::LOAD_OP_CLEAR,
                    .storeOp = Image::STORE_OP_STORE,
                    .clearValue = { .color = { 0.0f, 0.0f, 0.0f, 0.0f } }
                }
            }, 
        });
        forward_pass.bind(pipeline);
        forward_pass.bind(ebo, Buffer::INDEX, 0, sizeof(int32_t)); // index buffer
        forward_pass.bind(vbo, Buffer::VERTEX_0, 0, sizeof(Vertex)); // vertex buffer
        forward_pass.draw(8, 1);
        
        auto present_pass = engine.create<GraphicsPass>({ 
            .colorAttachments = {},
            .depthAttachment = {},
            .resolveAttachment = {},
        });
        present_pass.resolve(color_attachment, present_attachment, 
            { 
                {
                    .srcSubresource = { .aspectMask = Image::ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
                    .srcOffset = { 0, 0, 0 },
                    .dstSubresource = { .aspectMask = Image::ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
                    .dstOffset = { 0, 0, 0 },
                    .extent = { 800, 600 },
                }
            }
        );
    }
}

