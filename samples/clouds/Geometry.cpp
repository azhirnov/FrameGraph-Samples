#include "Geometry.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace FG
{

	void Geometry::cleanup()
	{
		_fg->ReleaseResource( _vertexBuffer );
		_fg->ReleaseResource( _indexBuffer );
	}

	bool Geometry::createVertexBuffer (const CommandBuffer &cmdbuf, std::string name)
	{
		BufferDesc	desc;
		desc.size	= ArraySizeOf(_vertices);
		desc.usage	= EBufferUsage::Transfer | EBufferUsage::Vertex;

		_vertexBuffer = _fg->CreateBuffer( desc, Default, name );
		CHECK_ERR( _vertexBuffer );

		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _vertexBuffer ).AddData( _vertices ));
		return true;
	}

	bool Geometry::createIndexBuffer (const CommandBuffer &cmdbuf, std::string name)
	{
		BufferDesc	desc;
		desc.size	= ArraySizeOf(_indices);
		desc.usage	= EBufferUsage::Transfer | EBufferUsage::Index;

		_indexBuffer = _fg->CreateBuffer( desc, Default, name );
		CHECK_ERR( _indexBuffer );

		cmdbuf->AddTask( UpdateBuffer{}.SetBuffer( _indexBuffer ).AddData( _indices ));
		return true;
	}

	/* Calls commands to ready the buffers for drawing.
	* Call this only after the respective command buffer is recording and UBO are bound
	*/
	DrawIndexed Geometry::enqueueDrawCommands ()
	{
		CHECK_ERR( _initialized );
		
		DrawIndexed	task;
		task.SetTopology( EPrimitive::TriangleList );
		task.AddBuffer( Default, _vertexBuffer );
		task.SetIndexBuffer( _indexBuffer, 0_b, EIndex::UInt );
		task.Draw( uint(_indices.size()) );
		return task;
	}

	bool Geometry::setupAsQuad ()
	{
		if ( _initialized )
			cleanup();

		_vertices = {
			{ { -0.5f, -0.5f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }, {0.0f, 0.0f, -1.0f} },
			{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { 0.5f,  0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { -0.5f,  0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },

			{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { 0.5f,  0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { -0.5f,  0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } }
		};

		_indices = {
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4
		};

		CommandBuffer	cmdbuf = _fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		CHECK_ERR( createVertexBuffer( cmdbuf, "quad" ));
		CHECK_ERR( createIndexBuffer( cmdbuf, "quad" ));

		CHECK_ERR( _fg->Execute( cmdbuf ));
		CHECK_ERR( _fg->Wait({ cmdbuf }));

		_initialized = true;
		return true;
	}

	bool Geometry::setupAsBackgroundQuad ()
	{
		if ( _initialized )
			cleanup();

		_vertices = {
			{ { -1.0f, -1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { 1.0f, -1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { 1.0f,  1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } },
			{ { -1.0f,  1.0f, 0.999f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, -1.0f } }
		};

		_indices = {
			0, 1, 2, 2, 3, 0
		};
		
		CommandBuffer	cmdbuf = _fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		CHECK_ERR( createVertexBuffer( cmdbuf, "BackgroundQuad" ));
		CHECK_ERR( createIndexBuffer( cmdbuf, "BackgroundQuad" ));

		CHECK_ERR( _fg->Execute( cmdbuf ));
		CHECK_ERR( _fg->Wait({ cmdbuf }));

		_initialized = true;
		return true;
	}

	void Geometry::initializeTBN() {
		// for each triangle
		/*
		for (int i = 0; i < indices.size(); i += 3) {
			Vertex a = vertices[indices[i]];
			Vertex b = vertices[indices[i + 1]];
			Vertex c = vertices[indices[i + 2]];

			glm::vec3 dp1 = b.pos - a.pos;
			glm::vec3 dp2 = c.pos - a.pos;

			glm::vec2 duv1 = b.uv - a.uv;
			glm::vec2 duv2 = c.uv - a.uv;

			float r = 1.f / (duv1.x * duv2.y - duv2.x * duv1.y);
			glm::vec3 tangent = r * (dp1 * duv2.y - dp2 * duv1.y);
			glm::vec3 bitangent = r * (dp2 * duv1.x - dp1 * duv2.x);

			// normalized in shader
			vertices[indices[i]].tan += tangent;
			vertices[indices[i + 1]].tan += tangent;
			vertices[indices[i + 2]].tan += tangent;

			vertices[indices[i]].bit += bitangent;
			vertices[indices[i + 1]].bit += bitangent;
			vertices[indices[i + 2]].bit += bitangent;
		}
		*/
	}

	bool Geometry::setupFromMesh(std::string path)
	{
		if ( _initialized )
			cleanup();

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err, warn;

		CHECK_ERR( tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str() ));

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for (const auto& shape : shapes) {

			for (const auto& index : shape.mesh.indices) {
				Vertex vertex = {};
			
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.col = { 1.0f, 1.0f, 1.0f };

				vertex.nor = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				//vertex.tan = { 0.f, 0.f, 0.f }; // going to handle in shader for now
				//vertex.bit = { 0.f, 0.f, 0.f };

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
					_vertices.push_back(vertex);
				}

				_indices.push_back(uniqueVertices[vertex]);
			}
		}
		
		CommandBuffer	cmdbuf = _fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		CHECK_ERR( cmdbuf );

		CHECK_ERR( createVertexBuffer( cmdbuf, path ));
		CHECK_ERR( createIndexBuffer( cmdbuf, path ));

		initializeTBN();
		
		CHECK_ERR( _fg->Execute( cmdbuf ));
		CHECK_ERR( _fg->Wait({ cmdbuf }));

		_initialized = true;
		return true;
	}

}   // FG
