#pragma once
#include <core/settings.h>
#include <core/instance.h>
#include <core/window.h>
#include <core/device.h>
#include <core/swap.h>
#include <core/frame.h>

#include <pool/node.h>
#include <pool/mesh.h>
#include <pool/light.h>
#include <pool/texture.h>
#include <pool/material.h>

#include <attachment/depth.h>
#include <attachment/color.h>
#include <attachment/ztree.h>
#include <attachment/cull/mesh.h>
#include <attachment/cull/light.h>

#include <pipeline/cull/frustum.h>
#include <pipeline/cull/light.h>
#include <pipeline/cull/mesh.h>
#include <pipeline/cull/occlusion.h>
#include <pipeline/generate/cluster.h>
#include <pipeline/generate/tile.h>
#include <pipeline/generate/ztree.h>
#include <pipeline/depth.h>
#include <pipeline/forward.h>

#include <pass/earlycull.h>
#include <pass/depthonly.h>
#include <pass/latecull.h>
#include <pass/forward.h>
#include <pass/present.h>


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
		
		// pools
		struct {
			pool::Node node;
			pool::Mesh mesh;
			pool::Light light;
			pool::Texture texture;
			pool::Material material;
		} pool;

		// attachments
		struct {
			attachment::Depth depth;
			attachment::Color color;
			attachment::ZTree ztree;
			struct {
				attachment::cull::Mesh mesh;
				attachment::cull::Light light;
			} cull;
		} attachment;

		// pipelines
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
			pass::EarlyCull earlycull;
			pass::DepthOnly depthonly;
			pass::LateCull latecull;
			pass::Forward forward;
			pass::Present present;
		} pass;
	};
}