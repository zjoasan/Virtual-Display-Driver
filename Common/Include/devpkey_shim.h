#pragma once

// Include the normal header first if present
#include <devpkey.h>

// If the SDK you're on doesn't provide these, define them yourself
#ifndef DEVPKEY_Device_LocationInfo
// {A45C254E-DF1C-4EFD-8020-67D146A850E0}, PID 15
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo,
    0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20,
    0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);
#endif

#ifndef DEVPKEY_Device_LocationPaths
// {A45C254E-DF1C-4EFD-8020-67D146A850E0}, PID 37
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationPaths,
    0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20,
    0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 37);
#endif