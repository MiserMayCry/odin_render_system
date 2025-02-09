#pragma once

#include "graphics/root_signature.h"
#include "utility.h"

using namespace Microsoft::WRL;

class PSO
{
 public:
  PSO() : pso_(nullptr) {};

  void SetRootSignature(const RootSignature& signature) {
    root_signature_ = &signature;
  }

  const RootSignature& GetRootSignature() const {
    return *root_signature_;
  }

  ID3D12PipelineState* GetPipelineStateObject(void) const { return pso_.Get(); }

protected:
  const RootSignature* root_signature_;
  ComPtr<ID3D12PipelineState> pso_;
};

class GraphicsPso : public PSO {

 public:
  GraphicsPso();
  
  void CopyDesc(const GraphicsPso& rhs);

  void SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* descs, UINT element_count);
  void SetVertexShader(const BYTE* code_buffer, UINT size);
  void SetVertexShader(ComPtr<ID3DBlob>& vertex_shader);

  void SetPixelShader(const BYTE* code_buffer, UINT size);
  void SetPixelShader(ComPtr<ID3DBlob>& pixel_shader);

  void SetRasterizeState(const D3D12_RASTERIZER_DESC& rasterizer);
  void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& dss);
  void SetBlendState(const D3D12_BLEND_DESC& blend);
  void SetPrimitiveTopologyType(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topology_type);
  void SetSampleMask(UINT mask);
  void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
  void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);

  void Finalize();

 private:
   D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc_;
   std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> input_layouts_;
};

class ComputePso : public PSO {
 public:
  ComputePso();
  void Finalize();

  void SetComputeShader(const BYTE* code_buffer, UINT size);
  void SetComputeShader(ComPtr<ID3DBlob>& pixel_shader);
  void SetRootSig(ID3D12RootSignature* rootSig) {
    root_sig_ = rootSig;
  }
 
 private:
  D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc_;
  ID3D12RootSignature* root_sig_;
};

inline void GraphicsPso::SetVertexShader(const BYTE* code_buffer, UINT size) {
  pso_desc_.VS = {const_cast<BYTE*>(code_buffer), size};
}

inline void GraphicsPso::SetVertexShader(ComPtr<ID3DBlob>& vertex_shader) {
  pso_desc_.VS ={   reinterpret_cast<BYTE*>(vertex_shader->GetBufferPointer()),
      (UINT)vertex_shader->GetBufferSize() };
}

inline void GraphicsPso::SetPixelShader(const BYTE* code_buffer, UINT size) {
  pso_desc_.PS = {const_cast<BYTE*>(code_buffer), size};
}

inline void GraphicsPso::SetPixelShader(ComPtr<ID3DBlob>& pixel_shader) {
  pso_desc_.PS = { reinterpret_cast<BYTE*>(pixel_shader->GetBufferPointer()),
      (UINT)pixel_shader->GetBufferSize() };
}

inline void GraphicsPso::SetRasterizeState(const D3D12_RASTERIZER_DESC& rasterizer) {
  pso_desc_.RasterizerState = rasterizer;
}

inline void GraphicsPso::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& dss) {
  pso_desc_.DepthStencilState = dss;
}

inline void GraphicsPso::SetBlendState(const D3D12_BLEND_DESC& blend) {
  pso_desc_.BlendState = blend;
}

inline void GraphicsPso::SetPrimitiveTopologyType(const D3D12_PRIMITIVE_TOPOLOGY_TYPE& topology_type) {
  pso_desc_.PrimitiveTopologyType = topology_type;
}
inline void GraphicsPso::SetSampleMask(UINT mask) {
  pso_desc_.SampleMask = mask;
}

inline void GraphicsPso::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, 
    DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality) {

  SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

inline void ComputePso::SetComputeShader(ComPtr<ID3DBlob>& pixel_shader) {
  pso_desc_.CS = { reinterpret_cast<BYTE*>(pixel_shader->GetBufferPointer()),
       (UINT)pixel_shader->GetBufferSize() };
}

inline void ComputePso::SetComputeShader(const BYTE* code_buffer, UINT size) {
  pso_desc_.CS = {const_cast<BYTE*>(code_buffer), size};
}



