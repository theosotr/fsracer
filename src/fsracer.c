#include "dr_api.h"
#include "drmgr.h"
#include "drwrap.h"


static void
wrap_pre_uv_fs_open(void *wrapcxt, OUT void **user_data);


static void
module_load_event(void *drcontext, const module_data_t *mod, bool loaded)
{
    app_pc towrap = (app_pc) dr_get_proc_address(mod->handle, "uv_fs_open");
    if (towrap != NULL) {
        bool wrapped = drwrap_wrap(towrap, wrap_pre_uv_fs_open, NULL);
        if (!wrapped) {
            dr_printf("Couldn't wrap our the uv_fs_open function\n");
        }
    }
}


static void
event_exit(void)
{
    dr_printf("Stopping the FSRacer Client...\n");
    drwrap_exit();
    drmgr_exit();
}


DR_EXPORT void
dr_client_main(client_id_t client_id, int argc, const char *argv[])
{
    dr_set_client_name("Client for Detecting Data Races in Files", "");
    dr_printf("Starting the FSRacer Client...\n");
    drmgr_init();
    drwrap_init();
    dr_register_exit_event(event_exit);
    drmgr_register_module_load_event(module_load_event);
}


static void
wrap_pre_uv_fs_open(void *wrapcxt, OUT void **user_data)
{
    dr_printf("The uv_fs_open function is going to be invoked\n");
    // We get the third argument of the uv_fs_open()
    // that corresponds to the path.
    const char *path = drwrap_get_arg(wrapcxt, 2);
    dr_printf("uv_fs_open(%s)\n", path);
}
