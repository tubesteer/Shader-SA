#include <mod/amlmod.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

MYMOD(net.shader.dump, ShaderDump, 1.0, YourName);

static void (*orig_glShaderSource)(
    GLuint shader,
    GLsizei count,
    const GLchar* const* string,
    const GLint* length
);

static int gShaderNum = 0;

static void SaveShader(const char* src)
{
    if(src == nullptr) return;

    mkdir("/sdcard/ShaderDump", 0777);

    char path[256];
    sprintf(
        path,
        "/sdcard/ShaderDump/shader_%05d.glsl",
        gShaderNum++
    );

    FILE* fp = fopen(path, "wb");

    if(fp)
    {
        fwrite(src, 1, strlen(src), fp);
        fclose(fp);
    }
}

static void Hooked_glShaderSource(
    GLuint shader,
    GLsizei count,
    const GLchar* const* string,
    const GLint* length
)
{
    if(string && count > 0)
    {
        const char* src = string[0];

        if(src)
        {
            size_t len = strlen(src);

            // Hindari dump shader kecil/aneh
            if(len > 100)
            {
                SaveShader(src);
            }
        }
    }

    orig_glShaderSource(
        shader,
        count,
        string,
        length
    );
}

extern "C" void OnModLoad()
{
    void* gles = dlopen("libGLESv2.so", RTLD_NOW);

    if(!gles)
    {
        logger->Error("Failed to load libGLESv2.so");
        return;
    }

    void* sym = dlsym(gles, "glShaderSource");

    if(!sym)
    {
        logger->Error("Failed to find glShaderSource");
        return;
    }

    aml->Hook(
        sym,
        (void*)Hooked_glShaderSource,
        (void**)&orig_glShaderSource
    );

    logger->Info("glShaderSource hooked!");
}
