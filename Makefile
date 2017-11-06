SKIA_LIBS = -lharfbuzz -lpng -lwebp -framework ApplicationServices -lwebpmux -lwebpdemux

make-folder:
	mkdir -p ./build &&\
	mkdir -p ./build/fonts

./build/libskia.a: 
	cd third_party/skia/ &&\
	python2 tools/git-sync-deps &&\
	bin/gn gen out/skia-debug/  --args='cc="clang" cxx="clang++" is_official_build=false skia_use_system_libjpeg_turbo=false skia_use_system_harfbuzz=false skia_use_system_libwebp=false skia_use_system_icu=false skia_use_freetype=true skia_use_system_freetype2=true skia_use_dng_sdk=false' &&\
	ninja -C out/skia-debug/ &&\
	cp ./out/skia-debug/*.a ./../../build/

copy-fonts:
	cp -r ./fonts/* ./build/fonts

example: make-folder copy-fonts ./build/libskia.a 
	cp data.json ./build/ &&\
	cp example.png ./build/ &&\
	cp ./src/opengl/shaders/* ./build/ &&\
	clang++ -g -std=c++17 -Wall -o build/transcoding \
	-lglfw -framework OpenGL \
	./src/*.cpp ./src/opengl/*.cpp \
	./build/*.a $(SKIA_LIBS) \
	 -I./src/ -I./src/opengl/ -I./build -I./third_party/skia/include/core -I./third_party/skia/include/gpu -I./third_party/skia/ \
	 -stdlib=libc++ -DGL_SILENCE_DEPRECATION=1 &&\
	 cd ./build &&\
	 ./transcoding

clean:
	rm -rf ./build/*
