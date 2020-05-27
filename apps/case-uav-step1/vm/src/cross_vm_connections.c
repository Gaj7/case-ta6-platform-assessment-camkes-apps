// Adapted from https://github.com/SEL4PROJ/camkes-arm-vm/blob/master/apps/vm_cross_connector/src/cross_vm_connections.c

#include <camkes.h>
#include <vmlinux.h>
#include <sel4vm/guest_vm.h>

#include <sel4vmmplatsupport/drivers/cross_vm_connection.h>
#include <sel4vmmplatsupport/drivers/pci_helper.h>
#include <pci/helper.h>

#ifdef CONFIG_PLAT_QEMU_ARM_VIRT
#define CONNECTION_BASE_ADDRESS 0xDF000000
#else
#define CONNECTION_BASE_ADDRESS 0x3F000000
#endif

extern dataport_caps_handle_t id_list_handle;
static struct camkes_crossvm_connection connections[] = {
    {&id_list_handle, NULL, -1},
};

void init_cross_vm_connections(vm_t *vm, void *cookie) {
    cross_vm_connections_init(vm, CONNECTION_BASE_ADDRESS, connections, ARRAY_SIZE(connections));
}

DEFINE_MODULE(cross_vm_connections, NULL, init_cross_vm_connections)
