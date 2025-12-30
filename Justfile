QEMUFLAGS := "-m 2G -debugcon stdio -enable-kvm -cpu SandyBridge,vmx=on"
HDD := "out/hv.hdd"
ISO := "out/hv.iso"

all:
	just build
	just make-hdd
	just run

debug:
	just build
	just make-hdd
	just run-debug

verbose:
	just build
	just make-hdd
	just run-verbose

get-ovmf:
	curl -L https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/edk2-ovmf.tar.gz | gunzip | tar -xf -

make-hdd:
	rm -f {{HDD}}
	dd if=/dev/zero bs=1M count=0 seek=64 of={{HDD}}
	sgdisk {{HDD}} -n 1:2048 -t 1:ef00 -m 1
	mformat -i {{HDD}}@@1M
	mmd -i {{HDD}}@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i {{HDD}}@@1M out/hv.elf ::/boot
	mcopy -i {{HDD}}@@1M limine.conf limine/limine-bios.sys ::/boot/limine
	mcopy -i {{HDD}}@@1M limine/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i {{HDD}}@@1M limine/BOOTIA32.EFI ::/EFI/BOOT

run:
	qemu-system-x86_64 \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-x86_64.fd,readonly=on \
		-hda {{HDD}} \
		{{QEMUFLAGS}}

run-debug:
	qemu-system-x86_64 \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-x86_64.fd,readonly=on \
		-hda {{HDD}} \
		{{QEMUFLAGS}} -s -S

run-verbose:
	qemu-system-x86_64 \
		-M q35 \
		-drive if=pflash,unit=0,format=raw,file=edk2-ovmf/ovmf-code-x86_64.fd,readonly=on \
		-hda {{HDD}} \
		{{QEMUFLAGS}} -d int --no-reboot --no-shutdown

build:
	make -C hv

clean:
	rm -rf iso_root {{ISO}} {{HDD}}
	rm -rf out