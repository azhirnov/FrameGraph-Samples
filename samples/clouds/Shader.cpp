#include "Shader.h"

namespace FG
{
	// Need to move this
	static std::string readFile(const std::string& filename, const std::string& defines = "")
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::string buffer;
		buffer.resize(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return	"#version 450\n"
				"#extension GL_ARB_separate_shader_objects : enable\n\n" +
				defines +
				buffer;
	}

	/// Mesh Shader
	
	void MeshShader::cleanup()
	{
		_fg->ReleaseResource( _pipeline );
	}

	void MeshShader::cleanupUniforms()
	{
		_fg->ReleaseResource( _uniformCameraBuffer );
		_fg->ReleaseResource( _uniformModelBuffer );
		_fg->ReleaseResource( _uniformSunBuffer );
		_fg->ReleaseResource( _uniformSkyBuffer );
	}

	void MeshShader::createDescriptorSet()
	{
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"0"}, OUT _descriptorSet ));

		_descriptorSet.BindBuffer( UniformID{"UniformCameraObject"}, _uniformCameraBuffer );
		_descriptorSet.BindBuffer( UniformID{"UniformModelObject"}, _uniformModelBuffer );
		_descriptorSet.BindBuffer( UniformID{"UniformSunObject"}, _uniformSunBuffer );
		_descriptorSet.BindBuffer( UniformID{"UniformSkyObject"}, _uniformSkyBuffer );
		_descriptorSet.BindTexture( UniformID{"texColor"}, _textures[ALBEDO]->Image(), _textures[ALBEDO]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"pbrInfo"}, _textures[ROUGH_METAL_AO_HEIGHT]->Image(), _textures[ROUGH_METAL_AO_HEIGHT]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"normalMap"}, _textures[NORMAL]->Image(), _textures[NORMAL]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"cloudPlacement"}, _textures[3]->Image(), _textures[3]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"lowResCloudShape"}, _textures3D[0]->Image(), _textures3D[0]->Sampler() );
	}

	void MeshShader::createPipeline()
	{
		auto vertShaderCode = readFile(_shaderFilePaths[0]);
		auto fragShaderCode = readFile(_shaderFilePaths[1]);

		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", std::move(vertShaderCode) );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", std::move(fragShaderCode) );

		_pipeline = _fg->CreatePipeline( desc, _shaderFilePaths[1] );
		CHECK( _pipeline );
	}

	void MeshShader::createUniformBuffer()
	{
		_uniformCameraBuffer = _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformCameraObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformCameraObject" );
		_uniformModelBuffer	 = _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformModelObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformModelObject" );
		_uniformSunBuffer	 = _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSunObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSunObject" );
		_uniformSkyBuffer	 = _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSkyObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSkyObject" );
	}
	
	void MeshShader::updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject& cam, UniformModelObject& model, UniformSunObject& sun, UniformSkyObject& sky)
	{
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformCameraBuffer ).AddData( &cam, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformModelBuffer ).AddData( &model, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSunBuffer ).AddData( &sun, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSkyBuffer ).AddData( &sky, 1 ));
	}

	/// Background Shader
	
	void BackgroundShader::cleanup()
	{
		_fg->ReleaseResource( _pipeline );
	}

	void BackgroundShader::cleanupUniforms()
	{
		// currently no uniforms, only a texture
	}

	void BackgroundShader::createDescriptorSet()
	{
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"0"}, OUT _descriptorSet ));
	}

	void BackgroundShader::createPipeline()
	{
		auto vertShaderCode = readFile(_shaderFilePaths[0]);
		auto fragShaderCode = readFile(_shaderFilePaths[1]);
		
		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", std::move(vertShaderCode) );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", std::move(fragShaderCode) );

		_pipeline = _fg->CreatePipeline( desc, _shaderFilePaths[1] );
		CHECK( _pipeline );
	}

	void BackgroundShader::createUniformBuffer()
	{
		// currently no uniforms
	}

	/// Compute Shader
	
	void ComputeShader::cleanup()
	{
		_fg->ReleaseResource( _pipeline );
	}

	void ComputeShader::cleanupUniforms()
	{
		_fg->ReleaseResource( _uniformCameraBuffer );
		_fg->ReleaseResource( _uniformCameraBufferPrev );
		_fg->ReleaseResource( _uniformSunBuffer );
		_fg->ReleaseResource( _uniformSkyBuffer );
	}

	void ComputeShader::createStorageDescriptorSets()
	{
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"0"}, OUT _storageBufferSetA ));
	}

	void ComputeShader::createDescriptorSet()
	{
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"1"}, OUT _descriptorSet ));

		_descriptorSet.BindBuffer( UniformID{"UniformCameraObject"}, _uniformCameraBuffer );
		_descriptorSet.BindBuffer( UniformID{"UniformCameraObjectPrev"}, _uniformCameraBufferPrev );
		_descriptorSet.BindBuffer( UniformID{"UniformSunObject"}, _uniformSunBuffer );
		_descriptorSet.BindBuffer( UniformID{"UniformSkyObject"}, _uniformSkyBuffer );
		_descriptorSet.BindTexture( UniformID{"cloudPlacement"}, _textures[2]->Image(), _textures[2]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"nightSkyMap"}, _textures[3]->Image(), _textures[3]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"curlNoise"}, _textures[4]->Image(), _textures[4]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"lowResCloudShape"}, _textures3D[0]->Image(), _textures3D[0]->Sampler() );
		_descriptorSet.BindTexture( UniformID{"hiResCloudShape"}, _textures3D[1]->Image(), _textures3D[1]->Sampler() );
	}

	void ComputeShader::createPipeline()
	{
		auto computeShaderCode = readFile(_shaderFilePaths[0], _reprojection ? "#define REPROJECTION\n" : "");

		ComputePipelineDesc	desc;
		desc.AddShader( EShaderLangFormat::VKSL_110 | EShaderLangFormat::EnableDebugTrace, "main", std::move(computeShaderCode) );

		_pipeline = _fg->CreatePipeline( desc, _shaderFilePaths[0] );
		CHECK( _pipeline );
	}

	void ComputeShader::createUniformBuffer()
	{
		_uniformCameraBuffer	= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformCameraObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformCameraObject" );
		_uniformCameraBufferPrev= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformCameraObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformCameraObjectPrev" );
		_uniformSunBuffer		= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSunObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSunObject" );
		_uniformSkyBuffer		= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSkyObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSkyObject" );
	}

	void ComputeShader::updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject &cam, UniformCameraObject &camPrev, UniformSkyObject &sky, UniformSunObject &sun)
	{
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformCameraBuffer ).AddData( &cam, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformCameraBufferPrev ).AddData( &camPrev, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSunBuffer ).AddData( &sun, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSkyBuffer ).AddData( &sky, 1 ));
	}

	/// Post Process Shader
	
	void PostProcessShader::cleanup()
	{
		_fg->ReleaseResource( _pipeline );
	}

	void PostProcessShader::cleanupUniforms()
	{
		_fg->ReleaseResource( _uniformCameraBuffer );
		_fg->ReleaseResource( _uniformSunBuffer );
	}

	void PostProcessShader::createDescriptorSet()
	{
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"0"}, OUT _descriptorSet ));
		
		_descriptorSet.BindBuffer( UniformID{"UniformCameraObject"}, _uniformCameraBuffer );
		_descriptorSet.BindBuffer( UniformID{"UniformSunObject"}, _uniformSunBuffer );
	}

	void PostProcessShader::createPipeline()
	{
		auto vertShaderCode = readFile(_shaderFilePaths[0]);
		auto fragShaderCode = readFile(_shaderFilePaths[1]);
		
		GraphicsPipelineDesc	desc;
		desc.AddShader( EShader::Vertex,   EShaderLangFormat::VKSL_110, "main", std::move(vertShaderCode) );
		desc.AddShader( EShader::Fragment, EShaderLangFormat::VKSL_110, "main", std::move(fragShaderCode) );

		_pipeline = _fg->CreatePipeline( desc, _shaderFilePaths[1] );
		CHECK( _pipeline );
	}

	void PostProcessShader::createUniformBuffer()
	{
		_uniformCameraBuffer = _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformCameraObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformCameraObject" );
		_uniformSunBuffer	 = _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSunObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSunObject" );
	}

	void PostProcessShader::updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject &cam, UniformSunObject &sun)
	{
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformCameraBuffer ).AddData( &cam, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSunBuffer ).AddData( &sun, 1 ));
	}


	/// Reproject shader

	void ReprojectShader::cleanup()
	{
		_fg->ReleaseResource( _pipeline );
	}

	void ReprojectShader::cleanupUniforms()
	{
		_fg->ReleaseResource( _uniformSkyBuffer );
		_fg->ReleaseResource( _uniformSunBuffer );
		_fg->ReleaseResource( _uniformCameraBuffer );
		_fg->ReleaseResource( _uniformCameraBufferPrev );
	}

	void ReprojectShader::updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject& cam, UniformCameraObject& camPrev, UniformSkyObject& sky, UniformSunObject& sun)
	{
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformCameraBuffer ).AddData( &cam, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformCameraBufferPrev ).AddData( &camPrev, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSkyBuffer ).AddData( &sky, 1 ));
		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _uniformSunBuffer ).AddData( &sun, 1 ));
	}

	void ReprojectShader::createDescriptorSet()
	{
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"0"}, OUT _descriptorSet ));
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"1"}, OUT _descriptorSetB ));
		CHECK( _fg->InitPipelineResources( _pipeline, DescriptorSetID{"2"}, OUT _uniformSet ));
		
		_uniformSet.BindBuffer( UniformID{"UniformCameraObject"}, _uniformCameraBuffer );
		_uniformSet.BindBuffer( UniformID{"UniformCameraObjectPrev"}, _uniformCameraBufferPrev );
		_uniformSet.BindBuffer( UniformID{"UniformSunObject"}, _uniformSunBuffer );
		_uniformSet.BindBuffer( UniformID{"UniformSkyObject"}, _uniformSkyBuffer );
	}


	void ReprojectShader::createUniformBuffer()
	{
		_uniformSkyBuffer		= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSkyObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSkyObject" );
		_uniformSunBuffer		= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformSunObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformSunObject" );
		_uniformCameraBuffer	= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformCameraObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformCameraObject" );
		_uniformCameraBufferPrev= _fg->CreateBuffer( BufferDesc{}.Size( sizeof(UniformCameraObject) ).Usage( EBufferUsage::Uniform | EBufferUsage::Transfer ), Default, "UniformCameraObject" );
	}

	void ReprojectShader::createPipeline()
	{
		auto computeShaderCode = readFile(_shaderFilePaths[0]);
		
		ComputePipelineDesc	desc;
		desc.AddShader( EShaderLangFormat::VKSL_110, "main", std::move(computeShaderCode) );

		_pipeline = _fg->CreatePipeline( desc, _shaderFilePaths[0] );
		CHECK( _pipeline );
	}

}   // FG
