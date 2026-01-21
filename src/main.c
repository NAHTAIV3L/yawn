#include <stdio.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unistd.h>

int main() {
    int fd = -1;

    drmDevicePtr devices[64] = { NULL };
    int num_devices = drmGetDevices2(0, devices, sizeof(devices) / sizeof(devices[0]));
    int used_device = -1;
    printf("number of devices: %d\n", num_devices);
    for (int i = 0; i < num_devices; i++) {
        if (!(devices[i]->available_nodes & 1 << DRM_NODE_PRIMARY)) continue;
        fd = open(devices[i]->nodes[DRM_NODE_PRIMARY], O_RDWR);
        if (fd < 0)
            continue;

        used_device = i;
        break;
    }

    for (int i = 0; i < num_devices; i++) {
        if (i == used_device) continue;
        drmFreeDevice(&(devices[i]));
    }
    drmDevice* device = devices[used_device];
    printf("device file: %s\n", (device->nodes[DRM_NODE_PRIMARY]));

    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        printf("DRM version: %d.%d.%d\n",
               version->version_major,
               version->version_minor,
               version->version_patchlevel);
        printf("DRM driver: %s\n", version->name);
        drmFreeVersion(version);
    }
    drmModeRes* resources = drmModeGetResources(fd);
    if (!resources) return 1;

    printf("number of connectors: %d\n", resources->count_connectors);
    printf("number of crtcs: %d\n", resources->count_crtcs);
    printf("max resolution: %d, %d\n", resources->max_width, resources->max_height);
    printf("min resolution: %d, %d\n", resources->min_width, resources->min_height);

    drmModeFreeResources(resources);
    drmFreeDevice(&device);
    close(fd);
    return 0;
}
