default: m0 m4 linux

m0:
	mkdir -p build/m0
	cd build/m0 && cmake -D TARGET_M0=ON ../../
	$(MAKE) -C build/m0

m4:
	mkdir -p build/m4
	cd build/m4 && cmake -D TARGET_M4=ON ../../
	$(MAKE) -C build/m4

linux:
	mkdir -p build/linux
	cd build/linux && cmake -D TARGET_LINUX=ON ../../
	$(MAKE) -C build/linux

test: linux
	env GTEST_COLOR=1 $(MAKE) -C build/linux test ARGS=-VV

clean:
	rm -rf build
