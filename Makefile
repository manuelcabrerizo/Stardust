APP     = Stardust
CCFLAGS = -std=c++17 -g -shared -O0 -Wall -pedantic -Wno-language-extension-token -Wno-gnu-anonymous-struct -Wno-nested-anon-types
CFALGS  = $(CCFLAGS)
CC      = clang++
C       = clang
MKDIR   = mkdir
SRC     = src
INCL_PATH = -isystem "$(VULKAN_SDK)/Include" -Iinclude -Iinclude/math -Ivendor/stb_image
LIBS_PATH = -L"$(VULKAN_SDK)/Lib"
LIBS    = -luser32 -ld3d11 -ld3dcompiler -ldxguid -lvulkan-1

DEFINES =     -DSD_EXPORT  \
              -DSD_DEBUG=1 \
              -DSD_WIN32=1 \
              -DSD_D3D11=0 \
              -DSD_VULKAN=1

SRCS  =       $(SRC)/StardustEngine.cpp \
              $(SRC)/Platform.cpp \
              $(SRC)/Win32Platform.cpp \
              $(SRC)/Renderer.cpp \
              $(SRC)/BatchRenderer.cpp \
              $(SRC)/D3D11Renderer.cpp \
              $(SRC)/D3D11BatchRenderer.cpp \
              $(SRC)/VKRenderer.cpp \
              $(SRC)/VKBatchRenderer.cpp \
              $(SRC)/VKUtils.cpp \
              $(SRC)/Bindable.cpp \
              $(SRC)/VertexBuffer.cpp \
              $(SRC)/IndexBuffer.cpp \
              $(SRC)/GraphicPipeline.cpp \
              $(SRC)/ConstBuffer.cpp \
              $(SRC)/Texture2D.cpp \
              $(SRC)/math/Vector2.cpp \
              $(SRC)/math/Vector3.cpp \
              $(SRC)/math/Vector4.cpp \
              $(SRC)/math/Matrix4x4.cpp \
              $(SRC)/EventBus.cpp \
              $(SRC)/ServiceProvider.cpp \
              $(SRC)/spirv_reflect.cpp \
              $(SRC)/Model.cpp \
              $(SRC)/TextRenderer.cpp \
              $(SRC)/Input.cpp

$(APP) : $(SRCS)
       if not exist lib mkdir lib
       $(CC) $(SRCS) -o lib/$(APP).dll $(CCFLAGS) $(DEFINES) $(INCL_PATH) $(LIBS_PATH) $(LIBS)