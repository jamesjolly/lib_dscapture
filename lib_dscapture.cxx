
// lib_dscapture: boost::python module providing DS325 depth feed
// Copyright (C) 2014, James Jolly (jamesjolly@gmail.com)
// See MIT-LICENSE.txt for legalese.

#include <pthread.h>

#include <vector>
#include <iostream>
#include <cmath>
using namespace std;

#include <DepthSense.hxx>
namespace ds = DepthSense;

#include <boost/python.hpp>
namespace bp = boost::python;

#include <boost/numpy.hpp>
namespace bn = boost::numpy;

bool g_device_found = false;
ds::Context g_context;

uint32_t g_dframes_received = 0;
uint32_t g_last_capture_t = 0;
ds::DepthNode g_dnode;
unsigned char* depth_im_data;

const int c_PIXEL_COUNT_QVGA = 76800; // 320x240
const float c_SCALE_DEPTHVAL = 255.0/log10(32001); // deemphasizes differences as range increases

// event handler: new depth frame
void onNewDepthSample(ds::DepthNode node, ds::DepthNode::NewSampleReceivedData data)
{
    g_last_capture_t = data.timeOfCapture/1000000;
    for(size_t j = 0; j < data.depthMap.size(); j++)
    {
    	if(data.depthMap[j] < 32002) // sentinal value for oversaturated
    	{
			depth_im_data[j] = uint8_t(c_SCALE_DEPTHVAL*log10(data.depthMap[j]));
    	}
    	else
    	{
    		depth_im_data[j] = uint8_t(0);
    	}
    }
    g_dframes_received++;
}

// set close mode at 30 Hz
void configureDepthNode(ds::Node node)
{
    if(node.is<ds::DepthNode>() && !g_dnode.isSet())
    {
        g_dnode = node.as<ds::DepthNode>();
        g_dnode.newSampleReceivedEvent().connect(&onNewDepthSample);
        ds::DepthNode::Configuration config = g_dnode.getConfiguration();
        config.frameFormat = ds::FRAME_FORMAT_QVGA;
        config.framerate = 30;
        config.mode = ds::DepthNode::CAMERA_MODE_CLOSE_MODE;
        config.saturation = true;
        g_dnode.setEnableDepthMap(true);
        try
        {
            g_context.requestControl(g_dnode, 0);
            g_dnode.setConfiguration(config);
            cout << "depth node connected\n";
            depth_im_data = new unsigned char[c_PIXEL_COUNT_QVGA];
        }
        catch(ds::Exception& e)
        {
            cout << "Exception: " << e.what() << "\n";
        }
        g_context.registerNode(node);
    }
}

// event handler: setup depth feed
void onNodeConnected(ds::Device device, ds::Device::NodeAddedData data)
{
    configureDepthNode(data.node);
}

// event handler: tear down depth feed
void onNodeDisconnected(ds::Device device, ds::Device::NodeRemovedData data)
{
    if (data.node.is<ds::DepthNode>() && (data.node.as<ds::DepthNode>() == g_dnode))
    {
        g_dnode.unset();
        cout << "depth node disconnected\n";
    }
}

// event handler: device plugged in
void onDeviceConnected(ds::Context context, ds::Context::DeviceAddedData data)
{
    if(!g_device_found)
    {
        data.device.nodeAddedEvent().connect(&onNodeConnected);
        data.device.nodeRemovedEvent().connect(&onNodeDisconnected);
        g_device_found = true;
    }
}

// event handler: device unplugged
void onDeviceDisconnected(ds::Context context, ds::Context::DeviceRemovedData data)
{
    g_device_found = false;
    cout << "device disconnected\n";
}

void* capture_main(void* _)
{
    g_context = ds::Context::create("localhost");
    g_context.deviceAddedEvent().connect(&onDeviceConnected);
    g_context.deviceRemovedEvent().connect(&onDeviceDisconnected);

    // get list of devices already connected
    vector<ds::Device> da = g_context.getDevices();

    // only use first device
    if(da.size() >= 1)
    {
        g_device_found = true;
        da[0].nodeAddedEvent().connect(&onNodeConnected);
        da[0].nodeRemovedEvent().connect(&onNodeDisconnected);
        vector<ds::Node> na = da[0].getNodes();
        cout << "found " << na.size() << " nodes\n";
        for(int n = 0; n < na.size(); n++)
        {
            configureDepthNode(na[n]);
        }
    }

    g_context.startNodes();
    g_context.run();
    g_context.stopNodes();

    if(g_dnode.isSet())
    {
        g_context.unregisterNode(g_dnode);
		delete [ ] depth_im_data;
    }
}

void start()
{
	pthread_t capture_thread;
	if(pthread_create(&capture_thread, NULL, capture_main, NULL))
	{
		cout << "error creating capture thread\n";
	}
}

int ts()
{
	return g_last_capture_t;
}

int at()
{
	return g_dframes_received;
}

bn::ndarray get_frame()
{
	bp::tuple shape = bp::make_tuple(240, 320);
	bp::tuple stride = bp::make_tuple(320, 1);
	bn::dtype dt = bn::dtype::get_builtin<uint8_t>();
	bp::object own;
	if(g_dframes_received == 0)
	{
		return bn::zeros(shape, dt);
	}
	bn::ndarray data_ex = bn::from_data(depth_im_data, dt, shape, stride, own);
	return data_ex.copy();
}

BOOST_PYTHON_MODULE(lib_dscapture)
{
	bn::initialize();
	bp::numeric::array::set_module_and_type("numpy", "ndarray");
	bp::def("get_frame", get_frame);
	bp::def("start", start);
	bp::def("ts", ts);
	bp::def("at", at);
}

