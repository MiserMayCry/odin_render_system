#pragma once

#ifndef COMMANDCONTEXT_H
#define COMMANDCONTEXT_H

#include <assert.h>
#include <d3d12.h>
#include "base/common_helper.h"

#include "graphics/command_queue.h"
#include "graphics/dynamic_descriptor_heap.h"
#include "graphics/pipeline_state.h"
#include "resource/depth_stencil_buffer.h"
#include "resource/color_buffer.h"
#include "resource/gpu_buffer.h"

#include "linear_allocator.h"
#include "utility.h"

using namespace std;

class GraphicsContext;
class ComputeContext;
class CommandContext;

const int kTypeCount = 4;
const int kBarrierBufferCount = 16;

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

class ContextManager : public Singleton<ContextManager>
{
friend class Singleton<ContextManager>;
 public:
  //ContextManager();

  CommandContext* Allocate(D3D12_COMMAND_LIST_TYPE type);
  void Free(CommandContext* used_context) ;

  //static ContextManager& Instance() {
  //  static ContextManager instance;
  //  return instance;
  //}

 private:
  ContextManager() = default;

 private:
  vector<unique_ptr<CommandContext>> context_pool_[kTypeCount];
  queue<CommandContext*> avalible_contexts_[kTypeCount];

};

struct NonCopyableInterface {
  NonCopyableInterface() = default;
  NonCopyableInterface(const NonCopyableInterface&) = delete;
  NonCopyableInterface& operator=(const NonCopyableInterface&) = delete;
};

class CommandContext : public NonCopyableInterface {

friend class ContextManager;
  
 public:
  ~CommandContext();

  static CommandContext& Begin(const std::wstring& name);
  static void InitializeBuffer(GpuResource& resource, const void* init_data, size_t byte_size, size_t offset = 0);

  ID3D12GraphicsCommandList* GetCommandList() { return command_list_; }

  DynAlloc ReserveUploadMemory(size_t byte_size) {
    return cpu_linear_allocator_.Allocate(byte_size);
  }

  void SetName(const std::wstring& name) { command_list_->SetName(name.c_str()); }

  uint64_t Flush( bool wait_for_signal = false); // CommandQueue* command_queue,
  uint64_t Finish(bool wait_for_signal = false);

  GraphicsContext& GetGraphicsContext() {
    assert(type_ != D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE && "Cannot convert async compute context to graphics");
    return reinterpret_cast<GraphicsContext&>(*this);
  }

  ComputeContext& GetComputeContext() {
    return reinterpret_cast<ComputeContext&>(*this);
  } 

  void CopyBuffer(GpuResource& destination, GpuResource& source);
  //  void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES new_state, D3D12_RESOURCE_STATES old_state);
  void TransitionResource(GpuResource& resource, D3D12_RESOURCE_STATES new_state, bool flush_immediate = false);
  void InsertUavBarrier(GpuResource& resource, bool flush_immediate);
  void FlushResourceBarriers();

  void Initialize();
  void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* descriptor_heap);
  void SetDescriptorHeaps(UINT heap_count, D3D12_DESCRIPTOR_HEAP_TYPE types[], ID3D12DescriptorHeap* descriptor_heaps[]);
 protected:
  void BindDescriptorHeaps();
  
 private:
  CommandContext(D3D12_COMMAND_LIST_TYPE type);

  void Reset();


 protected:
  D3D12_COMMAND_LIST_TYPE type_;
  ID3D12CommandAllocator* command_allocator_;
  ID3D12GraphicsCommandList* command_list_;
  ID3D12PipelineState* graphics_pso_;
  ID3D12RootSignature* graphics_signature_;

  ID3D12PipelineState* compute_pso_;
  ID3D12RootSignature* compute_signature_;

  ContextManager* manager_;

  LinearAllocator cpu_linear_allocator_;
  LinearAllocator gpu_linear_allocator_;

  ID3D12DescriptorHeap* bind_descriptor_heaps_[D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

  UINT barrier_to_flush;
  D3D12_RESOURCE_BARRIER barrier_buffer[kBarrierBufferCount];

  DynamicDescriptorHeap dynamic_view_descriptor_heap_;
  DynamicDescriptorHeap dynamic_sampler_descriptor_heap_;
  
};

class GraphicsContext : public CommandContext {
public:
  static GraphicsContext& Begin(const std::wstring& name);
  void SetViewports(const D3D12_VIEWPORT *viewports, UINT viewport_count = 1);
  void SetScissorRects(const D3D12_RECT *rects, UINT viewport_count = 1);

  void SetConstantBuffer(UINT root_index, D3D12_GPU_VIRTUAL_ADDRESS cbv);
  void SetDescriptorTable(UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle);
  void SetDynamicDescriptors(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

  void SetDynamicConstantBufferView(UINT root_index, UINT buffer_size, const void* data);
  void SetRoot32BitConstant(UINT root_index, UINT src_data, UINT offset);

  void ClearColor(ColorBuffer& target);
  void ClearDepth(DepthStencilBuffer& target);
  void ClearStencil(DepthStencilBuffer& target);
  void ClearDepthStencil(DepthStencilBuffer& target);

  void SetRenderTarget(const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]);
  void SetRenderTargets(UINT rtv_count, const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[], D3D12_CPU_DESCRIPTOR_HANDLE dsv);
  
  void SetRootSignature(const RootSignature& root_signature);
  void SetPipelineState(const GraphicsPso& pso);

  //  void SetDynamicDescriptorHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

  void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
  void SetNullIndexBuffer();
  void SetVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& VBView);
  void SetVertexBuffers(UINT start_slot, UINT count, const D3D12_VERTEX_BUFFER_VIEW vb_views[]);

  void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitive_type);

  void DrawInstanced(UINT vertex_count_per_instance,  UINT instance_count, INT start_vertex_location, UINT start_instance_location);
  void DrawIndexedInstanced(UINT index_count_per_instance, UINT instance_count, UINT start_index_location, 
      INT base_vertex_location, UINT start_instance_location);
  
private:

};

class ComputeContext : public CommandContext {
 public:
  static ComputeContext& Begin(const std::wstring& name, bool is_async = false);

  void SetRootSignature(const RootSignature& root_signature);
  void SetPipelineState(ComputePso& pso);

  void SetDynamicDescriptors(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]);

  void Dispatch(UINT threadgroup_x_count, UINT threadgroup_y_count, UINT threadgroup_z_count);
  void SetRoot32BitConstants(UINT root_index, UINT count, const void* data, UINT offset);
};

inline void CommandContext::CopyBuffer(GpuResource& destination, GpuResource& source) {
  TransitionResource(destination, D3D12_RESOURCE_STATE_COPY_DEST, true);
  TransitionResource(source, D3D12_RESOURCE_STATE_COPY_SOURCE, true);
  FlushResourceBarriers();
  command_list_->CopyResource(destination.GetResource(), source.GetResource());
}

inline void CommandContext::FlushResourceBarriers() {
  if (barrier_to_flush > 0) {
    command_list_->ResourceBarrier(barrier_to_flush, barrier_buffer);
    barrier_to_flush = 0;
  }
}

inline void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* descriptor_heap) {
  if (bind_descriptor_heaps_[type] != descriptor_heap) {
    bind_descriptor_heaps_[type] = descriptor_heap;
  }
  BindDescriptorHeaps();
}

inline void CommandContext::SetDescriptorHeaps(UINT heap_count,
  D3D12_DESCRIPTOR_HEAP_TYPE types[], ID3D12DescriptorHeap* descriptor_heaps[]) {

  bool any_change = false;

  for (UINT i = 0; i < heap_count; ++i) {
    auto type = types[i];
    if (bind_descriptor_heaps_[type] != descriptor_heaps[type]) {
      bind_descriptor_heaps_[type] = descriptor_heaps[type];
      any_change = true;
    }
  }

  if (any_change) {
    BindDescriptorHeaps();
  }
}

inline void GraphicsContext::SetConstantBuffer(UINT root_index, D3D12_GPU_VIRTUAL_ADDRESS cbv) {
  command_list_->SetGraphicsRootConstantBufferView(root_index, cbv);
}

inline void GraphicsContext::SetDescriptorTable(UINT root_index, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
  command_list_->SetGraphicsRootDescriptorTable(root_index, handle);
}

inline void GraphicsContext::SetDynamicDescriptors(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
  //  cpu_descriptor_heap_
  dynamic_view_descriptor_heap_.SetGraphicsDescriptorHandles(root_index, offset, count, handles);
  
}

inline void GraphicsContext::SetDynamicConstantBufferView(UINT root_index, UINT buffer_size, const void* buffer_data) {
  assert(buffer_data != nullptr && Math::IsAligned(buffer_data, 16));
  DynAlloc cb = cpu_linear_allocator_.Allocate(buffer_size);
  memcpy(cb.Data, buffer_data, buffer_size);
  command_list_->SetGraphicsRootConstantBufferView(root_index, cb.GpuAddress);
}

inline void GraphicsContext::SetRoot32BitConstant(UINT root_index, UINT src_data, UINT offset) {
  command_list_->SetGraphicsRoot32BitConstant(root_index, src_data, offset);
}

inline void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& index_buffer_view) {
  command_list_->IASetIndexBuffer(&index_buffer_view);
}

inline void GraphicsContext::SetNullIndexBuffer() {
  command_list_->IASetIndexBuffer(nullptr);
}

inline void GraphicsContext::SetVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& vertex_buffer_view) {
  command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view);
}

inline void GraphicsContext::SetVertexBuffers(UINT start_slot, UINT count, const D3D12_VERTEX_BUFFER_VIEW vb_views[]) {
  command_list_->IASetVertexBuffers(start_slot, count, vb_views);
}

inline void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitive_type) {
  command_list_->IASetPrimitiveTopology(primitive_type);
}

inline void GraphicsContext::SetViewports(const D3D12_VIEWPORT *viewports, UINT viewport_count) {
  command_list_->RSSetViewports(viewport_count, viewports);
}

inline void GraphicsContext::SetScissorRects(const D3D12_RECT *rects, UINT rect_count) {
  command_list_->RSSetScissorRects(rect_count, rects);
}

inline void GraphicsContext::SetRootSignature(const RootSignature& root_signature) {
  if (graphics_signature_ == root_signature.GetSignature()) 
    return;

  graphics_signature_ = root_signature.GetSignature();
  command_list_->SetGraphicsRootSignature(graphics_signature_);
  
  //  dynamic linear parse??
  dynamic_view_descriptor_heap_.ParseGraphicsRootSignature(root_signature);
  dynamic_sampler_descriptor_heap_.ParseGraphicsRootSignature(root_signature);
}

inline void GraphicsContext::SetPipelineState(const GraphicsPso& pso) {
  if (graphics_pso_ == pso.GetPipelineStateObject())
    return;

  graphics_pso_ = pso.GetPipelineStateObject();
  command_list_->SetPipelineState(graphics_pso_);
}

//inline void GraphicsContext::SetDynamicDescriptorHandles(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
//  dynamic_view_descriptor_heap_.SetGraphicsDescriptorHandles(root_index, offset, count, handles);
//}

inline void GraphicsContext::DrawInstanced(UINT vertex_count_per_instance,  UINT instance_count, 
    INT start_vertex_location, UINT start_instance_location) {
  FlushResourceBarriers();
  dynamic_view_descriptor_heap_.CommitGraphicsRootDescriptorTables(command_list_);
  dynamic_sampler_descriptor_heap_.CommitGraphicsRootDescriptorTables(command_list_);
  command_list_->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
}

inline void GraphicsContext::DrawIndexedInstanced(UINT index_count_per_instance, UINT instance_count, 
    UINT start_index_location, INT base_vertex_location, UINT start_instance_location) {

  FlushResourceBarriers();
  dynamic_view_descriptor_heap_.CommitGraphicsRootDescriptorTables(command_list_);
  //  dynamic view descriptor heap  
  dynamic_sampler_descriptor_heap_.CommitGraphicsRootDescriptorTables(command_list_);
  //  dynamic sampler descriptor heap
   
  command_list_->DrawIndexedInstanced(index_count_per_instance, instance_count, 
      start_index_location, base_vertex_location, start_instance_location);
}

inline void ComputeContext::SetRootSignature(const RootSignature& root_signature) {
  if (root_signature.GetSignature() == compute_signature_)
    return;

  compute_signature_ = root_signature.GetSignature();
  command_list_->SetComputeRootSignature(compute_signature_);

  dynamic_view_descriptor_heap_.ParseComputeRootSignature(root_signature);
  dynamic_sampler_descriptor_heap_.ParseComputeRootSignature(root_signature);  
}

inline void ComputeContext::SetPipelineState(ComputePso& pso) {
  if (compute_pso_ == pso.GetPipelineStateObject())
    return;

  compute_pso_ = pso.GetPipelineStateObject();
  command_list_->SetPipelineState(pso.GetPipelineStateObject());
}

inline void ComputeContext::SetDynamicDescriptors(UINT root_index, UINT offset, UINT count, D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
  dynamic_view_descriptor_heap_.SetComputeDescriptorHandles(root_index, offset, count, handles);
}


inline void ComputeContext::Dispatch(UINT threadgroup_x_count, UINT threadgroup_y_count, UINT threadgroup_z_count) {
  FlushResourceBarriers();
  dynamic_view_descriptor_heap_.CommitComputeRootDescriptorTables(command_list_);
  //  dynamic view descriptor heap  
  dynamic_sampler_descriptor_heap_.CommitComputeRootDescriptorTables(command_list_);
  //  dynamic sampler descriptor heap
  command_list_->Dispatch(threadgroup_x_count, threadgroup_y_count, threadgroup_z_count);
}

inline void ComputeContext::SetRoot32BitConstants(UINT root_index, UINT count, const void* data, UINT offset) {
  command_list_->SetComputeRoot32BitConstants(root_index, count, data, offset);
}


#endif









