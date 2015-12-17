// -*- C++ -*-
/*!
 * @file  Kinect.cpp
 * @brief RTC Kinect 4 Windows
 * @date $Date$
 *
 * $Id$
 */

#include "Kinect.h"
#include <windows.h>
#include <Ole2.h>
#include "NuiApi.h"

#define _USE_MATH_DEFINES
#include <math.h>
// Module specification
// <rtc-template block="module_spec">
static const char* kinect_spec[] =
  {
    "implementation_id", "Kinect",
    "type_name",         "Kinect",
    "description",       "RTC Kinect 4 Windows",
    "version",           "1.0.0",
    "vendor",            "ysuga.net",
    "category",          "HumanInterface",
    "activity_type",     "PERIODIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables
    "conf.default.debug", "0",
    "conf.default.enable_camera", "1",
    "conf.default.enable_depth", "1",
    "conf.default.camera_width", "640",
    "conf.default.camera_height", "480",
    "conf.default.depth_width", "320",
    "conf.default.depth_height", "240",
    "conf.default.player_index", "0",
	"conf.default.kinect_index", "0",
    // Widget
    "conf.__widget__.debug", "text",
    "conf.__widget__.enable_camera", "text",
    "conf.__widget__.enable_depth", "text",
    "conf.__widget__.camera_width", "text",
    "conf.__widget__.camera_height", "text",
    "conf.__widget__.depth_width", "text",
    "conf.__widget__.depth_height", "text",
    "conf.__widget__.player_index", "text",
	"conf.__widget__.kinect_index", "text",
    // Constraints
    ""
  };
// </rtc-template>

/*!
 * @brief constructor
 * @param manager Maneger Object
 */
Kinect::Kinect(RTC::Manager* manager)
    // <rtc-template block="initializer">
  : RTC::DataFlowComponentBase(manager),
    m_targetElevationIn("targetElevation", m_targetElevation),
    m_imageOut("image", m_image),
    m_depthOut("depth", m_depth),
    m_currentElevationOut("currentElevation", m_currentElevation),
    m_skeletonOut("skeleton", m_skeleton)

    // </rtc-template>
{
}

/*!
 * @brief destructor
 */
Kinect::~Kinect()
{
}



RTC::ReturnCode_t Kinect::onInitialize()
{
  // Registration: InPort/OutPort/Service
  // <rtc-template block="registration">
  // Set InPort buffers
  addInPort("targetElevation", m_targetElevationIn);
  
  // Set OutPort buffer
  addOutPort("image", m_imageOut);
  addOutPort("depth", m_depthOut);
  addOutPort("currentElevation", m_currentElevationOut);
  addOutPort("skeleton", m_skeletonOut);
  
  // Set service provider to Ports
  
  // Set service consumers to Ports
  
  // Set CORBA Service Ports
  
  // </rtc-template>

  // <rtc-template block="bind_config">
  // Bind variables and configuration variable
  bindParameter("debug", m_debug, "0");
  bindParameter("enable_camera", m_enable_camera, "1");
  bindParameter("enable_depth", m_enable_depth, "1");
  bindParameter("camera_width", m_camera_width, "640");
  bindParameter("camera_height", m_camera_height, "480");
  bindParameter("depth_width", m_depth_width, "320");
  bindParameter("depth_height", m_depth_height, "240");
  bindParameter("player_index", m_player_index, "0");
  bindParameter("kinect_index", m_kinect_index, "0");
  // </rtc-template>
  
  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t Kinect::onFinalize()
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Kinect::onStartup(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Kinect::onShutdown(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t Kinect::onActivated(RTC::UniqueId ec_id)
{

	HRESULT hr = NuiCreateSensorByIndex(m_kinect_index, &m_pNuiSensor);
	if ( FAILED(hr) ) {
		std::cerr << " - Can not find Kinect (" << m_kinect_index << ")" << std::endl;
		return RTC::RTC_ERROR;
	}

    /**
	 * The configured values should be reflected to the initialization process.
	 *
	 * m_enable_camera -> camera image
	 * m_enable_depth  -> depth image
	 * m_player_index  -> player index detection
	 *
	 * important: if player indexing is enabled, depth map image is limited up to 320x240
	 */

	DWORD dwFlag = NUI_INITIALIZE_FLAG_USES_SKELETON;
	if(m_enable_camera) {
		dwFlag |= NUI_INITIALIZE_FLAG_USES_COLOR;
	}
	if(m_enable_depth) {
		if(m_player_index) {
			dwFlag |= NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;
		} else {
			dwFlag |= NUI_INITIALIZE_FLAG_USES_DEPTH;
		}
	}

	hr = m_pNuiSensor->NuiInitialize(dwFlag); 
    if( FAILED( hr ) )
    {
		std::cout << " - NUI Initialize Failed." << std::endl;
		return RTC::RTC_ERROR;
    }

	if(m_depth_width == 640 && m_depth_height == 480 && m_enable_depth && m_player_index) {
		std::cout << " - If PlayerIndex and Depth Map is ON, resolution should be 320X240" << std::endl;
		return RTC::RTC_ERROR;
	}
	NUI_IMAGE_RESOLUTION eResolution;
	if(m_camera_width == 640 && m_camera_height == 480) {
		eResolution = NUI_IMAGE_RESOLUTION_640x480;
	} else {
		std::cout << " - Invalid Image Resolution" << std::endl;
		return RTC::RTC_ERROR;
	}
	if(m_enable_camera) {
		hr = m_pNuiSensor->NuiImageStreamOpen(::NUI_IMAGE_TYPE_COLOR, eResolution, 0, 2, NULL, &m_pVideoStreamHandle );
		if( FAILED( hr ) )
		{
			std::cout << " - NUI Image Stream Open Failed." << std::endl;
			return RTC::RTC_ERROR;
		}
	}

	if(m_depth_width == 640 && m_depth_height == 480) {
		eResolution = NUI_IMAGE_RESOLUTION_640x480;
	} else if(m_depth_width == 320 && m_depth_height == 240) {
		eResolution = NUI_IMAGE_RESOLUTION_320x240;
	} else {
		std::cout << " - Invalid Image Resolution" << std::endl;
		return RTC::RTC_ERROR;
	}
	if(m_enable_depth) {
		if(m_player_index) {
			hr = m_pNuiSensor->NuiImageStreamOpen(::NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, eResolution, 0, 2, NULL, &m_pDepthStreamHandle );
		} else {
			hr = m_pNuiSensor->NuiImageStreamOpen(::NUI_IMAGE_TYPE_DEPTH, eResolution, 0, 2, NULL, &m_pDepthStreamHandle );
		}
	}
    if( FAILED( hr ) )
    {
		std::cout << " - NUI Image Stream Open Failed." << std::endl;
		return RTC::RTC_ERROR;
    }

	
	this->m_image.width = m_camera_width;
	this->m_image.height = m_camera_height;
	this->m_image.pixels.length(m_camera_width*m_camera_height*3);

	this->m_depth.bits.length(m_depth_width*m_depth_height);
	coil::sleep(3);

	return RTC::RTC_OK;
}


RTC::ReturnCode_t Kinect::onDeactivated(RTC::UniqueId ec_id)
{
	m_pNuiSensor->NuiShutdown( );

	return RTC::RTC_OK;
}


/**
 * Wriing camera image to imgae port
 *
 * Color space conversion should be accelerated
 * (OpenCV or Intel Performance Primitive Library 
 *  would be useful for this case. OpenMP will 
 * also useful for this kind of processing)
 *
 *
 * Aquired Image by NUI     -> BGRW (4 bytes)
 * RTC::CameraImage struct  -> BGR (3 bytes)
 *
 */

HRESULT Kinect::WriteColorImage(void)
{
	static const long TIMEOUT_IN_MILLI = 100;
	NUI_IMAGE_FRAME ImageFrame;
    HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pVideoStreamHandle, TIMEOUT_IN_MILLI, &ImageFrame );
    if( FAILED( hr ) ) {
		std::cout << "NuiImageStreamGetNextFrame failed." << std::endl;
		return hr;
    }

    //NuiImageBuffer * pTexture = pImageFrame->pFrameTexture;
    //KINECT_LOCKED_RECT LockedRect;
    INuiFrameTexture * pTexture = ImageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect( 0, &LockedRect, NULL, 0 );
    if( LockedRect.Pitch != 0 )
    {
        BYTE * pBuffer = (BYTE*) LockedRect.pBits;
		for(int h = 0;h < m_camera_height;h++) {
			for(int w = 0;w < m_camera_width;w++) {
				BYTE* pixel = pBuffer + (h * m_camera_width * 4) + w * 4;
				BYTE b = pixel[0];
				BYTE g = pixel[1];
				BYTE r = pixel[2];
				int offset = h*m_camera_width*3+w*3;
				m_image.pixels[offset + 0] = b;
				m_image.pixels[offset + 1] = g;
				m_image.pixels[offset + 2] = r;
			}
			m_imageOut.write();
		}

    }
    else {
		std::cout << "Buffer length of received texture is bogus\r\n" << std::endl;
    }
	pTexture->UnlockRect(0);

    m_pNuiSensor->NuiImageStreamReleaseFrame( m_pVideoStreamHandle, &ImageFrame );

	return S_OK;
}

/**
 * Writing Depth Image to the Outport
 *
 * In this release the depth map is converted to the gray scale image.
 * But in the future, the depth map should have its own data type, 
 * because the aquired data includes not only depth data [mm]
 * but also the tracking user id!
 *
 */
HRESULT Kinect::WriteDepthImage(void)
{
	static const long TIMEOUT_IN_MILLI = 100;
	/*
	const NUI_IMAGE_FRAME * pImageFrame = NULL;
    
	*/
	NUI_IMAGE_FRAME ImageFrame;
	BOOL bNearMode = false;
	INuiFrameTexture* pTexture;
	HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, TIMEOUT_IN_MILLI, &ImageFrame );
	//pTexture = ImageFrame.pFrameTexture;
    hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(m_pDepthStreamHandle, &ImageFrame, &bNearMode, &pTexture);
    if( FAILED( hr ) ) {
		std::cout << "NuiImageStreamGetNextFrame failed." << std::endl;
		return hr;
    }

    //NuiImageBuffer * pTexture = pImageFrame->pFrameTexture;
    //KINECT_LOCKED_RECT LockedRect;
    //INuiFrameTexture * pTexture = ImageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect( 0, &LockedRect, NULL, 0 );
    if( LockedRect.Pitch != 0 )
    {
        NUI_DEPTH_IMAGE_PIXEL * pBuffer = reinterpret_cast<NUI_DEPTH_IMAGE_PIXEL*>(LockedRect.pBits);

		m_depth.timestamp = ImageFrame.liTimeStamp.QuadPart;
		m_depth.width = m_depth_width;
		m_depth.height = m_depth_height;
		m_depth.verticalFieldOfView = 45.6  / 180.0 * M_PI;
		m_depth.horizontalFieldOfView = 58.5 * M_PI / 180.0;
		//m_depth.bits.length(m_depth_height* m_depth_width);
		for(int h = 0;h < m_depth_height;h++) {
			for(int w = 0;w < m_depth_width;w++) {
				
				int index = h*m_depth_width + w;
				m_depth.bits[index] = (pBuffer[index].depth);

				//std::cout << pBuffer[index].depth << std::endl;
				/**
				USHORT* pixel = (USHORT*)(pBuffer + (h * m_depth_width * sizeof(USHORT)) + w * sizeof(USHORT));

				USHORT RealDepth = (*pixel & 0xfff8) >> 3;
				USHORT Player = *pixel & 7;

				// transform 13-bit depth information into an 8-bit intensity appropriate
				// for display (we disregard information in most significant bit)
				BYTE depth = 255 - (BYTE)(256*RealDepth/0x0fff);

				unsigned char r, g, b;
				r=g=b = depth/2;
				int offset = h*m_depth_width*3+w*3;
				//m_depth.pixels[offset + 0] = b;
				//m_depth.pixels[offset + 1] = g;
				//m_depth.pixels[offset + 2] = r;
				*/
			}
			m_depthOut.write();
		}

    }
    else {
		std::cout << "Buffer length of received texture is bogus\r\n" << std::endl;
    }
	pTexture->UnlockRect(0);
    m_pNuiSensor->NuiImageStreamReleaseFrame( m_pDepthStreamHandle, &ImageFrame );

	return S_OK;
}


/**
 * Controlling Elevation Motor
 */
HRESULT Kinect::WriteElevation()
{
	HRESULT hr;
	LONG angle;
	if(m_targetElevationIn.isNew()) {
		hr = m_pNuiSensor->NuiCameraElevationSetAngle(m_targetElevation.data);
		if( FAILED(hr) ) {
			return hr;
		}
	}
	hr = m_pNuiSensor->NuiCameraElevationGetAngle(&angle);
	if( FAILED(hr) ) {
		return hr;
	}
	m_currentElevation.data = angle;
	m_currentElevationOut.write();
	return S_OK;
}

void operator<<=(KINECT::Vector4& dst, ::Vector4& src) {
	/*
	dst.v[0] = src.v[0];
	dst.v[1] = src.v[1];
	dst.v[2] = src.v[2];
	dst.v[3] = src.v[3];
	*/
	dst.v[0] = src.x;
	dst.v[1] = src.y;
	dst.v[2] = src.z;
	dst.v[3] = src.w;

}
/*
struct _NUI_SKELETON_DATA {
    NUI_SKELETON_TRACKING_STATE eTrackingState;
    DWORD dwTrackingID;
    DWORD dwEnrollmentIndex;
    DWORD dwUserIndex;
    Vector4 Position;
    Vector4 SkeletonPositions[NUI_SKELETON_POSITION_COUNT];
    NUI_SKELETON_POSITION_TRACKING_STATE 
        eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_COUNT];
    DWORD dwQualityFlags;
*/
void operator<<=(KINECT::NuiSkeletonData& dst, ::NUI_SKELETON_DATA& src) {
	dst.trackingID = src.dwTrackingID;
	dst.trackingState = (KINECT::NUI_SKELETON_TRACKING_STATE)src.eTrackingState;
	dst.enrollmentIndex = src.dwEnrollmentIndex;
	dst.userIndex = src.dwUserIndex;
	dst.position <<= src.Position;
	for(int i = 0;i < 20;i++) {
		dst.skeletonPositions[i] <<= src.SkeletonPositions[i];
		dst.eSkeletonPositionTrackingState[i]  = (KINECT::NUI_SKELETON_POSITION_TRACKING_STATE)src.eSkeletonPositionTrackingState[i];
	}
	dst.qualityFlags = src.dwQualityFlags;
}


/**
 *
 */
HRESULT Kinect::WriteSkeleton()
{
	static const long TIMEOUT_IN_MILI = 100;

	NUI_SKELETON_FRAME skeletonFrame;
	HRESULT ret = m_pNuiSensor->NuiSkeletonGetNextFrame(TIMEOUT_IN_MILI, & skeletonFrame);
	if(FAILED(ret)) {
		return ret;
	}

	m_skeleton.dwFrameNumber = (long)skeletonFrame.dwFrameNumber;
	m_skeleton.liTimeStamp   = (long long)skeletonFrame.liTimeStamp.QuadPart;
	m_skeleton.dwFlags       = skeletonFrame.dwFlags;
	m_skeleton.vFloorClipPlane <<= skeletonFrame.vFloorClipPlane;
	m_skeleton.vNormalToGravity <<= skeletonFrame.vNormalToGravity;

	// int NUI_SKELETON_COUNT=6; Undefined?
	//m_skeleton.skeletonData.length(6);
	for(int i = 0;i < 6;i++) {
		m_skeleton.SkeletonData[i] <<= skeletonFrame.SkeletonData[i];
	}
	m_skeletonOut.write();
	
	return S_OK;
}

RTC::ReturnCode_t Kinect::onExecute(RTC::UniqueId ec_id)
{
	if(m_enable_camera) {
		if( FAILED(WriteColorImage())) {
			return RTC::RTC_ERROR;
		}
	}

	if(m_enable_depth) {
		if( FAILED(WriteDepthImage())) {
			return RTC::RTC_ERROR;
		}
	}

	if( FAILED(WriteElevation()) ) {
		return RTC::RTC_ERROR;
	}

	if( FAILED(WriteSkeleton()) ) {
		return RTC::RTC_ERROR;
	}

	
	return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t Kinect::onAborting(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Kinect::onError(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t Kinect::onReset(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t Kinect::onStateUpdate(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t Kinect::onRateChanged(RTC::UniqueId ec_id)
{
  return RTC::RTC_OK;
}
*/



extern "C"
{
 
  void KinectInit(RTC::Manager* manager)
  {
    coil::Properties profile(kinect_spec);
    manager->registerFactory(profile,
                             RTC::Create<Kinect>,
                             RTC::Delete<Kinect>);
  }
  
};


