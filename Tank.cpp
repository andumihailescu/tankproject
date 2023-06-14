#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include <dinput.h>
#include <Dshow.h>
#include "Camera.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

LPDIRECT3D9             g_pD3D = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;

LPD3DXMESH              g_pMesh = NULL;
D3DMATERIAL9*           g_pMeshMaterials = NULL;
LPDIRECT3DTEXTURE9*     g_pMeshTextures = NULL;
DWORD                   g_dwNumMaterials = 0L;

LPDIRECT3DVERTEXBUFFER9		g_pSkyboxVertexBuffer = NULL;
LPDIRECT3DTEXTURE9			g_pSkyboxTextures[6];

LPDIRECTINPUT8				g_pDin;
LPDIRECTINPUTDEVICE8		g_pDinKeyboard;

LPDIRECTINPUTDEVICE8		g_Mouse;
DIMOUSESTATE				buffered_mouse;
DIMOUSESTATE				mouse_state;

BYTE						g_Keystate[256];

CXCamera* camera;

HWND	hWnd;
IGraphBuilder* pGraph = NULL;
IMediaControl* pControl = NULL;
IMediaEventEx* pEvent = NULL;

float angle = 0;
float x = 0;
float y = -40;
float z = 0;

#define D3DFVF_CVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)
#define   WM_GRAPHNOTIFY    WM_APP + 1

struct CUSTOMVERTEX
{
    float x, y, z;
    float tu, tv;
};

CUSTOMVERTEX g_Skybox[24] =
{
    { -10.0f, -10.0f, 10.0f, 0.0f, 1.0f },
    { -10.0f, 10.0f, 10.0f, 0.0f, 0.0f },
    { 10.0f, -10.0f, 10.0f, 1.0f, 1.0f },
    { 10.0f, 10.0f, 10.0f, 1.0f, 0.0f },

    { 10.0f, -10.0f, -10.0f, 0.0f, 1.0f },
    { 10.0f, 10.0f, -10.0f, 0.0f, 0.0f },
    { -10.0f, -10.0f, -10.0f, 1.0f, 1.0f },
    { -10.0f, 10.0f, -10.0f, 1.0f, 0.0f },

    { -10.0f, -10.0f, -10.0f, 0.0f, 1.0f },
    { -10.0f, 10.0f, -10.0f, 0.0f, 0.0f },
    { -10.0f, -10.0f, 10.0f, 1.0f, 1.0f },
    { -10.0f, 10.0f, 10.0f, 1.0f, 0.0f },

    { 10.0f, -10.0f, 10.0f, 0.0f, 1.0f },
    { 10.0f, 10.0f, 10.0f, 0.0f, 0.0f },
    { 10.0f, -10.0f, -10.0f, 1.0f, 1.0f },
    { 10.0f, 10.0f, -10.0f, 1.0f, 0.0f },

    { -10.0f, 10.0f, 10.0f, 0.0f, 1.0f },
    { -10.0f, 10.0f, -10.0f, 0.0f, 0.0f },
    { 10.0f, 10.0f, 10.0f, 1.0f, 1.0f },
    { 10.0f, 10.0f, -10.0f, 1.0f, 0.0f },

    { -10.0f, -10.0f, -10.0f, 0.0f, 1.0f },
    { -10.0f, -10.0f, 10.0f, 0.0f, 0.0f },
    { 10.0f, -10.0f, -10.0f, 1.0f, 1.0f },
    { 10.0f, -10.0f, 10.0f, 1.0f, 0.0f }
};

HRESULT InitD3D(HWND hWnd)
{
    if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
        return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp, &g_pd3dDevice)))
    {
        if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &d3dpp, &g_pd3dDevice)))
            return E_FAIL;
    }

    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0xffffffff);

    camera = new CXCamera(g_pd3dDevice);


    D3DXVECTOR3 vEyePt(0.0f, -39.0f, 4.0f);
    D3DXVECTOR3 vLookatPt(0.0f, -39.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    camera->LookAtPos(&vEyePt, &vLookatPt, &vUpVec);
    camera->MoveRight(1);
    camera->MoveForward(-4);

    camera->Update();

    return S_OK;
}

HRESULT InitDInput(HINSTANCE hInstance, HWND hWnd)
{

    DirectInput8Create(hInstance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)&g_pDin,
        NULL);

    g_pDin->CreateDevice(GUID_SysKeyboard, &g_pDinKeyboard, NULL);

    g_pDin->CreateDevice(GUID_SysMouse, &g_Mouse, NULL);

    g_Mouse->SetDataFormat(&c_dfDIMouse);

    g_Mouse->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

    g_pDinKeyboard->SetDataFormat(&c_dfDIKeyboard);

    g_pDinKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

    return S_OK;
}

VOID DetectInput()
{
    g_Mouse->Acquire();

    g_Mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouse_state);

    g_pDinKeyboard->Acquire();

    g_pDinKeyboard->GetDeviceState(256, (LPVOID)g_Keystate);
}

void ProccesInput()
{
    if (g_Keystate[DIK_S] & 0x80)
        x -= 0.1;
    if (g_Keystate[DIK_W] & 0x80)
        x += 0.1;
    if (g_Keystate[DIK_D] & 0x80)
        angle += 0.1;
    if (g_Keystate[DIK_A] & 0x80)
        angle -= 0.1;

    if (g_Keystate[DIK_UP] & 0x80)
        camera->MoveForward(0.5);
    if (g_Keystate[DIK_DOWN] & 0x80)
        camera->MoveForward(-0.5);

    if (g_Keystate[DIK_LEFT] & 0x80)
        camera->MoveRight(-0.5);
    if (g_Keystate[DIK_RIGHT] & 0x80)
        camera->MoveRight(0.5);

    if (mouse_state.rgbButtons[0] & 0x80)
        camera->RotateRight(-0.05);
    if (mouse_state.rgbButtons[1] & 0x80)
        camera->RotateRight(0.05);

    if (mouse_state.lZ & 0x80)
        camera->RotateDown(0.05);
    if (-mouse_state.lZ & 0x80)
        camera->RotateDown(-0.05);
}

HRESULT InitGeometry()
{
    LPD3DXBUFFER pD3DXMtrlBuffer;
    HRESULT hRet;

    if (FAILED(D3DXLoadMeshFromX("Tank.x", D3DXMESH_SYSTEMMEM,
        g_pd3dDevice, NULL,
        &pD3DXMtrlBuffer, NULL, &g_dwNumMaterials,
        &g_pMesh)))
    {

        if (FAILED(D3DXLoadMeshFromX("..\\Tank.x", D3DXMESH_SYSTEMMEM,
            g_pd3dDevice, NULL,
            &pD3DXMtrlBuffer, NULL, &g_dwNumMaterials,
            &g_pMesh)))
        {
            MessageBox(NULL, "Could not find Tank.x", "Tank.exe", MB_OK);
            return E_FAIL;
        }
    }

    hRet = D3DXCreateTextureFromFile(g_pd3dDevice,  ("Skybox\\sahara_ft.tga"), &g_pSkyboxTextures[0]);
    hRet |= D3DXCreateTextureFromFile(g_pd3dDevice, ("Skybox\\sahara_bk.tga"), &g_pSkyboxTextures[1]);
    hRet |= D3DXCreateTextureFromFile(g_pd3dDevice, ("Skybox\\sahara_rt.tga"), &g_pSkyboxTextures[2]);
    hRet |= D3DXCreateTextureFromFile(g_pd3dDevice, ("Skybox\\sahara_lf.tga"), &g_pSkyboxTextures[3]);
    hRet |= D3DXCreateTextureFromFile(g_pd3dDevice, ("Skybox\\sahara_up.tga"), &g_pSkyboxTextures[4]);
    hRet |= D3DXCreateTextureFromFile(g_pd3dDevice, ("Skybox\\sahara_dn.tga"), &g_pSkyboxTextures[5]);

    if (FAILED(hRet))
    {
        ::MessageBox(NULL, "Could not open one or more image files", "Error Opening Texture Files", MB_OK | MB_ICONSTOP);
        return false;
    }

    if (FAILED(g_pd3dDevice->CreateVertexBuffer(24 * sizeof(CUSTOMVERTEX),
        0, D3DFVF_CVERTEX,
        D3DPOOL_DEFAULT, &g_pSkyboxVertexBuffer, NULL)))
    {
        return E_FAIL;
    }

    VOID* pVertices;
    if (FAILED(g_pSkyboxVertexBuffer->Lock(0, sizeof(g_Skybox), (void**)&pVertices, 0)))
        return E_FAIL;
    memcpy(pVertices, g_Skybox, sizeof(g_Skybox));
    g_pSkyboxVertexBuffer->Unlock();


    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    g_pMeshMaterials = new D3DMATERIAL9[g_dwNumMaterials];
    g_pMeshTextures = new LPDIRECT3DTEXTURE9[g_dwNumMaterials];

    for (DWORD i = 0; i < g_dwNumMaterials; i++)
    {
        g_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;

        g_pMeshMaterials[i].Ambient = g_pMeshMaterials[i].Diffuse;

        g_pMeshTextures[i] = NULL;
        if (d3dxMaterials[i].pTextureFilename != NULL &&
            lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
        {
            if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
                d3dxMaterials[i].pTextureFilename,
                &g_pMeshTextures[i])))
            {
                const TCHAR* strPrefix = TEXT("..\\");
                const int lenPrefix = lstrlen(strPrefix);
                TCHAR strTexture[MAX_PATH];
                lstrcpyn(strTexture, strPrefix, MAX_PATH);
                lstrcpyn(strTexture + lenPrefix, d3dxMaterials[i].pTextureFilename, MAX_PATH - lenPrefix);

                if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice,
                    strTexture,
                    &g_pMeshTextures[i])))
                {
                    MessageBox(NULL, "Could not find texture map", "Tank.exe", MB_OK);
                }
            }
        }
    }

    pD3DXMtrlBuffer->Release();

    return S_OK;
}

VOID Cleanup()
{
    if (g_pMeshMaterials != NULL)
        delete[] g_pMeshMaterials;

    if (g_pMeshTextures)
    {
        for (DWORD i = 0; i < g_dwNumMaterials; i++)
        {
            if (g_pMeshTextures[i])
                g_pMeshTextures[i]->Release();
        }
        delete[] g_pMeshTextures;
    }
    if (g_pMesh != NULL)
        g_pMesh->Release();

    if (g_pd3dDevice != NULL)
        g_pd3dDevice->Release();

    if (g_pD3D != NULL)
        g_pD3D->Release();
}

VOID SetupMatrices()
{
    /*D3DXMATRIX g_Transform;
    D3DXMatrixIdentity(&g_Transform);
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &g_Transform);
    D3DXVECTOR3 vEyePt(0.0f, 3.0f, -5.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
    g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);*/

    D3DXMATRIX matWorld;
    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 500.0f);
    g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
    //D3DXMatrixTranslation(&matWorld, dx, dy, dz);

    g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

    camera->Update();
}

HRESULT InitDirectShow(HWND hWnd)
{
    CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL,
        CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);

    hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
    hr = pGraph->QueryInterface(IID_IMediaEventEx, (void**)&pEvent);

    hr = pGraph->RenderFile(L"WarSounds.wav", NULL);

    pEvent->SetNotifyWindow((OAHWND)hWnd, WM_GRAPHNOTIFY, 0);

    pControl->Run();


    return S_OK;
}

void HandleGraphEvent()
{
    if (pEvent == NULL)
    {
        return;
    }
    long evCode;
    LONG_PTR param1, param2;

    while (SUCCEEDED(pEvent->GetEvent(&evCode, &param1, &param2, 0)))
    {
        pEvent->FreeEventParams(evCode, param1, param2);
        switch (evCode)
        {
        case EC_COMPLETE:
        case EC_USERABORT:
        case EC_ERRORABORT:
            InitDirectShow(hWnd);
            return;
        }
    }
}

VOID Render()
{
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
        D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

    if (SUCCEEDED(g_pd3dDevice->BeginScene()))
    {
        SetupMatrices();
        HandleGraphEvent();

        D3DXMATRIX g_Transform;
        D3DXMATRIX g_TranslateX;
        D3DXMATRIX g_RotateY;

        D3DXMATRIXA16 matScaleSkybox;
        D3DXMatrixScaling(&matScaleSkybox, 4, 4, 4);
        g_pd3dDevice->SetTransform(D3DTS_WORLD, &matScaleSkybox);

        g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        g_pd3dDevice->SetStreamSource(0, g_pSkyboxVertexBuffer, 0, sizeof(CUSTOMVERTEX));
        g_pd3dDevice->SetFVF(D3DFVF_CVERTEX);

        for (ULONG i = 0; i < 6; ++i)
        {
            g_pd3dDevice->SetTexture(0, g_pSkyboxTextures[i]);

            g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, i * 4, 2);

        }

        D3DXMatrixIdentity(&g_Transform);
        D3DXMatrixIdentity(&g_TranslateX);
        D3DXMatrixIdentity(&g_RotateY);
        D3DXMatrixTranslation(&g_TranslateX, x, -40, 0);
        D3DXMatrixRotationY(&g_RotateY, angle);
        g_Transform = g_TranslateX * g_RotateY;
        g_pd3dDevice->SetTransform(D3DTS_WORLD, &g_Transform);

        g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

        for (DWORD i = 0; i < g_dwNumMaterials; i++)
        {
            g_pd3dDevice->SetMaterial(&g_pMeshMaterials[i]);
            g_pd3dDevice->SetTexture(0, g_pMeshTextures[i]);
            g_pMesh->DrawSubset(i);
        }

        g_pd3dDevice->EndScene();
    }

    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "Main", NULL };
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindow("Main", "Tank Project",
        WS_OVERLAPPEDWINDOW, 100, 100, 700, 700,
        GetDesktopWindow(), NULL, wc.hInstance, NULL);

    if (SUCCEEDED(InitD3D(hWnd)))
    {
        InitDInput(hInst, hWnd);
        if (FAILED(InitDirectShow(hWnd)))
            return 0;

        if (SUCCEEDED(InitGeometry()))
        {
            ShowWindow(hWnd, SW_SHOWDEFAULT);
            UpdateWindow(hWnd);

            MSG msg;
            ZeroMemory(&msg, sizeof(msg));
            while (msg.message != WM_QUIT)
            {
                if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
                {
                    DetectInput();
                    ProccesInput();
                    Render();
                }  
            }
        }
    }

    UnregisterClass("Main", wc.hInstance);
    return 0;
}