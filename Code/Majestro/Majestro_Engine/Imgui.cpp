#include "pch.h"
#include "Imgui.h"


void ImGuiManager::Initialize(HWND hwnd,
    ID3D12Device* device,
    DXGI_FORMAT rtvFormat,
    ID3D12DescriptorHeap* srvHeap)
{


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplWin32_Init(hwnd);

    const int FrameCount = 2; // 2 or 3

    UINT descriptorSize =
        device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );

    D3D12_CPU_DESCRIPTOR_HANDLE imguiCpuHandle =
        srvHeap->GetCPUDescriptorHandleForHeapStart();
    imguiCpuHandle.ptr += IMGUI_INDEX_START * descriptorSize;

    D3D12_GPU_DESCRIPTOR_HANDLE imguiGpuHandle =
        srvHeap->GetGPUDescriptorHandleForHeapStart();
    imguiGpuHandle.ptr += IMGUI_INDEX_START * descriptorSize;



    ImGui_ImplDX12_Init(
        device,
        FrameCount, //FrameCount
        rtvFormat,
        srvHeap,
        imguiCpuHandle,
        imguiGpuHandle
    );


    //ImGui_ImplWin32_EnableDpiAwareness();
    //float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    //// Setup scaling
    //ImGuiStyle& style = ImGui::GetStyle();
    //style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    //style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)


    //ImGui::StyleColorsDark();

}

void ImGuiManager::BeginFrame()
{

    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::Render()
{
    DemoWindow();
    ImGui::Render();

}


void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* cmd, ID3D12DescriptorHeap* srvHeap)
{
    cmd->SetDescriptorHeaps(1, &srvHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void ImGuiManager::Shutdown()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiManager::DemoWindow()
{
	ImGui::Begin("Demo Window");
	ImGui::Text("Hello from ImGui!");
	ImGui::End();
}
