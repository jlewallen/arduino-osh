default: m0 m4 linux

m0:
	mkdir -p build/m0
	cd build/m0 && cmake -D TARGET_M0=ON ../../
	cd build/m0 && $(MAKE)

m4:
	mkdir -p build/m4
	cd build/m4 && cmake -D TARGET_M4=ON ../../
	cd build/m4 && $(MAKE)

linux:
	mkdir -p build/linux
	cd build/linux && cmake -D TARGET_LINUX=ON ../../
	cd build/linux && $(MAKE)

test: linux
	cd build/linux && make test

clean:
	rm -rf build
