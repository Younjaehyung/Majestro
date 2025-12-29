#pragma once

class ImGuiManager
{
private:
    ImGuiManager() = default;
	~ImGuiManager() = default;


public:

    static ImGuiManager& Get()
    {
        static ImGuiManager instance;
        return instance;
	}

    void Initialize(
        HWND hwnd,
        ID3D12Device* device,
        DXGI_FORMAT rtvFormat,
        ID3D12DescriptorHeap* srvHeap,
		ID3D12CommandQueue* commandQueue
    );

    void BeginFrame();
    void EndFrame(ID3D12GraphicsCommandList* cmd, ID3D12DescriptorHeap* srvHeap);
	void Render();
    void Shutdown();
	void DemoWindow();



};
