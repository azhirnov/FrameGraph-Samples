#pragma once

#include "Texture.h"
#include "Geometry.h"
#include "SkyManager.h"
#include <fstream>

namespace FG
{

	struct UniformCameraObject {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 cameraPosition;
		glm::vec4 cameraParams;
	};

	struct UniformModelObject {
		glm::mat4 model;
		glm::mat4 invTranspose;
	};

	struct UniformStorageImageObject {
	};

	class Shader
	{
	protected:
		// All shaders have layouts and pipelines to delete.
		virtual void cleanup() = 0;

		// Different shaders will have different numbers of uniform buffers, need to implement cleanup for each.
		virtual void cleanupUniforms() = 0;
		
		virtual void createDescriptorSet() = 0;
		virtual void createUniformBuffer() = 0;
		virtual void createPipeline() = 0;

		std::vector<std::string> _shaderFilePaths;

		PipelineResources		_descriptorSet;

		// Textures to be used for samplers
		std::vector<Texture*>   _textures;
		std::vector<Texture3D*> _textures3D;

		FrameGraph              _fg;

	public:
		// all shaders need a setup function, but will have variable # arguments...
		// for now: make part of constructor

		Shader(FrameGraph fg) : _fg{fg} {}

		virtual ~Shader() {}
	
		// Samplers must be initialized before pipeline / descriptor creation.
		void addTexture(Texture* tex) { _textures.push_back(tex); }
		void addTexture3D(Texture3D* tex) { _textures3D.push_back(tex); }
	};
	
	// TODO: only albedo for the moment,
	// 1. Add cook torrance code for ROUGH_METAL_AO
	// 2. Add normal mapping support
	// 3. Emissive?
	// 4. Find ways to pack alpha channels?
	enum PBRTEXTURES
	{
		ALBEDO = 0, ROUGH_METAL_AO_HEIGHT, NORMAL
	};

	/*
	* A shader pipeline to rasterize a mesh.
	*/
	class MeshShader : public Shader
	{
	private:
	
	protected:
		GPipelineID			_pipeline;

		UniformCameraObject	_cameraUniforms;
		UniformModelObject	_modelUniforms;
		UniformSunObject	_sunUniforms;
		UniformSkyObject	_skyUniforms;

		BufferID			_uniformCameraBuffer;
		BufferID			_uniformModelBuffer;
		BufferID			_uniformSunBuffer;
		BufferID			_uniformSkyBuffer;
		
		void cleanup() override;
		void createDescriptorSet() override;
		void createUniformBuffer() override;
		void createPipeline() override;
		void cleanupUniforms() override;

	public:
		void setupShader(std::string vertPath, std::string fragPath) {
			_shaderFilePaths.push_back(vertPath);
			_shaderFilePaths.push_back(fragPath);

			createPipeline();
			createUniformBuffer();
			createDescriptorSet();
		}
	
		MeshShader(FrameGraph fg) : Shader(fg) {}

		MeshShader(FrameGraph fg, std::string vertPath, std::string fragPath, Texture* tex, Texture* pbrTex, Texture* normalTex, Texture* coverageTex, Texture3D* loResCloudShape) :
			Shader(fg) {
			addTexture(tex);
			addTexture(pbrTex);
			addTexture(normalTex);
			addTexture(coverageTex);
			addTexture3D(loResCloudShape);
			setupShader(vertPath, fragPath);
		}

		virtual ~MeshShader() {
			cleanupUniforms();
			cleanup();
		}

		void updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject& cam, UniformModelObject& model, UniformSunObject& sun, UniformSkyObject& sky);

		template <typename T>
		void bindShader(T &task)
		{
			task.SetPipeline( _pipeline );
			task.AddResources( DescriptorSetID{"0"}, &_descriptorSet );
			task.SetCullMode( ECullMode::Back );
			task.SetFrontFaceCCW( false );
			task.AddColorBuffer( RenderTargetID::Color_0,
								 EBlendFactor::SrcAlpha, EBlendFactor::One,
								 EBlendFactor::OneMinusSrcAlpha, EBlendFactor::Zero,
								 EBlendOp::Add, EBlendOp::Add );
			task.SetDepthTestEnabled( true );
			task.SetDepthWriteEnabled( true );
			task.SetDepthCompareOp( ECompareOp::Less );
		}
	};
	
	/*
	  Pipeline for drawing a background quad
	*/

	class BackgroundShader : public Shader
	{
	private:

	protected:
		void cleanup() override;
		void createDescriptorSet() override;
		void createUniformBuffer() override;
		void createPipeline() override;
		void cleanupUniforms() override;
		
		GPipelineID			_pipeline;

		bool swappedBuffers = false;

	public:
		void setupShader(std::string vertPath, std::string fragPath) {
			_shaderFilePaths.push_back(vertPath);
			_shaderFilePaths.push_back(fragPath);

			createPipeline();
			createUniformBuffer();
			createDescriptorSet();
		}

		BackgroundShader(FrameGraph fg) : Shader(fg) {}
		
		BackgroundShader(FrameGraph fg, std::string vertPath, std::string fragPath, Texture* texA, Texture* texB) : Shader(fg)
		{
			addTexture(texA);
			addTexture(texB);
			setupShader(vertPath, fragPath);
			swappedBuffers = false;
		}

		virtual ~BackgroundShader() {
			cleanupUniforms();
			cleanup();
		}
		
		template <typename T>
		void bindShader(T &task)
		{
			if ( swappedBuffers )
				_descriptorSet.BindTexture( UniformID{"texColor"}, _textures[1]->Image(), _textures[1]->Sampler() );
			else
				_descriptorSet.BindTexture( UniformID{"texColor"}, _textures[0]->Image(), _textures[0]->Sampler() );

			task.SetPipeline( _pipeline );
			task.AddResources( DescriptorSetID{"0"}, &_descriptorSet );
			task.SetCullMode( ECullMode::Back );
			task.SetFrontFaceCCW( false );
			task.SetDepthTestEnabled( true );
			task.SetDepthWriteEnabled( true );
			task.SetDepthCompareOp( ECompareOp::Less );
		}
	};

	/*
	  Pipeline for computing clouds
	*/

	class ComputeShader : public Shader
	{
	private:

	protected:
		void cleanup() override;
		void createDescriptorSet() override;
		void createUniformBuffer() override;
		void createPipeline() override;
		void cleanupUniforms() override;
		void createStorageDescriptorSets();
		
		CPipelineID			_pipeline;

		UniformStorageImageObject	_storageImageUniform;
		UniformStorageImageObject	_storageImageUniformPrev;
		UniformCameraObject			_cameraUniforms;

		BufferID			_uniformCameraBuffer;
		BufferID			_uniformCameraBufferPrev;
		BufferID			_uniformSunBuffer;
		BufferID			_uniformSkyBuffer;

		// need sets to ping-pong image buffers
		PipelineResources	_storageBufferSetA;

		bool	swappedBuffers = false;
		bool	_reprojection = false;

	public:
		void setupShader(std::string path) {
			_shaderFilePaths.push_back(path);

			createPipeline();
			createUniformBuffer();
			createDescriptorSet();
			createStorageDescriptorSets();
		}

		ComputeShader(FrameGraph fg) : Shader(fg) {}

		ComputeShader(FrameGraph fg, bool reprojection, std::string path, Texture* storageTex, Texture* storageTexPrev, Texture* placementTex,
					  Texture* nightSkyTex, Texture* curlTexture, Texture3D* lowResCloudShapeTex, Texture3D* hiResCloudShapeTex) :
			Shader(fg), _reprojection{reprojection}
		{
			// Note: This texture is intended to be written to. In this application, it is set to be the sampled texture of a separate BackgroundShader.
			addTexture(storageTex);
			addTexture(storageTexPrev);
			addTexture(placementTex);
			addTexture(nightSkyTex);
			addTexture(curlTexture);
			addTexture3D(lowResCloudShapeTex);
			addTexture3D(hiResCloudShapeTex);
			setupShader(path);
			swappedBuffers = false;
		}

		virtual ~ComputeShader() {
			cleanupUniforms();
			cleanup();
		}

		void updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject& cam, UniformCameraObject& camPrev, UniformSkyObject& sky, UniformSunObject& sun);
		
		template <typename T>
		void bindShader(T &task)
		{
			if ( swappedBuffers )
				_storageBufferSetA.BindImage( UniformID{"resultImage"}, _textures[1]->Image() );
			else
				_storageBufferSetA.BindImage( UniformID{"resultImage"}, _textures[0]->Image() );
			
			task.SetPipeline( _pipeline );
			task.AddResources( DescriptorSetID{"0"}, &_storageBufferSetA );
			task.AddResources( DescriptorSetID{"1"}, &_descriptorSet );

	        swappedBuffers = !swappedBuffers;
		}
	};

	// Another compute shader for transferring pixels from one background to another
	class ReprojectShader : public Shader
	{
	private:

	protected:
		void cleanup() override;
		void createDescriptorSet() override;
		void createUniformBuffer() override;
		void createPipeline() override;
		void cleanupUniforms() override;
		
		CPipelineID			_pipeline;

		PipelineResources	_descriptorSetB; // draws to a different texture every other frame
		bool swappedBuffers = false;

		PipelineResources	_uniformSet;

		BufferID	_uniformCameraBuffer;
		BufferID	_uniformCameraBufferPrev;
		BufferID	_uniformSkyBuffer;
		BufferID	_uniformSunBuffer;

	public:
		void setupShader(std::string path) {
			_shaderFilePaths.push_back(path);

			createPipeline();
			createUniformBuffer();
			createDescriptorSet();
		}

		ReprojectShader(FrameGraph fg) : Shader(fg) {}

		ReprojectShader(FrameGraph fg, std::string shaderPath, Texture* texA, Texture* texB) : Shader(fg)
		{
			addTexture(texA);
			addTexture(texB);
			setupShader(shaderPath);
			swappedBuffers = false;
		}

		virtual ~ReprojectShader() {
			cleanupUniforms();
			cleanup();
		}

		void updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject& cam, UniformCameraObject& camPrev, UniformSkyObject& sky, UniformSunObject& sun);
		
		template <typename T>
		void bindShader(T &task)
		{
			if ( swappedBuffers )
			{
				_descriptorSet.BindImage( UniformID{"targetImage"}, _textures[1]->Image() );
				_descriptorSetB.BindImage( UniformID{"sourceImage"}, _textures[0]->Image() );
			}
			else
			{
				_descriptorSet.BindImage( UniformID{"targetImage"}, _textures[0]->Image() );
				_descriptorSetB.BindImage( UniformID{"sourceImage"}, _textures[1]->Image() );
			}
			
			task.SetPipeline( _pipeline );
			task.AddResources( DescriptorSetID{"0"}, &_descriptorSet );
			task.AddResources( DescriptorSetID{"1"}, &_descriptorSetB );
			task.AddResources( DescriptorSetID{"2"}, &_uniformSet );

	        swappedBuffers = !swappedBuffers;
		}
	};

	/*
	  Pipeline for post processing effects
	*/

	// This class should be extended by classes for each individual post processing effect, because each will have different uniform setups.
	// at the moment, this is identical to the background shader class
	class PostProcessShader : public Shader
	{
	private:

	protected:
		void cleanup() override;
		void createDescriptorSet() override;
		void createUniformBuffer() override;
		void createPipeline() override;
		void cleanupUniforms() override;
		
		GPipelineID			_pipeline;

		// Uniform buffers and buffer memory eventually
		// ex: gaussian blur parameters, high pass parameters, sun position for radial blur, god rays, etc
		// TODO: make this class's function virtual and override them in the subclass
		// need a GodRayShader class that has these uniforms:
	
		// God ray shader uniforms
		BufferID	_uniformSunBuffer;
		BufferID	_uniformCameraBuffer;

	public:
		void setupShader(std::string vertPath, std::string fragPath) {
			_shaderFilePaths.push_back(vertPath);
			_shaderFilePaths.push_back(fragPath);

			createPipeline();
			createUniformBuffer();
			createDescriptorSet();
		}

		//TODO: change this constructor to take an image descriptor instead of a texture, or somehow create a texture from the framebuffer image descriptor
		PostProcessShader(FrameGraph fg) : Shader(fg) {}

		PostProcessShader(FrameGraph fg, std::string vertPath, std::string fragPath, Texture* texA) : Shader(fg)
		{
			addTexture(texA);
			setupShader(vertPath, fragPath);
		}

		virtual ~PostProcessShader() {
			cleanupUniforms();
			cleanup();
		}

		void updateUniformBuffers(const CommandBuffer &cmdbuf, UniformCameraObject& cam, UniformSunObject& sun);
		
		template <typename T>
		void bindShader(T &task)
		{
			_descriptorSet.BindTexture( UniformID{"texColor"}, _textures[0]->Image(), _textures[0]->Sampler() );
			task.SetPipeline( _pipeline );
			task.AddResources( DescriptorSetID{"0"}, &_descriptorSet );
			task.SetCullMode( ECullMode::Back );
			task.SetFrontFaceCCW( false );
			task.SetDepthTestEnabled( true );
			task.SetDepthWriteEnabled( true );
			task.SetDepthCompareOp( ECompareOp::Less );
		}
	};

}   // FG
