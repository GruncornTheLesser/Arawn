#pragma once
#include <cstdint>
#include <array>
#include <functional>
#include <stdexcept>


#include <core/settings.h>
#include <core/instance.h>
#include <core/window.h>
#include <core/device.h>
#include <core/swap.h>
#include <core/frame.h>

#include <pipeline/cull/frustum.h>
#include <pipeline/cull/light.h>
#include <pipeline/cull/mesh.h>
#include <pipeline/cull/occlusion.h>
#include <pipeline/generate/cluster.h>
#include <pipeline/generate/tile.h>
#include <pipeline/generate/ztree.h>
#include <pipeline/depth.h>
#include <pipeline/forward.h>

#include <image/depth.h>
#include <image/color.h>

#include <pass/earlycull.h>
#include <pass/depthonly.h>
#include <pass/latecull.h>
#include <pass/forward.h>
#include <pass/present.h>

#include <buffer/cull/light.h>
#include <buffer/cull/early.h>
#include <buffer/cull/late.h>
#include <buffer/cluster.h>
#include <buffer/tile.h>



namespace arawn {
	struct Engine {
		Engine(const core::Settings& settings);
		~Engine() noexcept;

		void configure(const core::Settings& settings);
		bool closed() const;
		void render();

		// settings
		core::Settings settings;

		// core
		core::Instance instance;
		core::Window window;
		core::Device device;
		
		//domains
		core::Frame frame;
		core::Swap swap;

		// attachments
		struct {
			struct {
				pipeline::cull::Frustum frustum;
				pipeline::cull::Occlusion occlusion;
				pipeline::cull::Light light;
				pipeline::cull::Mesh mesh;
			} cull;
			struct {
				pipeline::gen::ZTree ztree;
				pipeline::gen::Tile tile;
				pipeline::gen::Cluster cluster;
			} gen;
			pipeline::DepthOnly depthOnly;
			pipeline::Forward forward;
		} pipeline;

		struct {
			image::Depth depth;
			image::Color color;
			struct {
				buffer::cull::Early early;
				buffer::cull::Late late;
				buffer::cull::Late mesh;
			} cull;
		} attachments;

		/*
		struct {
			pool::Node node;
			pool::Mesh mesh;
			pool::Light light;
			pool::Texture texture;
			pool::Material material;
		} pool;
		*/
	};
}