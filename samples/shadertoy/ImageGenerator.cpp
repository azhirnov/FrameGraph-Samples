// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ImageGenerator.h"
#include "stl/Algorithms/StringUtils.h"
#include "scene/Saver/DDS/DDSSaver.h"
#include "scene/Loader/DDS/DDSLoader.h"
#include "scene/Loader/Intermediate/IntermImage.h"

#ifdef FG_STD_FILESYSTEM
#	include <filesystem>
	namespace FS = std::filesystem;
#endif

namespace FG
{

/*
=================================================
	constructor
=================================================
*/
	ImageGenerator::ImageGenerator (const Config &cfg) : _config{cfg}
	{}
	
/*
=================================================
	destructor
=================================================
*/
	ImageGenerator::~ImageGenerator ()
	{}

/*
=================================================
	Initialize
=================================================
*/
	bool  ImageGenerator::Initialize (Shader_t shader, StringView imageName)
	{
		{
			AppConfig	cfg;
			cfg.surfaceSize			= uint2(1024, 768);
			cfg.windowTitle			= "Image generator";
			cfg.shaderDirectories	= { FG_DATA_PATH "../shaderlib", FG_DATA_PATH };
			//cfg.enableDebugLayers	= false;
			CHECK_ERR( _CreateFrameGraph( cfg ));
		}

		_view.reset( new ShaderView{_frameGraph} );

		GetFPSCamera().SetPosition({ 0.0f, 0.0f, 0.0f });

		_view->SetMode( uint2(_config.imageSize), _config.viewMode );
		_view->SetCamera( GetFPSCamera() );
		_view->SetFov( _cameraFov );
		_view->SetImageFormat( _config.imageFormat, _config.imageSamples );

		shader( _view.get() );

		_imageName = imageName;

		{
			ImageDesc	desc;
			desc.imageType	= _config.imageSize.z > 1 ? EImage::Tex3D : EImage::Tex2D;
			desc.dimension	= _config.imageSize;
			desc.format		= _config.imageFormat;
			desc.usage		= EImageUsage::Transfer | EImageUsage::Sampled;

			_image = _frameGraph->CreateImage( desc, Default, "ResultImage" );
			CHECK_ERR( _image );
		}
		return true;
	}

/*
=================================================
	OnKey
=================================================
*/
	void  ImageGenerator::OnKey (StringView key, EKeyAction action)
	{
		if ( action == EKeyAction::Down )
		{
			if ( key == "escape" and GetWindow() )	GetWindow()->Quit();
		}
	}
		
/*
=================================================
	DrawScene
=================================================
*/
	bool  ImageGenerator::DrawScene ()
	{
		CHECK_ERR( _view );

		// finish recording and exit
		if ( _frameCounter >= _config.imageSize.z )
		{
			if ( GetWindow() )
				GetWindow()->Quit();

			CHECK( _SaveImage() );
			return true;
		}

		CommandBuffer	cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		_UpdateCamera();
		_view->SetCamera( GetFPSCamera() );

		// draw
		const SecondsF	time	{ float(_frameCounter) * 0.01f };
		const SecondsF	dt		{ 0.01f };

		auto[task, image_l, image_r] = _view->Draw( cmdbuf, _frameCounter, time, dt );
		CHECK_ERR( task and image_l );

		// present
		{
			if ( _config.imageSamples > 1 )
			{
				task = cmdbuf->AddTask( ResolveImage{}.From( image_l ).To( _image )
											.AddRegion( {}, int3{0}, {}, int3{0, 0, int(_frameCounter)}, uint3{_config.imageSize.x, _config.imageSize.y, 1} )
											.DependsOn( task ));
			}
			else
			{
				task = cmdbuf->AddTask( CopyImage{}.From( image_l ).To( _image )
											.AddRegion( {}, int3{0}, {}, int3{0, 0, int(_frameCounter)}, uint3{_config.imageSize.x, _config.imageSize.y, 1} )
											.DependsOn( task ));
			}

			cmdbuf->AddTask( Present{ GetSwapchain(), image_l }.DependsOn( task ));

			CHECK_ERR( _frameGraph->Execute( cmdbuf ));
			CHECK_ERR( _frameGraph->Flush() );
		}

		++_frameCounter;

		_SetLastCommandBuffer( cmdbuf );
		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  ImageGenerator::Destroy ()
	{
		_view.reset();

		_frameGraph->ReleaseResource( _image );

		_DestroyFrameGraph();
	}
	
/*
=================================================
	_SaveImage
=================================================
*/
	bool  ImageGenerator::_SaveImage ()
	{
		CommandBuffer	cmdbuf = _frameGraph->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		auto	OnLoaded = [this] (const ImageView &view)
		{
			IntermImage::Mipmaps_t	mips;
			mips.resize(1);
			mips[0].resize(1);

			auto&			level	= mips[0][0];
			ImageDesc		desc	= _frameGraph->GetDescription( _image );
			IntermImagePtr	interm	= MakeShared<IntermImage>( std::move(mips), desc.imageType, _imageName );

			level.dimension		= desc.dimension;
			level.format		= desc.format;
			level.rowPitch		= view.RowPitch();
			level.slicePitch	= view.SlicePitch();

			for (auto& part : view.Parts())
			{
				level.pixels.insert( level.pixels.end(), part.begin(), part.end() );
			}

			DDSSaver	saver;
			CHECK_ERR( saver.SaveImage( _imageName, interm ), void());

			FG_LOGI( "Image saved to '"s << _imageName << "'" );
			
			IntermImagePtr	tmp	= MakeShared<IntermImage>( _imageName );

			DDSLoader	loader;
			CHECK( loader.LoadImage( tmp, {}, null, false ));
		};

		cmdbuf->AddTask( ReadImage{}.SetImage( _image, int3{0}, _config.imageSize ).SetCallback( std::move(OnLoaded) ));

		CHECK_ERR( _frameGraph->Execute( cmdbuf ));
		CHECK_ERR( _frameGraph->Wait({ cmdbuf }));

		return true;
	}


}	// FG
