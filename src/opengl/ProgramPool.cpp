// Copyright (c) 2019 Lightricks. All rights reserved.
// Created by Shachar Langbeheim.

#include "ProgramPool.hpp"

#include <OpenGL/gl3.h>
#include <fstream>
#include <sstream>
#include <vector>

#include "GLException.hpp"

using namespace WREOpenGL;

void ProgramPool::delete_program(GLuint program_name) {
  auto shaders = this->name_to_shader_names_mapping[program_name];
  auto description = this->name_to_description_mapping[program_name];

  glDeleteProgram(program_name);
  glDeleteShader(shaders.first);
  glDeleteShader(shaders.second);
}

void ProgramPool::flush() {
  std::vector<GLuint> names_to_remove;
  for (auto &pair : program_reference_count) {
    if (pair.second > 0) {
      continue;
    }
    delete_program(pair.first);
    names_to_remove.insert(names_to_remove.end(), pair.first);
  }

  for (auto &name : names_to_remove) {
    auto description = this->name_to_description_mapping[name];
    this->description_to_name_mapping.erase(description);
    this->name_to_shader_names_mapping.erase(name);
    this->name_to_description_mapping.erase(name);
    this->program_reference_count.erase(name);
  }
}

void ProgramPool::clear() {
  for (auto &pair : name_to_description_mapping) {
    delete_program(pair.first);
  }

  this->description_to_name_mapping.clear();
  this->name_to_shader_names_mapping.clear();
  this->name_to_description_mapping.clear();
  this->program_reference_count.clear();
}

static GLuint build_shader(const GLchar *shader_source, GLenum shader_type) {
  GLuint shader = glCreateShader(shader_type);
  if (!shader || !glIsShader(shader)) {
    return 0;
  }
  glShaderSource(shader, 1, &shader_source, 0);
  glCompileShader(shader);
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE) {
    return shader;
  }
  GLint logSize = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
  GLchar *errorLog = (GLchar *)calloc(logSize, sizeof(GLchar));
  glGetShaderInfoLog(shader, logSize, 0, errorLog);
  throw_gl_exception("Failed to build shader with error:\n%s", errorLog);
  return 0;
}

static string get_shader_filename(string shader, GLenum shader_type) {
  return shader + (shader_type == GL_VERTEX_SHADER ? ".vsh" : ".fsh");
}

static string get_shader_text(string shader_name, GLenum shader_type) {
  auto shader_file_name = get_shader_filename(shader_name, shader_type);
  std::ifstream stream(shader_file_name);
  if (!stream.is_open()) {
    throw_gl_exception("Can't open shader file %s", shader_file_name.c_str());
  }
  std::stringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

int build_shader(string shader_name, GLenum shader_type) {
  auto text = get_shader_text(shader_name, shader_type);

  return build_shader(text.c_str(), shader_type);
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
  GLuint program = glCreateProgram();
  GLCheckDbg("create program");
  glAttachShader(program, vertex_shader);
  GLCheckDbg("attach vertex shader");
  glAttachShader(program, fragment_shader);
  GLCheckDbg("attach fragment shader");
  glLinkProgram(program);
  GLCheckDbg("linking program");

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status != GL_TRUE) {
    GLint maxLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

    GLchar infoLog[maxLength];
    glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
    throw_gl_exception("Failed to create program with error:\n%s", infoLog);
  }
  return program;
}

string get_key(string vertex_shader, string fragment_shader) {
  return "vertx:" + vertex_shader + ".fragment:" + fragment_shader;
}

GLuint ProgramPool::get_program(string vertex_shader, string fragment_shader) {
  auto key = get_key(vertex_shader, fragment_shader);
  auto search = this->description_to_name_mapping.find(key);
  if (search != this->description_to_name_mapping.end()) {
    this->program_reference_count[search->second]++;
    return search->second;
  }

  auto v_shader_name = build_shader(vertex_shader, GL_VERTEX_SHADER);
  auto f_shader_name = build_shader(fragment_shader, GL_FRAGMENT_SHADER);
  auto program = create_program(v_shader_name, f_shader_name);

  this->description_to_name_mapping[key] = program;
  this->name_to_shader_names_mapping[program] = std::pair<int, int>{v_shader_name, f_shader_name};
  this->name_to_description_mapping[program] = key;
  this->program_reference_count[program] = 1;

  return program;
}

void ProgramPool::release_program(GLuint program_name) {
  program_reference_count[program_name]--;
}
