#include <stdint.h>
#include <string>
using std::string;

void setupOpenGL(int width, int height);

uint32_t get_texture();

void blendFrames(uint32_t texture1ID, uint32_t texture2ID, float blend_ratio);

void getCurrentResults(int width, int height, uint8_t *outputBuffer);

void loadTexture(uint32_t texture_name, int width, int height, const uint8_t *buffer);

void tearDownOpenGL();

uint32_t render_text(string text);

uint32_t render_lottie(double time);
