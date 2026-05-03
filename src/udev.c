#include "udev.h"
#include <libudev.h>
#include <string.h>
#include <xf86drm.h>
#include <stdio.h>
#include <stdlib.h>

struct strings* udev_get_gpus() {
    struct udev* udev = NULL;
    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to initalize udev\n");
        return NULL;
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        fprintf(stderr, "Failed to create udev enumerate\n");
        goto error;
    }

    udev_enumerate_add_match_subsystem(enumerate, "drm");
    udev_enumerate_add_match_sysname(enumerate, DRM_PRIMARY_MINOR_NAME "[0-9]*");
    udev_enumerate_add_match_property(enumerate, "DEVTYPE", "drm_minor");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    if (!devices) {
        fprintf(stderr, "Failed to get udev list entry\n");
        goto error;
    }

    struct udev_list_entry* device;

    struct strings* strings = calloc(1, sizeof(*strings));

    udev_list_entry_foreach(device, devices) {
        struct udev_device* dev = udev_device_new_from_syspath(udev, udev_list_entry_get_name(device));
        da_append(strings, strdup(udev_device_get_devnode(dev)));
        udev_device_unref(dev);
    }

    return strings;

 error:
    if (enumerate) udev_enumerate_unref(enumerate);
	if (udev) udev_unref(udev);
    return NULL;
}
