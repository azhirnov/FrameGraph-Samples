#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace FG
{
	
	Texture::Texture(FrameGraph fg, EPixelFormat format, RawSamplerID samp) : _fg{fg}, _sampler{samp}, _format{format}
	{}
	
	Texture::~Texture()
	{
		cleanup();
	}

	void Texture::cleanup()
	{
		_fg->ReleaseResource( _image );
		_initialized = false;
	}

	void Texture::createSampler()
	{
		if ( _sampler )
			return;

		SamplerDesc	info;
		info.SetAddressMode( EAddressMode::Repeat, EAddressMode::Repeat, EAddressMode::Repeat );
		info.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		info.SetAnisotropy( 16.0f );
		info.SetBorderColor( EBorderColor::IntOpaqueBlack );
		info.minLod = 0.0f;
		info.maxLod = 0.0f;
		info.mipLodBias = 0.0f;

		_sampler = _fg->CreateSampler( info ).Release();
	}

	bool Texture::initFromFile(std::string path)
	{
		CHECK_ERR( not _initialized );

		int			width, height, channels;
		stbi_uc*	pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		CHECK_ERR( pixels );

		_dim = uint2{ int2{ width, height }};

		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{ _dim, 1 };
		desc.usage		= EImageUsage::Sampled | EImageUsage::Transfer;
		desc.format		= _format;

		_image = _fg->CreateImage( desc, Default, path );
		CHECK_ERR( _image );
		
		CommandBuffer	cmdbuf = _fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		cmdbuf->AddTask( UpdateImage{}.SetImage( _image ).SetData( pixels, width * height * 4u, uint3{_dim, 1} ));
		
		CHECK_ERR( _fg->Execute( cmdbuf ));
		CHECK_ERR( _fg->Wait({ cmdbuf }));

		createSampler();

		_initialized = true;
		return true;
	}

	bool Texture::initForStorage(uint2 extent)
	{
		CHECK_ERR( not _initialized );

		_dim = extent;
		
		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{ _dim, 1 };
		desc.usage		= EImageUsage::Sampled | EImageUsage::Transfer | EImageUsage::Storage;
		desc.format		= _format;

		_image = _fg->CreateImage( desc );
		CHECK_ERR( _image );

		createSampler();

		_initialized = true;
		return true;
	}
	
	bool Texture::initForColorAttachment(uint2 extent)
	{
		CHECK_ERR( not _initialized );
		
		_dim = extent;

		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{ _dim, 1 };
		desc.usage		= EImageUsage::ColorAttachment | EImageUsage::Sampled;
		desc.format		= _format;

		_image = _fg->CreateImage( desc );
		CHECK_ERR( _image );

		_initialized = true;
		return true;
	}

	bool Texture::initForDepthAttachment(uint2 extent)
	{
		CHECK_ERR( not _initialized );
		
		_dim = extent;
		_format = EPixelFormat::Depth32F;

		ImageDesc	desc;
		desc.imageType	= EImage::Tex2D;
		desc.dimension	= uint3{ _dim, 1 };
		desc.usage		= EImageUsage::DepthStencilAttachment;
		desc.format		= _format;

		_image = _fg->CreateImage( desc );
		CHECK_ERR( _image );

		createSampler(); // probably not necessary

		_initialized = true;
		return true;
	}

	/// Texture3D
	
	Texture3D::Texture3D(FrameGraph fg, uint3 dim, EPixelFormat format, RawSamplerID samp) : _fg{fg}, _sampler{samp}, _format{format}, _dim{dim}
	{}

	Texture3D::~Texture3D()
	{
		cleanup();
	}

	void Texture3D::cleanup()
	{
		_fg->ReleaseResource( _image );
		_initialized = false;
	}


	void Texture3D::createSampler()
	{
		if ( _sampler )
			return;

		SamplerDesc	info;
		info.SetAddressMode( EAddressMode::Repeat, EAddressMode::Repeat, EAddressMode::Repeat );
		info.SetFilter( EFilter::Linear, EFilter::Linear, EMipmapFilter::Linear );
		info.SetBorderColor( EBorderColor::IntOpaqueBlack );
		info.minLod = 0.0f;
		info.maxLod = 0.0f;
		info.mipLodBias = 0.0f;

		_sampler = _fg->CreateSampler( info ).Release();
	}

	bool Texture3D::initFromFile(std::string path)
	{
		CHECK_ERR( not _initialized );
	
		ImageDesc	desc;
		desc.imageType	= EImage::Tex3D;
		desc.dimension	= _dim;
		desc.usage		= EImageUsage::Sampled | EImageUsage::Transfer;
		desc.format		= _format;

		_image = _fg->CreateImage( desc, Default, path );
		CHECK_ERR( _image );

		CommandBuffer	cmdbuf = _fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		for (uint32_t i = 0; i < _dim.z; ++i)
		{
			int			width, height, channels;
			stbi_uc*	pixels = stbi_load((path + "(" + std::to_string(i) + ").tga").c_str(), &width, &height, &channels, STBI_rgb_alpha);

			CHECK_ERR( pixels );

			cmdbuf->AddTask( UpdateImage{}.SetImage( _image, int3{0, 0, int(i)} ).SetData( pixels, size_t(width * height * channels), uint3{int3{width, height, 1}} ));

			stbi_image_free(pixels);
		}

		CHECK_ERR( _fg->Execute( cmdbuf ));
		CHECK_ERR( _fg->Wait({ cmdbuf }));

		createSampler();

		_initialized = true;
		return true;
	}

	bool Texture3D::initForStorage(uint3 extent)
	{
		CHECK_ERR( not _initialized );

		_dim = extent;
		
		ImageDesc	desc;
		desc.imageType	= EImage::Tex3D;
		desc.dimension	= _dim;
		desc.usage		= EImageUsage::Sampled | EImageUsage::Transfer;
		desc.format		= _format;

		_image = _fg->CreateImage( desc );
		CHECK_ERR( _image );

		createSampler();

		_initialized = true;
		return true;
	}

}   // FG
