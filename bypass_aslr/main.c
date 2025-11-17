#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld_images.h>
#include <unistd.h>

#define BINARY_BASE 0x100000000ULL

void get_pid(pid_t pid, mach_port_t *task) {
    kern_return_t kr;
    kr = task_for_pid(mach_task_self(), pid, task);
    if (kr != KERN_SUCCESS) {
        printf("[-] task_for_pid failed: %s", mach_error_string(kr));
        *task = MACH_PORT_NULL;
    }
}

static void cleanup_task(mach_port_t *task) {
    if (task != NULL && *task != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), *task);
        *task = MACH_PORT_NULL;
    }
}

bool get_aslr_offset(pid_t pid, const char *image_hint, mach_vm_address_t *slide_out, mach_port_t *task_out) {
    if (slide_out == NULL || task_out == NULL) {
        printf("[-] invalid argument for ASLR lookup\n");
        return false;
    }

    *slide_out = 0;
    *task_out = MACH_PORT_NULL;
    get_pid(pid, task_out);
    if (*task_out == MACH_PORT_NULL) {
        return false;
    }

    task_dyld_info_data_t dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t kr = task_info(*task_out, TASK_DYLD_INFO, (task_info_t)&dyld_info, &count);
    if (kr != KERN_SUCCESS) {
        printf("[-] task_info failed: %s\n", mach_error_string(kr));
        cleanup_task(task_out);
        return false;
    }

    struct dyld_all_image_infos infos;
    mach_vm_size_t read_size = 0;
    kr = mach_vm_read_overwrite(*task_out,
                                dyld_info.all_image_info_addr,
                                sizeof(infos),
                                (mach_vm_address_t)&infos,
                                &read_size);
    if (kr != KERN_SUCCESS || read_size != sizeof(infos)) {
        printf("[-] mach_vm_read_overwrite (infos) failed: %s\n", mach_error_string(kr));
        cleanup_task(task_out);
        return false;
    }

    for (uint32_t i = 0; i < infos.infoArrayCount; ++i) {
        struct dyld_image_info image_info;
        mach_vm_address_t remote_info = (mach_vm_address_t)infos.infoArray + (i * sizeof(struct dyld_image_info));
        read_size = 0;
        kr = mach_vm_read_overwrite(*task_out,
                                    remote_info,
                                    sizeof(image_info),
                                    (mach_vm_address_t)&image_info,
                                    &read_size);
        if (kr != KERN_SUCCESS || read_size != sizeof(image_info)) {
            continue;
        }

        char path_buffer[PATH_MAX] = {0};
        if (image_info.imageFilePath != NULL) {
            read_size = 0;
            kr = mach_vm_read_overwrite(*task_out,
                                        (mach_vm_address_t)image_info.imageFilePath,
                                        PATH_MAX - 1,
                                        (mach_vm_address_t)path_buffer,
                                        &read_size);
            if (kr == KERN_SUCCESS && read_size > 0) {
                size_t clamp = (size_t)((read_size < (PATH_MAX - 1)) ? read_size : (PATH_MAX - 1));
                path_buffer[clamp] = '\0';
            }
        }

        bool path_matches = (image_hint == NULL);
        if (!path_matches && path_buffer[0] != '\0' && strstr(path_buffer, image_hint) != NULL) {
            path_matches = true;
        }

        if (path_matches) {
            mach_vm_address_t load_address = (mach_vm_address_t)image_info.imageLoadAddress;
            *slide_out = load_address - BINARY_BASE;
            printf("[+] ASLR slide for %s: 0x%llx\n",
                   path_buffer[0] ? path_buffer : "main image",
                   (unsigned long long)*slide_out);
            return true;
        }
    }

    printf("[-] Target image %s not found\n", image_hint ? image_hint : "(null)");
    cleanup_task(task_out);
    return false;
}
// for cheats (example assult cube)
void bypassMemoryProtection(mach_port_t task, vm_address_t address, int32_t hp) {
    if (task == MACH_PORT_NULL) {
        printf("[-] invalid task\n");
        return;
    }
    kern_return_t kr;
    kr = vm_protect(task, address, sizeof(hp), FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        printf("[-] vm_protect failed: %s", mach_error_string(kr));
        return;
    }
    printf("[+] Memory protection bypassed\n");
    kr = vm_write(task, address, (vm_address_t)&hp, sizeof(hp));
    if (kr != KERN_SUCCESS) {
        printf("[-] vm_write failed: %s", mach_error_string(kr));
        return;
    }
    printf("[+] Memory modification success\n");
}


void bypass_aslr_for_jumper(vm_address_t address) {
    pid_t pid;
    printf("Enter PID: ");
    scanf("%d", &pid);
    mach_port_t task = MACH_PORT_NULL;
    mach_vm_address_t slide = 0;
    if (!get_aslr_offset(pid, NULL, &slide, &task)) {
        printf("[-] Failed to get ASLR slide\n");
    }
    vm_address_t addrWithAslr = slide + address;
    printf("Your address: 0x%lx\n", addrWithAslr);
    cleanup_task(&task);
}

int main(int argc, const char * argv[]) {
    vm_address_t address;
    printf("Enter offset to function (nm -gU your_binary): ");
    scanf("%lx", &address);
    bypass_aslr_for_jumper(address);
    return EXIT_SUCCESS;
}


