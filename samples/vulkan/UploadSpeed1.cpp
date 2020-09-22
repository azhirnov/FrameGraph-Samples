// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Memory upload test.

	Intel UHD 630 (PCIe 3, Laptop)
		Type: 0, Heap: 0, Flags: DeviceLocal | HostVisible | HostCoherent,              Speed: 12293 Mb/s
		Type: 1, Heap: 0, Flags: DeviceLocal | HostVisible | HostCoherent | HostCached, Speed:  8587 Mb/s
		Type: 1, Heap: 0, Flags: DeviceLocal | HostVisible | HostCoherent | HostCached, Speed:  7159 Mb/s
		Type: 0, Heap: 0, Flags: DeviceLocal | HostVisible | HostCoherent,              Speed:  9349 Mb/s
		Type: 1, Heap: 0, Flags: DeviceLocal | HostVisible | HostCoherent | HostCached, Speed:  7759 Mb/s
		
	Nvidia GTX 1070 (PCIe 3, Laptop)
		Type:  8, Heap: 1, Flags: HostVisible | HostCoherent,               Speed: 5897 Mb/s
		Type:  9, Heap: 1, Flags: HostVisible | HostCoherent | HostCached,  Speed: 9145 Mb/s
		Type:  9, Heap: 1, Flags: HostVisible | HostCoherent | HostCached,  Speed: 7270 Mb/s
		Type: 10, Heap: 2, Flags: DeviceLocal | HostVisible | HostCoherent, Speed: 4696 Mb/s
		
	Nvidia RTX 2080 (PCIe 2 16x)
		Type: 2, Heap: 1, Flags: HostVisible | HostCoherent,               Speed: 1681 Mb/s
		Type: 3, Heap: 1, Flags: HostVisible | HostCoherent | HostCached,  Speed: 2345 Mb/s
		Type: 3, Heap: 1, Flags: HostVisible | HostCoherent | HostCached,  Speed: 3989 Mb/s
		Type: 4, Heap: 2, Flags: DeviceLocal | HostVisible | HostCoherent, Speed: 1449 Mb/s
		
	AMD RX 570 (PCIe 2 8x)
		Type: 1, Heap: 1, Flags: HostVisible | HostCoherent,               Speed: 4537 Mb/s
		Type: 2, Heap: 2, Flags: DeviceLocal | HostVisible | HostCoherent, Speed: 1392 Mb/s
		Type: 3, Heap: 1, Flags: HostVisible | HostCoherent | HostCached,  Speed: 3659 Mb/s
		Type: 2, Heap: 2, Flags: DeviceLocal | HostVisible | HostCoherent, Speed: 1391 Mb/s
*/

#include "framework/Vulkan/VulkanDevice.h"
#include "framework/Vulkan/VulkanSwapchain.h"
#include "framework/Window/WindowGLFW.h"
#include "framework/Window/WindowSDL2.h"
#include "compiler/SpvCompiler.h"
#include "stl/Math/Color.h"
#include "stl/Algorithms/StringUtils.h"

namespace {

class UploadTestApp final : public IWindowEventListener, public VulkanDeviceFn
{
private:
	VulkanDeviceInitializer	vulkan;
	WindowPtr				window;


public:
	UploadTestApp ()
	{
		VulkanDeviceFn_Init( vulkan );
	}
	
	void OnKey (StringView, EKeyAction) override {}
	void OnResize (const uint2 &) override {}
	
	void OnRefresh () override {}
	void OnDestroy () override {}
	void OnUpdate () override {}
	void OnMouseMove (const float2 &) override {}

	bool Initialize ();
	void Destroy ();
	bool Run ();

};

/*
=================================================
	Initialize
=================================================
*/
bool UploadTestApp::Initialize ()
{
# if defined(FG_ENABLE_GLFW)
	window.reset( new WindowGLFW() );

# elif defined(FG_ENABLE_SDL2)
	window.reset( new WindowSDL2() );

# else
#	error unknown window library!
# endif
	

	// create window and vulkan device
	{
		const char	title[] = "Upload test";

		CHECK_ERR( window->Create( { 800, 600 }, title ));
		window->AddListener( this );
		
		CHECK_ERR( vulkan.CreateInstance( window->GetVulkanSurface(), title, "Engine", vulkan.GetRecomendedInstanceLayers(), {}, {1,0} ));
		CHECK_ERR( vulkan.ChooseHighPerformanceDevice() );
		CHECK_ERR( vulkan.CreateLogicalDevice( Default, Default ));
		
		vulkan.CreateDebugCallback( DefaultDebugMessageSeverity );
	}
	return true;
}

/*
=================================================
	Destroy
=================================================
*/
void UploadTestApp::Destroy ()
{
	VK_CALL( vkDeviceWaitIdle( vulkan.GetVkDevice() ));
	
	vulkan.DestroyLogicalDevice();
	vulkan.DestroyInstance();

	window->Destroy();
	window.reset();
}

/*
=================================================
	Run
=================================================
*/
bool UploadTestApp::Run ()
{
	using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
	using Seconds_t		= std::chrono::duration< double >;

	const VkMemoryPropertyFlags	required_mem_flags[] = {
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	const auto	MemFlagsToString = [] (VkMemoryPropertyFlags flags)
	{
		String	result;
		for (uint i = 1; i <= uint(flags); i <<= 1)
		{
			if ( not AllBits( flags, i ))
				continue;

			switch ( VkMemoryPropertyFlagBits(i) )
			{
				case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT :	result << "DeviceLocal | ";		break;
				case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT :	result << "HostVisible | ";		break;
				case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :	result << "HostCoherent | ";	break;
				case VK_MEMORY_PROPERTY_HOST_CACHED_BIT :	result << "HostCached | ";		break;
			}
		}
		if ( result.size() )
			result.erase( result.end() -3, result.end() );
		return result;
	};

	const uint		num_iter	= 1;
	const BytesU	buf_size	= 220_Mb;
	const auto&		mem_props	= vulkan.GetProperties().memoryProperties;
	Array<uint64_t>	buf_data;

	buf_data.resize( size_t(buf_size) / sizeof(buf_data[0]) );

	for (size_t i = 0; i < buf_data.size(); ++i) {
		buf_data[i] = uint(i);
	}


	for (auto mem_flags : required_mem_flags)
	for (uint c = 0; c < 32; ++c)
	{
		// create staging buffer
		VkBuffer		staging_buffer	= VK_NULL_HANDLE;
		VkDeviceMemory	staging_memory	= VK_NULL_HANDLE;
		uint *			mapped			= null;
		{
			VkBufferCreateInfo	info = {};
			info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			info.flags			= 0;
			info.size			= VkDeviceSize(buf_size);
			info.usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

			VK_CHECK( vkCreateBuffer( vulkan.GetVkDevice(), &info, null, OUT &staging_buffer ));
		
			VkMemoryRequirements	mem_req;
			vkGetBufferMemoryRequirements( vulkan.GetVkDevice(), staging_buffer, OUT &mem_req );
	
			VkMemoryAllocateInfo	alloc = {};
			alloc.sType				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc.allocationSize	= mem_req.size;
			alloc.memoryTypeIndex	= UMax;

			uint	heap_index = UMax;

			for (uint i = c; i < mem_props.memoryTypeCount; ++i)
			{
				const auto&		mem_type = mem_props.memoryTypes[i];

				if ( AllBits( mem_req.memoryTypeBits, 1u << i ) and AllBits( mem_type.propertyFlags, mem_flags ))
				{
					c						= i;
					alloc.memoryTypeIndex	= i;
					mem_flags				= mem_type.propertyFlags;
					heap_index				= mem_type.heapIndex;
					break;
				}
			}

			if ( alloc.memoryTypeIndex == UMax )
			{
				vkDestroyBuffer( vulkan.GetVkDevice(), staging_buffer, null );
				break;
			}

			VK_CHECK( vkAllocateMemory( vulkan.GetVkDevice(), &alloc, null, OUT &staging_memory ));
		
			VK_CALL( vkBindBufferMemory( vulkan.GetVkDevice(), staging_buffer, staging_memory, 0 ));

			VK_CALL( vkMapMemory( vulkan.GetVkDevice(), staging_memory, 0, info.size, 0, OUT BitCast<void **>(&mapped) ));

			// run tests
			const TimePoint_t	start = TimePoint_t::clock::now();
			std::atomic_signal_fence( std::memory_order_release );

			for (uint j = 0; j < num_iter; ++j)
			{
				std::memcpy( mapped, buf_data.data(), size_t(buf_size) );
			}
			
			std::atomic_signal_fence( std::memory_order_acquire );
			const TimePoint_t	end = TimePoint_t::clock::now();

			String	str;
			str << "Type: " << ToString( alloc.memoryTypeIndex )
				<< ", Heap: " << ToString( heap_index )
				<< ", Flags: " << MemFlagsToString( mem_flags )
				<< ", Speed: " << ToString( double(size_t(buf_size / 1_Mb) * num_iter) / std::chrono::duration_cast<Seconds_t>( end - start ).count() ) << " Mb/s";

			FG_LOGI( str );

			VK_CHECK( vkDeviceWaitIdle( vulkan.GetVkDevice() ));

			vkUnmapMemory( vulkan.GetVkDevice(), staging_memory );
			vkDestroyBuffer( vulkan.GetVkDevice(), staging_buffer, null );
			vkFreeMemory( vulkan.GetVkDevice(), staging_memory, null );
		}
	}
	return true;
}

}	// anonymous namespace

/*
=================================================
	UploadSpeed_Sample1
=================================================
*/
extern void UploadSpeed_Sample1 ()
{
	UploadTestApp	app;
	
	if ( app.Initialize() )
	{
		app.Run();
		app.Destroy();
	}
}
