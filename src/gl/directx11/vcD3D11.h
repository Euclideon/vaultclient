#ifndef vcOpenGL_h__
#define vcOpenGL_h__

#include "udPlatform/udPlatform.h"

#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#ifndef VERIFY_GL
#ifdef _DEBUG
//#define VERIFY_GL() if (int glError = glGetError()) { /*cuDebugLog("GLError %d: (%s)\n", glError, msg);*/ __debugbreak(); }
#define VERIFY_GL()
#else
#define VERIFY_GL()
#endif
#endif //VERIFY_GL

#include "gl/vcGLState.h"
#include "gl/vcTexture.h"

struct vcTexture
{
  ID3D11SamplerState *pSampler;
  ID3D11ShaderResourceView *pTextureView;
  ID3D11Texture2D *pTextureD3D;

  DXGI_FORMAT d3dFormat;

  bool isDynamic;
  bool isRenderTarget;

  vcTextureFormat format;
  int width, height;
};

struct vcFramebuffer
{
  ID3D11RenderTargetView *pRenderTargetView;
  ID3D11DepthStencilView *pDepthStencilView;
};

struct vcShaderConstantBuffer
{
  int type; // 0 = VSConstantBuffer, 1 = PSConstantBuffer
  int expectedSize;
  uint32_t registerSlot;

  ID3D11Buffer *pBuffer;
  char bufferName[32];
};

struct vcShader
{
  ID3D11VertexShader *pVertexShader;
  ID3D11PixelShader* pPixelShader;

  ID3D11InputLayout *pLayout;

  vcShaderConstantBuffer bufferObjects[16];
  int numBufferObjects;
};

struct vcShaderSampler
{
  //GLuint id;
};

struct vcMesh
{
  ID3D11Buffer *pVBO;
  ID3D11Buffer *pIBO;

  ID3D11InputLayout *pInputLayout;
  ID3D11Buffer *pVertexConstantBuffer;

  uint32_t vertexCount;
  uint32_t maxVertexCount;
  uint32_t indexCount;
  uint32_t maxIndexCount;

  D3D11_USAGE drawType;
  int indexBytes;
  uint32_t vertexSize;
};

static const D3D11_FILTER vcTFMToD3D[] = { D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_FILTER_MIN_MAG_MIP_LINEAR }; // Filter Mode
UDCOMPILEASSERT(udLengthOf(vcTFMToD3D) == vcTFM_Total, "TextureFilterModes not equal size");

static const D3D11_TEXTURE_ADDRESS_MODE vcTWMToD3D[] = { D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_CLAMP }; // Wrap Mode
UDCOMPILEASSERT(udLengthOf(vcTWMToD3D) == vcTFM_Total, "TextureFilterModes not equal size");

extern ID3D11Device *g_pd3dDevice;
extern ID3D11DeviceContext *g_pd3dDeviceContext;

#endif // vcOpenGL_h__
