export GDK ?= /opt/toolchains/mars/m68k-elf
include $(GDK)/makefile.gen

.PHONY: rom build-rom
rom: release
	mkdir -p build
	cp out/rom.bin build/DREVKANO_A_ULTIMA_QUEBRA.bin
	sha256sum build/DREVKANO_A_ULTIMA_QUEBRA.bin
	file build/DREVKANO_A_ULTIMA_QUEBRA.bin || true
build-rom: rom
	@echo 'ROM compilada em build/DREVKANO_A_ULTIMA_QUEBRA.bin'
