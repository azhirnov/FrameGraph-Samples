#pragma once

#include "framegraph/FG.h"

namespace FG
{

	class Texture
	{
	private:
		FrameGraph		_fg;
		ImageID			_image;
		RawSamplerID	_sampler;
		EPixelFormat	_format	= Default;
		uint2			_dim;
		bool			_initialized = false;

		void createSampler();

	public:
		Texture(FrameGraph fg, EPixelFormat format = EPixelFormat::RGBA8_UNorm, RawSamplerID samp = Default);
		~Texture();
		
		void cleanup ();

		bool initFromFile(std::string path);
		bool initForStorage(uint2 extent);
		bool initForColorAttachment(uint2 extent);
		bool initForDepthAttachment(uint2 extent);

		ND_ RawImageID		Image ()	const	{ return _image; }
		ND_ RawSamplerID	Sampler ()	const	{ return _sampler; }
	};

	/// Texture 3D

	class Texture3D
	{
	private:
		FrameGraph		_fg;
		ImageID			_image;
		RawSamplerID	_sampler;
		EPixelFormat	_format	= Default;
		uint3			_dim;
		bool			_initialized = false;
		
		void createSampler();

	public:
		Texture3D(FrameGraph fg, uint3 dim, EPixelFormat format = EPixelFormat::RGBA8_UNorm, RawSamplerID samp = Default);
		~Texture3D();
		
		void cleanup ();

		// This function should supply the "base" name of each texture slice file.
		bool initFromFile(std::string path);
		bool initForStorage(uint3 extent);

		ND_ RawImageID		Image ()	const	{ return _image; }
		ND_ RawSamplerID	Sampler ()	const	{ return _sampler; }
	};

}   // FG
