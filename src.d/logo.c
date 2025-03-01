/* -------------------------------------------------------------------------- *
 * Client surface (window) for a Wayland compositor                           *
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <string.h>
#include <unistd.h>

#include <wayland-client.h>
#include "../inc.d/xdg-shell-protocol-client.h"

#include <syscall.h>
#include <sys/mman.h>


static void registry_global(void *,
                            struct wl_registry *,
                            uint32_t,
                            const char *,
                            uint32_t);
static void registry_global_remove(void *, struct wl_registry *, uint32_t);
static void xsurface_configure(void *, struct xdg_surface *, uint32_t);
static void xtoplevel_configure(void *,
                                struct xdg_toplevel *,
                                int32_t,
                                int32_t,
                                struct wl_array *);
static void xtoplevel_configure_bounds(void *,
                                       struct xdg_toplevel *,
                                       int32_t,
                                       int32_t);
static void xtoplevel_close(void *, struct xdg_toplevel *);
static void render_surface(uint8_t *);


// Settings
char *wayland_display = "wayland-1";
uint32_t width = 288;
uint32_t height = 288;

// Use global variables to not have to pass stuff over *void *data*
static struct wl_display *display;
static struct wl_registry *registry;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *shell;
static struct xdg_surface *xsurface;
static struct xdg_toplevel *xtoplevel;
static struct wl_surface *surface;
static struct wl_shm_pool *shm_pool;
static struct wl_buffer *buffer;
static uint8_t *pixle_data;
static bool quit;

// Listeners with their corresponding events (i.e. reactions to server changes)
static struct wl_registry_listener registry_listener = {
    // Called for each available global
    .global = registry_global,
    // Reaction of client to removal of globals (e.g. unplugged screen)
    .global_remove = registry_global_remove
};
static struct xdg_surface_listener xsurface_listener = {
    .configure = xsurface_configure
};
static struct xdg_toplevel_listener xtoplevel_listener = {
    .configure = xtoplevel_configure,
    .close = xtoplevel_close,
    .configure_bounds = xtoplevel_configure_bounds
};


int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    uint64_t size = 4*width*height;
    uint64_t stride = 4*width;
    quit = false;

    // Connect to a wayland display; *display* represents connection to server
    display = wl_display_connect(wayland_display);
    if (!display) {
        printf("Wayland display *%s* does not exist\n", wayland_display);
        exit(1);
    } else {
        printf("Connected to wayland display *%s*\n", wayland_display);
    }

    // Request creating a registry object to list and bind the client to globals
    registry = wl_display_get_registry(display);

    // Add registry listener
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // Query initial globals
    wl_display_roundtrip(display);

    // Check if all reuired globals are there
    if (!compositor || !shm || !shell) {
        printf("%s\n", "Required globals could not be obtained");
        exit(1);
    }

    // Send a request to create a surface to attach buffers for display
    surface = wl_compositor_create_surface(compositor);

    /* Allocate a shared memory pool for the buffer; only the address of the  *
     * buffer in this pool, exclusively for the client and compositor, must   *
     * be sent over the socket                                                */
    int fd;
    if ((fd = syscall(SYS_memfd_create, "buffer", 0)) < 0) {
        printf("%s\n", "No file descriptor obtained (memfd_create return: -1)");
        exit(1);
    }
    if (ftruncate(fd, size) < 0) {
        printf("%s\n", "Could not truncate size of file descriptor");
        exit(1);
    };

    // Tell compositor to make a similar mmap call (send request)
    shm_pool = wl_shm_create_pool(shm, fd, size);

    /* Map file descriptor to memory (no offset as always one buffer in pool) *
     * Note that *pixle_data* can be used to set the data saven in the buffer */
    pixle_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // Request a (here single) buffer for the cleint in the shared memory pool
    buffer = wl_shm_pool_create_buffer(shm_pool,
                                       0, // No offset, just one buffer
                                       width,
                                       height,
                                       stride,
                                       WL_SHM_FORMAT_ARGB8888);

    // Request a surface
    xsurface = xdg_wm_base_get_xdg_surface(shell, surface);
    xdg_surface_add_listener(xsurface, &xsurface_listener, NULL);

    // Request a toplevel to the (xdg) surface
    xtoplevel = xdg_surface_get_toplevel(xsurface);
    xdg_toplevel_add_listener(xtoplevel, &xtoplevel_listener, NULL);
    xdg_toplevel_set_title(xtoplevel, "Wayland Logo");

    // Render and commit to surface
    render_surface(pixle_data);
    wl_surface_commit(surface);

    // Loop keeping client alive and sending events to the client
    while (true) {
        if (quit) break;
        wl_display_dispatch(display); // Default event queue; block until events
    }

    // Clean up
    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}

// Handle event of noticing a global
static void registry_global(void *data,
                            struct wl_registry *wl_registry,
                            uint32_t name,
                            const char *interface,
                            uint32_t version) {
    (void)data; (void)name; (void)interface; (void)version;

    // Obtain required globals (in registry) by binding them to the interface
    if (!strcmp(interface, "wl_compositor")) {
        compositor = wl_registry_bind(wl_registry,
                                      name,
                                      &wl_compositor_interface,
                                      4);
    } else
    if (!strcmp(interface, "wl_shm")) {
        shm = wl_registry_bind(wl_registry,
                               name,
                               &wl_shm_interface,
                               1);
    } else
    if (!strcmp(interface, "xdg_wm_base")) {
        shell = wl_registry_bind(wl_registry,
                                 name,
                                 &xdg_wm_base_interface,
                                 1);
    }
}

// Handle removal of globals
static void registry_global_remove(void *data,
                                   struct wl_registry *wl_registry,
                                   uint32_t name) {
    (void)data; (void)wl_registry; (void)name;
    // Just don't care about loosing globals ...
}

// Configure xdg surface
static void xsurface_configure(void *data,
                               struct xdg_surface *xdg_surface,
                               uint32_t serial) {
    (void)data; (void)xdg_surface; (void)serial;
    wl_surface_attach(surface, buffer, 0, 0); // Attach buffer to surface
    wl_surface_damage_buffer(surface, 0, 0, width, height); // Overwrite
    xdg_surface_ack_configure(xdg_surface, serial);
    render_surface(pixle_data);
    wl_surface_commit(surface);
}

// Configure xdg toplevel
static void xtoplevel_configure(void *data,
                                struct xdg_toplevel *xdg_toplevel,
                                int32_t w,
                                int32_t h,
                                struct wl_array *states) {
    (void)data; (void)xdg_toplevel; (void)w; (void)h; (void)states;
}

// Close xdg toplevel
static void xtoplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    (void)data; (void)xdg_toplevel;
    quit = true;
}

// Configure bounds of toplevel
static void xtoplevel_configure_bounds(void *data,
                                       struct xdg_toplevel *xdg_toplevel,
                                       int32_t w,
                                       int32_t h) {
    (void)data; (void)xdg_toplevel; (void)w; (void)h;
}

// Render the surface
static void render_surface(uint8_t *data) {
    uint64_t k;
    char line[100];
    FILE *fd = fopen("./img.d/logo.dat", "r");
    for (k=0; k<height*width; k++) {
        (void)fgets(line, sizeof(line), fd);
        (void)sscanf(line,
                     "%hhu:%hhu:%hhu:%hhu\n",
                     data+4*k+2,
                     data+4*k+1,
                     data+4*k+0,
                     data+4*k+3);
    }
    fclose(fd);
}
