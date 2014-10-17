lib_dscapture
=============

Start on a DS325 Python API. =)

Includes an example that uses OpenCV to render the depth video. Tested on Ubuntu 12.04 with Python 2.7.3 and Boost 1.46.

To get started, add the appropriate DepthSenseSDK include and lib dirs to
the include_directories and link_directories lists in CMakeLists.txt. Then
build the boost::python module and the example script for a spin:

```
cmake CMakeLists.txt
make
./player_cv2.py
```

