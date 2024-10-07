#include "render.h"

bool initialized = false; // check if ImGui is initialized
ImGuiIO* io = nullptr; // stored globally to reduce overhead inside renderOverlay

/*
* Initializes the ImGui overlay inside our hook.
* @return S_OK if successful, E_FAIL otherwise
*/
HRESULT __stdcall initOverlay() {
    LOG("Initializing ImGui...");
    initialized = true;

    LOG("Creating ImGui context...");
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->WantCaptureKeyboard = true;
    io->WantCaptureMouse = true;

    HRESULT hr;
    D3DDEVICE_CREATION_PARAMETERS cparams;
    hr = ptrDevice->GetCreationParameters(&cparams);
    if (FAILED(hr)) {
        LOG("Failed to get device creation parameters.");
        return;
    }

    // init backend with correct window handle
    if (!ImGui_ImplWin32_Init(cparams.hFocusWindow)) {
        LOG("ImGui_ImplWin32_Init failed.");
        return;
    }
    if (!ImGui_ImplDX9_Init(ptrDevice)) {
        LOG("ImGui_ImplDX9_Init failed.");
        return;
    }
    LOG("ImGui initialized successfully.");
}

/*
* Called every frame within renderOverlay() for the code behind the ImGui components.
*/
void __stdcall renderContent() {
    ImGui::Begin("example imgui overlay after hooking");
    ImGui::Text("Now hooked using ImGui.");
    if (ImGui::Button("Close")) {
        exit(0);
    }
}


/*
* Called every frame to render the ImGui overlay from inside our hook.
*/
void __stdcall renderOverlay() {

    HRESULT hr;
    if (!initialized) {
		hr = initOverlay();
        if (FAILED(hr)) {
            LOG("Failed to initialize ImGui overlay.");
            return;
        }
        LOG("Initialized ImGui overlay.");
    }

    // this needs to be done every frame in order to have responsive input (e.g. for buttons to work when clicked)
    io->MouseDown[0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    io->MouseDown[1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
    io->MouseDown[2] = GetAsyncKeyState(VK_MBUTTON) & 0x8000;

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	// buttons, text, etc.
    renderContent();

    ImGui::End();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

