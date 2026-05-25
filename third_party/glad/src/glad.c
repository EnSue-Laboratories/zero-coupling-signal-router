#include "glad/glad.h"

PFNGLGENVERTEXARRAYSPROC glad_glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glad_glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC glad_glDeleteVertexArrays;
PFNGLGENBUFFERSPROC glad_glGenBuffers;
PFNGLBINDBUFFERPROC glad_glBindBuffer;
PFNGLBUFFERDATAPROC glad_glBufferData;
PFNGLBUFFERSUBDATAPROC glad_glBufferSubData;
PFNGLDELETEBUFFERSPROC glad_glDeleteBuffers;
PFNGLCREATESHADERPROC glad_glCreateShader;
PFNGLSHADERSOURCEPROC glad_glShaderSource;
PFNGLCOMPILESHADERPROC glad_glCompileShader;
PFNGLGETSHADERIVPROC glad_glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glad_glGetShaderInfoLog;
PFNGLDELETESHADERPROC glad_glDeleteShader;
PFNGLCREATEPROGRAMPROC glad_glCreateProgram;
PFNGLATTACHSHADERPROC glad_glAttachShader;
PFNGLLINKPROGRAMPROC glad_glLinkProgram;
PFNGLGETPROGRAMIVPROC glad_glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glad_glGetProgramInfoLog;
PFNGLDELETEPROGRAMPROC glad_glDeleteProgram;
PFNGLUSEPROGRAMPROC glad_glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glad_glGetUniformLocation;
PFNGLUNIFORM1IPROC glad_glUniform1i;
PFNGLUNIFORM2FPROC glad_glUniform2f;
PFNGLENABLEVERTEXATTRIBARRAYPROC glad_glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glad_glVertexAttribPointer;
PFNGLACTIVETEXTUREPROC glad_glActiveTexture;
PFNGLGENFRAMEBUFFERSPROC glad_glGenFramebuffers;
PFNGLBINDFRAMEBUFFERPROC glad_glBindFramebuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC glad_glFramebufferTexture2D;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glad_glCheckFramebufferStatus;
PFNGLDELETEFRAMEBUFFERSPROC glad_glDeleteFramebuffers;

typedef union {
    GLADgenericproc generic;
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLBUFFERDATAPROC glBufferData;
    PFNGLBUFFERSUBDATAPROC glBufferSubData;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLUNIFORM1IPROC glUniform1i;
    PFNGLUNIFORM2FPROC glUniform2f;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLACTIVETEXTUREPROC glActiveTexture;
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
} zcsr_glad_proc;

#define ZCSR_LOAD(name) do { zcsr_glad_proc p; p.generic = load(#name); glad_##name = p.name; if (!glad_##name) return 0; } while (0)

int gladLoadGLLoader(GLADloadproc load) {
    if (!load) return 0;
    ZCSR_LOAD(glGenVertexArrays);
    ZCSR_LOAD(glBindVertexArray);
    ZCSR_LOAD(glDeleteVertexArrays);
    ZCSR_LOAD(glGenBuffers);
    ZCSR_LOAD(glBindBuffer);
    ZCSR_LOAD(glBufferData);
    ZCSR_LOAD(glBufferSubData);
    ZCSR_LOAD(glDeleteBuffers);
    ZCSR_LOAD(glCreateShader);
    ZCSR_LOAD(glShaderSource);
    ZCSR_LOAD(glCompileShader);
    ZCSR_LOAD(glGetShaderiv);
    ZCSR_LOAD(glGetShaderInfoLog);
    ZCSR_LOAD(glDeleteShader);
    ZCSR_LOAD(glCreateProgram);
    ZCSR_LOAD(glAttachShader);
    ZCSR_LOAD(glLinkProgram);
    ZCSR_LOAD(glGetProgramiv);
    ZCSR_LOAD(glGetProgramInfoLog);
    ZCSR_LOAD(glDeleteProgram);
    ZCSR_LOAD(glUseProgram);
    ZCSR_LOAD(glGetUniformLocation);
    ZCSR_LOAD(glUniform1i);
    ZCSR_LOAD(glUniform2f);
    ZCSR_LOAD(glEnableVertexAttribArray);
    ZCSR_LOAD(glVertexAttribPointer);
    ZCSR_LOAD(glActiveTexture);
    ZCSR_LOAD(glGenFramebuffers);
    ZCSR_LOAD(glBindFramebuffer);
    ZCSR_LOAD(glFramebufferTexture2D);
    ZCSR_LOAD(glCheckFramebufferStatus);
    ZCSR_LOAD(glDeleteFramebuffers);
    return 1;
}

#undef ZCSR_LOAD
