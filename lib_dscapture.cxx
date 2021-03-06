
// lib_dscapture: boost::python module providing DS325 depth feed
// Copyright (C) 2014, James Jolly (jamesjolly@gmail.com)
// See MIT-LICENSE.txt for legalese.

#include <vector>
#include <iostream>
#include <cmath>
using namespace std;

#include <DepthSense.hxx>
namespace ds = DepthSense;

#include <boost/thread.hpp>

#include <boost/python.hpp>
namespace bp = boost::python;

#include <boost/numpy.hpp>
namespace bn = boost::numpy;

bool g_device_found = false;
ds::Context g_context;

ds::DepthNode g_dnode;
unsigned char* depth_im_data;
uint32_t g_dframes_received = 0;
uint32_t g_last_depth_time = 0;

ds::ColorNode g_cnode;
unsigned char* color_im_data;
uint32_t g_cframes_received = 0;
uint32_t g_last_color_time = 0;

const int c_PIXEL_COUNT_QVGA = 76800; // 320x240
const int c_PIXEL_COUNT_VGA = 921600; // 640x480
const float c_SCALE_DEPTHVAL = 255.0/log10(32001);

// cameras default to 30 Hz, close mode for TOF
const int c_DEFAULT_DEPTH_FRAMERATE = 30;
const int c_DEFAULT_CONFIG_MODE = ds::DepthNode::CAMERA_MODE_CLOSE_MODE;

// event handler: new color frame
void onNewColorSample(ds::ColorNode node, ds::ColorNode::NewSampleReceivedData data)
{
    for(size_t j = 0; j < 3*c_PIXEL_COUNT_VGA; j++)
    {
        color_im_data[j] = data.colorMap[j];
    }
    g_last_color_time = data.timeOfCapture/1000000;
    g_cframes_received++;
}

// event handler: new depth frame
void onNewDepthSample(ds::DepthNode node, ds::DepthNode::NewSampleReceivedData data)
{
    for(size_t j = 0; j < data.depthMap.size(); j++)
    {
        if(data.depthMap[j] < 32002) // sentinal value for oversaturated
        {
            depth_im_data[j] = uint8_t(c_SCALE_DEPTHVAL*log10(data.depthMap[j] + 1));
            // log10( ) - deemphasizes differences as range increases
        }
        else
        {
            depth_im_data[j] = uint8_t(0);
        }
    }
    g_last_depth_time = data.timeOfCapture/1000000;
    g_dframes_received++;
}

void configureDepthNode(ds::Node node, int p_config_framerate, int p_config_mode)
{
    if(node.is<ds::DepthNode>() && !g_dnode.isSet())
    {
        g_dnode = node.as<ds::DepthNode>();
        g_dnode.newSampleReceivedEvent().connect(&onNewDepthSample);
        ds::DepthNode::Configuration config = g_dnode.getConfiguration();
        config.frameFormat = ds::FRAME_FORMAT_QVGA;
        config.framerate = p_config_framerate;
        config.mode = ds::DepthNode::CameraMode(p_config_mode);
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

void configureColorNode(ds::Node node, int p_config_framerate)
{
    if(node.is<ds::ColorNode>() && !g_cnode.isSet())
    {
        g_cnode = node.as<ds::ColorNode>();
        g_cnode.newSampleReceivedEvent().connect(&onNewColorSample);
        ds::ColorNode::Configuration config = g_cnode.getConfiguration();
        config.frameFormat = ds::FRAME_FORMAT_VGA;
        config.compression = ds::COMPRESSION_TYPE_MJPEG;
        config.powerLineFrequency = ds::POWER_LINE_FREQUENCY_60HZ;
        config.framerate = p_config_framerate;
        g_cnode.setEnableColorMap(true);
        try
        {
            g_context.requestControl(g_cnode, 0);
            g_cnode.setConfiguration(config);
            cout << "color node connected\n";
            color_im_data = new unsigned char[c_PIXEL_COUNT_VGA*3];
        }
        catch(ds::Exception& e)
        {
            cout << "Exception: " << e.what() << "\n";
        }
        g_context.registerNode(node);
    }
}

// node discovered event: setup depth, color nodes
void onNodeConnected(ds::Device device, ds::Device::NodeAddedData data)
{
    configureDepthNode(data.node, c_DEFAULT_DEPTH_FRAMERATE, c_DEFAULT_CONFIG_MODE);
    configureColorNode(data.node, c_DEFAULT_DEPTH_FRAMERATE);
}

// event handler: tear down depth feed
void onNodeDisconnected(ds::Device device, ds::Device::NodeRemovedData data)
{
    if(data.node.is<ds::DepthNode>() && (data.node.as<ds::DepthNode>() == g_dnode))
    {
        g_dnode.unset();
        cout << "depth node disconnected\n";
    }
    if(data.node.is<ds::ColorNode>() && (data.node.as<ds::ColorNode>() == g_cnode))
    {
        g_cnode.unset();
        cout << "color node disconnected\n";
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

void capture_main(int p_config_framerate, int p_config_mode)
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
            configureDepthNode(na[n], p_config_framerate, p_config_mode);
            configureColorNode(na[n], p_config_framerate);
        }
    }

    // starts DepthSense event loop
    g_context.startNodes();
    g_context.run();
}

void start(
    int p_config_framerate=c_DEFAULT_DEPTH_FRAMERATE, 
    int p_config_mode=c_DEFAULT_CONFIG_MODE)
{
    boost::thread capture_thread(capture_main, p_config_framerate, p_config_mode);
}

void stop()
{
    cout << "stopping capture thread\n";
    g_context.stopNodes();
    if(g_dnode.isSet())
    {
        g_context.unregisterNode(g_dnode);
        delete [ ] depth_im_data;
    }
    if(g_cnode.isSet())
    {
        g_context.unregisterNode(g_cnode);
        delete [ ] color_im_data;
    }
}

int last_ctime()
{
    return g_last_color_time;
}

int last_cframe()
{
    return g_cframes_received;
}

int last_dtime()
{
    return g_last_depth_time;
}

int last_dframe()
{
    return g_dframes_received;
}

bn::ndarray get_cframe()
{
    bp::tuple shape = bp::make_tuple(480, 640, 3);
    bn::dtype dt = bn::dtype::get_builtin<uint8_t>();
    if(g_cframes_received == 0)
    {
        return bn::zeros(shape, dt).copy();
    }
    bp::object own;
    bp::tuple stride = bp::make_tuple(3*640, 3, 1);
    bn::ndarray data_ex = bn::from_data(color_im_data, dt, shape, stride, own);    
    return data_ex.copy();
}

bn::ndarray get_dframe()
{
    bp::tuple shape = bp::make_tuple(240, 320);
    bn::dtype dt = bn::dtype::get_builtin<uint8_t>();
    if(g_dframes_received == 0)
    {
        return bn::zeros(shape, dt).copy();
    }
    bp::object own;
    bp::tuple stride = bp::make_tuple(320, 1);
    bn::ndarray data_ex = bn::from_data(depth_im_data, dt, shape, stride, own);
    return data_ex.copy();
}

BOOST_PYTHON_MODULE(lib_dscapture)
{
    bn::initialize();
    bp::numeric::array::set_module_and_type("numpy", "ndarray");
    bp::def("start", start);
    bp::def("stop", stop);

    bp::def("get_cframe", get_cframe);
    bp::def("last_ctime", last_ctime);
    bp::def("last_cframe", last_cframe);

    bp::def("get_dframe", get_dframe);
    bp::def("last_dtime", last_dtime);
    bp::def("last_dframe", last_dframe);
}

