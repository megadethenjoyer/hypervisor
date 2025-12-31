Type 1 VMX hypervisor

Building:
$ git clone --recurse-submodules https://github.com/megadethenjoyer/hypervisor.git; cd hypervisor
$ just get-ovmf
$ just

Requires mtools, qemu, just and make
Currently relies heavily on limine boot protocol guarantees (e.g. GDT, paging)

In the later stages I plan on implementing hardware emulation in a guest, sort of how Xen works
Hopefully I might just be able to (at least to get a simple solution) load SeaBIOS onto the guest,
let it load GRUB and let that load Linux, and use qemu somehow