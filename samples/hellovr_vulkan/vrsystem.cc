#include <iostream>
#include <vector>
#include <string>

#include "vrsystem.h"
#include "util.h"
#include "global.h"
#include "vulkansystem.h"
#include "shared/Matrices.h"

using namespace std;

VRSystem::VRSystem() {
	render_width = 0;
	render_height = 0;

	near_clip = 0.1f;
	far_clip = 30.0f;

	vr::EVRInitError err = vr::VRInitError_None;
	hmd = vr::VR_Init( &err, vr::VRApplication_Scene );
	check(err);

	render_models = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &err );

	driver_str = query_str(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	display_str = query_str(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

	cout << "driver: " << driver_str << " display: " << display_str << endl;

	if ( !vr::VRCompositor() ) {
		cerr << "Couldn't create VRCompositor" << endl;
		throw "";
	}

	eye_pos_buffer.resize(2);
	eye_pos_buffer[0].init(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrix4), HOST_COHERENT);
	eye_pos_buffer[1].init(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrix4), HOST_COHERENT);
	

}

void VRSystem::render_stereo_targets() {
	auto vk = Global::vk();
	VkViewport viewport = { 0.0f, 0.0f, (float ) render_width, ( float ) render_height, 0.0f, 1.0f };
	vkCmdSetViewport( vk.cur_cmd_buffer, 0, 1, &viewport );
	VkRect2D scissor = { 0, 0, render_width, render_height};
	vkCmdSetScissor( vk.cur_cmd_buffer, 0, 1, &scissor );

	left_eye_fb.to_colour_optimal();
	if (left_eye_fb.depth_stencil.layout == VK_IMAGE_LAYOUT_UNDEFINED)
		left_eye_fb.to_depth_optimal();
	left_eye_fb.start_render_pass();
  	//render stuff
	left_eye_fb.end_render_pass();
	left_eye_fb.to_read_optimal();


	right_eye_fb.to_colour_optimal();
	if (right_eye_fb.depth_stencil.layout == VK_IMAGE_LAYOUT_UNDEFINED)
		right_eye_fb.to_depth_optimal();
	right_eye_fb.start_render_pass();
  	//render stuff
	right_eye_fb.end_render_pass();
	right_eye_fb.to_read_optimal();

}

Matrix4 VRSystem::get_eye_transform( vr::Hmd_Eye eye )
{
	vr::HmdMatrix34_t mat_eye = hmd->GetEyeToHeadTransform( eye );
	Matrix4 mat(
		mat_eye.m[0][0], mat_eye.m[1][0], mat_eye.m[2][0], 0.0, 
		mat_eye.m[0][1], mat_eye.m[1][1], mat_eye.m[2][1], 0.0,
		mat_eye.m[0][2], mat_eye.m[1][2], mat_eye.m[2][2], 0.0,
		mat_eye.m[0][3], mat_eye.m[1][3], mat_eye.m[2][3], 1.0f
		);

	return mat.invert();
}

Matrix4 VRSystem::get_hmd_projection( vr::Hmd_Eye eye )
{
	vr::HmdMatrix44_t mat = hmd->GetProjectionMatrix( eye, near_clip, far_clip );

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		);
}


Matrix4 VRSystem::get_view_projection( vr::Hmd_Eye eye ) {
	if( eye == vr::Eye_Left )
		return projection_left * eye_pos_left * hmd_pose;
	else if( eye == vr::Eye_Right )
		return projection_right * eye_pos_right * hmd_pose;
	throw StringException("not valid eye");
}

void VRSystem::render_scene() {

}

void VRSystem::setup_render_models()
{
	for( uint32_t d = vr::k_unTrackedDeviceIndex_Hmd + 1; d < vr::k_unMaxTrackedDeviceCount; d++ )
	{
		if( !hmd->IsTrackedDeviceConnected( d ) )
			continue;

		//TODO: Setup render model

		//SetupRenderModelForTrackedDevice( d );
	}

}

void VRSystem::setup_render_model_for_device(int d) {

}

void VRSystem::update_track_pose() {
	vr::VRCompositor()->WaitGetPoses(tracked_pose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

	for ( int d = 0; d < vr::k_unMaxTrackedDeviceCount; ++d )
	{
		if ( tracked_pose[d].bPoseIsValid )
		{
			tracked_pose_mat4[d] = vrmat_to_mat4( tracked_pose[d].mDeviceToAbsoluteTracking );
			device_class[d] = hmd->GetTrackedDeviceClass(d);
		}
	}

	if ( tracked_pose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
	{
		hmd_pose = tracked_pose_mat4[vr::k_unTrackedDeviceIndex_Hmd];
		hmd_pose.invert();
	}
}

string VRSystem::query_str(vr::TrackedDeviceIndex_t devidx, vr::TrackedDeviceProperty prop) {
	vr::TrackedPropertyError *err = NULL;
	uint32_t buflen = hmd->GetStringTrackedDeviceProperty( devidx, prop, NULL, 0, err );
	if( buflen == 0)
		return "";

	string buf(' ', buflen);
	buflen = hmd->GetStringTrackedDeviceProperty( devidx, prop, &buf[0], buflen, err );
	return buf;      
}

vector<string> VRSystem::get_inst_ext_required() {
	uint32_t buf_size = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( nullptr, 0 );
	if (!buf_size)
		throw StringException("no such GetVulkanInstanceExtensionsRequired");

	string buf(' ', buf_size);
	vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( &buf[0], buf_size );

    // Break up the space separated list into entries on the CUtlStringList
	vector<string> ext_list;
	string cur_ext;
	uint32_t idx = 0;
	while ( idx < buf_size ) {
		if ( buf[ idx ] == ' ' ) {
			ext_list.push_back( cur_ext );
			cur_ext.clear();
		} else {
			cur_ext += buf[ idx ];
		}
		++idx;
	}
	if ( cur_ext.size() > 0 ) {
		ext_list.push_back( cur_ext );
	}

	return ext_list;
}

vector<string> VRSystem::get_dev_ext_required() {
	auto vk = Global::vk();
	uint32_t buf_size = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( vk.phys_dev, nullptr, 0 );
	if (!buf_size)
		throw StringException("No such GetVulkanDeviceExtensionsRequired");

	string buf(' ', buf_size);
	vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( vk.phys_dev, &buf[0], buf_size );

    // Break up the space separated list into entries on the CUtlStringList
	vector<string> ext_list;
	string cur_ext;
	uint32_t idx = 0;
	while ( idx < buf_size ) {
		if ( buf[ idx ] == ' ' ) {
			ext_list.push_back( cur_ext );
			cur_ext.clear();
		} else {
			cur_ext += buf[ idx ];
		}
		++idx;
	}
	if ( cur_ext.size() > 0 ) {
		ext_list.push_back( cur_ext );
	}

	return ext_list;
}

vector<string> VRSystem::get_inst_ext_required_verified() {
	auto instance_ext_req = get_inst_ext_required();
	instance_ext_req.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

#if defined ( _WIN32 )
	instance_ext_req.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#else
	//instance_ext_req.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#endif


	uint32_t n_instance_ext(0);
	check( vkEnumerateInstanceExtensionProperties( NULL, &n_instance_ext, NULL ), "vkEnumerateInstanceExtensionProperties");

	vector<VkExtensionProperties> ext_prop(n_instance_ext);

	check( vkEnumerateInstanceExtensionProperties( NULL, &n_instance_ext, &ext_prop[0]), "vkEnumerateInstanceExtensionProperties" );

	for (auto req_inst : instance_ext_req) {
		bool found(false);
		for (auto prop : ext_prop)
			if (req_inst == string(prop.extensionName))
				found = true;
			if (!found) {
				cerr << "couldn't find extension" << endl;
				throw "";
			}
		}
		
		return instance_ext_req;
	}

	vector<string> VRSystem::get_dev_ext_required_verified() {
		auto vk = Global::vk();
		auto dev_ext_req = get_dev_ext_required();
		dev_ext_req.push_back( VK_KHR_SURFACE_EXTENSION_NAME );

#if defined ( _WIN32 )
		dev_ext_req.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#else
		//dev_ext_req.push_back( VK_KHR_XLIB_SURFACE_EXTENSION_NAME );
#endif


		uint32_t n_dev_ext(0);
		check( vkEnumerateDeviceExtensionProperties( vk.phys_dev, NULL, &n_dev_ext, NULL ), "vkEnumerateDeviceExtensionProperties");

		vector<VkExtensionProperties> ext_prop(n_dev_ext);

		check( vkEnumerateDeviceExtensionProperties( vk.phys_dev, NULL, &n_dev_ext, &ext_prop[0]), "vkEnumerateDeviceExtensionProperties" );

		for (auto req_dev : dev_ext_req) {
			bool found(false);
			for (auto prop : ext_prop)
				if (req_dev == string(prop.extensionName))
					found = true;
				if (!found) {
					cerr << "couldn't find extension" << endl;
					throw "";
				}
			}
			
			return dev_ext_req;
		}

		VRSystem::~VRSystem() {
			vr::VR_Shutdown();
		}

		void VRSystem::setup_render_targets() {
			hmd->GetRecommendedRenderTargetSize( &render_width, &render_height );
			left_eye_fb.init(render_width, render_height);
			right_eye_fb.init(render_width, render_height);
			
		}