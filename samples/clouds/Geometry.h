#pragma once

#include "scene/Math/GLM.h"

namespace FG
{

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 col;
		glm::vec2 uv;
		glm::vec3 nor;

		bool operator==(const Vertex& other) const {
			return pos == other.pos && col == other.col && uv == other.uv && nor == other.nor;
		}
	};


	class Geometry
	{
	private:
		BufferID		_vertexBuffer;
		BufferID		_indexBuffer;

		std::vector<Vertex>		_vertices;
		std::vector<uint32_t>	_indices;
		FrameGraph				_fg;

		bool _initialized = false;

		void initializeTBN();
		bool createVertexBuffer(const CommandBuffer &cmdbuf, std::string name);
		bool createIndexBuffer(const CommandBuffer &cmdbuf, std::string name);

	public:
		Geometry(FrameGraph fg) : _fg{fg} {}
		~Geometry() { cleanup(); }
		
		void cleanup();

		// not terribly neat, but better than subclasses for now...
		bool setupAsQuad ();
		bool setupAsBackgroundQuad ();
		bool setupFromMesh (std::string path);

		ND_ DrawIndexed  enqueueDrawCommands ();
	};

}   // FG


namespace std {
	template<> struct hash<FG::Vertex> {
		size_t operator()(FG::Vertex const& vertex) const {
			return size_t(FGC::HashOf( vertex.pos ) + FGC::HashOf( vertex.col ) + FGC::HashOf( vertex.uv ));
		}
	};
}
