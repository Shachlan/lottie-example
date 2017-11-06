#include <inttypes.h>
#include <png.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "openGLShading.hpp"
using std::string;
#include <OpenGL/gl3.h>

unsigned int width;
unsigned int height;
int row_size;
png_bytep *row_pointers;
uint8_t *input_data;
uint8_t *output_data;

// Code copied from
// https://stackoverflow.com/questions/1362945/how-to-decode-a-png-image-to-raw-bytes-from-c-code-with-libpng
static void read_png_file(string filename) {
  FILE *fp = fopen(filename.c_str(), "rb");
  png_byte bit_depth;
  png_byte color_type;
  unsigned int y;

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    abort();
  }
  png_infop info = png_create_info_struct(png);
  if (!info)
    abort();
  if (setjmp(png_jmpbuf(png)))
    abort();
  png_init_io(png, fp);
  png_read_info(png, info);
  width = png_get_image_width(png, info);
  height = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);
  /* Read any color_type into 8bit depth, RGBA format. */
  /* See http://www.libpng.org/pub/png/libpng-manual.txt */
  if (bit_depth == 16)
    png_set_strip_16(png);
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);
  /* PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth. */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);
  if (png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);
  /* These color_type don't have an alpha channel then fill it with 0xff. */
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);
  png_read_update_info(png, info);
  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  row_size = png_get_rowbytes(png, info);
  for (y = 0; y < height; y++) {
    row_pointers[y] = (png_byte *)malloc(row_size);
  }
  input_data = (uint8_t *)malloc(width * height * 4);
  output_data = (uint8_t *)malloc(width * height * 4);
  png_read_image(png, row_pointers);
  for (int i = 0; i < height; i++) {
    auto row = row_pointers[i];
    for (int j = 0; j < row_size; j++) {
      input_data[j + (i * row_size)] = row[j];
    }
  }
  fclose(fp);
}

static void write_png_file(string filename) {
  for (int i = 0; i < height; i++) {
    auto row = row_pointers[i];
    for (int j = 0; j < row_size; j++) {
      row[j] = output_data[j + (i * row_size)];
    }
  }
  FILE *fp = fopen(filename.c_str(), "wb");
  if (!fp)
    abort();
  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png)
    abort();
  png_infop info = png_create_info_struct(png);
  if (!info)
    abort();
  if (setjmp(png_jmpbuf(png)))
    abort();
  png_init_io(png, fp);
  png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);
  /* To remove the alpha channel for PNG_COLOR_TYPE_RGB format, */
  /* Use png_set_filler(). */
  /*png_set_filler(png, 0, PNG_FILLER_AFTER);*/
  png_write_image(png, row_pointers);
  png_write_end(png, NULL);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  read_png_file("example.png");

  setupOpenGL(width, height);

  auto primary_texture = get_texture();

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  double counter = 0.0;
  for (int i = 0 ; i < 10; i ++) {
    loadTexture(primary_texture, width, height, input_data);
    counter += 0.1;
    auto rendered_text = render_lottie(counter);
    // auto rendered_text = render_text("hello world " + std::to_string(counter));

    blendFrames(primary_texture, rendered_text, 0.2);
    getCurrentResults(width, height, output_data);

    write_png_file("result" + std::to_string(i) + ".png");
  }

  for (int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);
  free(input_data);
  free(output_data);

  return 0;
}
