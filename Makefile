default: m0 m4

m0:
	mkdir -p build/m0
	cd build/m0 && cmake -D TARGET_M0=ON ../../
	cd build/m0 && $(MAKE)

m4:
	mkdir -p build/m4
	cd build/m4 && cmake -D TARGET_M4=ON ../../
	cd build/m4 && $(MAKE)

clean:
	rm -rf build
