APP     = Stardust
CCFLAGS = -std=c++17 -g -Wall -pedantic -Wno-language-extension-token -Wno-gnu-anonymous-struct -Wno-nested-anon-types
CFALGS  = $(CCFLAGS)
CC      = clang++
C       = clang
MKDIR   = mkdir -p
SRC     = src
OBJ     = obj
INCL_PATH = -isystem "$(VULKAN_SDK)/Include" -Ilibs/stb_image
LIBS_PATH = -L"$(VULKAN_SDK)/Lib"
LIBS    = -luser32 -ld3d11 -ld3dcompiler -ldxguid -lvulkan-1
SHADER  = assets/shaders
DXC = C:\VulkanSDK\1.4.341.1\Bin\dxc.exe

DEFINES = -DSD_DEBUG=1 \
		  -DSD_WIN32=1 \
		  -DSD_D3D11=0 \
		  -DSD_VULKAN=1

SRCS  = $(SRC)/main.cpp \
		$(SRC)/WizardEngine.cpp \
		$(SRC)/StardustEngine.cpp \
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
		$(SRC)/Model.cpp

SHADERS = $(SHADER)/Vertex.hlsl \
	      $(SHADER)/Vertex2D.hlsl \
		  $(SHADER)/Pixel.hlsl

$(APP) : $(SRCS)
	$(CC) $(SRCS) -o $(APP).exe $(CCFLAGS) $(DEFINES) $(INCL_PATH) $(LIBS_PATH) $(LIBS)

sh : $(SHADERS)
	fxc -O3 -T vs_5_0 -E main -Fo $(SHADER)/vert.dxbc $(SHADER)/Vertex.hlsl
	fxc -O3 -T vs_5_0 -E main -Fo $(SHADER)/vert2d.dxbc $(SHADER)/Vertex2D.hlsl
	fxc -O3 -T ps_5_0 -E main -Fo $(SHADER)/frag.dxbc $(SHADER)/Pixel.hlsl
	$(DXC) -spirv -T vs_6_0 -E main -DSPIRV -fspv-target-env=vulkan1.0 -Fo $(SHADER)/vert.spv $(SHADER)/Vertex.hlsl
	$(DXC) -spirv -T vs_6_0 -E main -DSPIRV -fspv-target-env=vulkan1.0 -Fo $(SHADER)/vert2d.spv $(SHADER)/Vertex2D.hlsl
	$(DXC) -spirv -T ps_6_0 -E main -DSPIRV -fspv-target-env=vulkan1.0 -Fo $(SHADER)/frag.spv $(SHADER)/Pixel.hlsl