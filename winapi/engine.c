#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <memory.h>
#include <assert.h>

#include <windows.h>
#include <dwmapi.h>

#include "engine.h"
#include "game.h"
#include "stb_easy_font.h"

static HINSTANCE g_hInstance;
static ATOM g_hClass;
static LARGE_INTEGER g_frequency;
static LARGE_INTEGER g_starttime;
static HWND g_hWnd;
static io_t g_io;
static HDC g_hDC;
static HBITMAP g_hBitmap;

static char* assPrintToBufferProc(const char* buf, void* user, int len)
{
	BUFFER* buffer = (BUFFER*)user;
	assResizeBuffer(buffer, 1, len + 1);
	char* data = (char*)buffer->data + buffer->elementCount - 1;
	data[0] = 0;
	return data;
}

float assGsTvAspectRatio()
{
	return 1.0f;
}

float assGetTime()
{
	LARGE_INTEGER time;
	if (!g_frequency.QuadPart) {
		QueryPerformanceFrequency(&g_frequency);
		QueryPerformanceCounter(&g_starttime);
	}
	QueryPerformanceCounter(&time);
	return (float)(time.QuadPart - g_starttime.QuadPart)/ g_frequency.QuadPart;
}

unsigned assGsHeight()
{
	return g_io.height;
}

unsigned assGsWidth()
{
	return g_io.width;
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
char *GetLastErrorAsString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) {
		return NULL; //No error message has been recorded
	}

	LPSTR messageBuffer = NULL;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	char* message = malloc(size);
	memcpy(message, messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

static LRESULT __stdcall MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_GETMINMAXINFO: {
		DefWindowProcA(hWnd, uMsg, wParam, lParam);
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = 512;
		lpMMI->ptMinTrackSize.y = 384;
		break;
	}
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_UP:
			g_io.throttle = 1.0f;
			break;
		case VK_DOWN:
			break;
		case VK_RIGHT:
			g_io.right = true;
			break;
		case VK_LEFT:
			g_io.left = true;
			break;
		case VK_ESCAPE:
			DestroyWindow(hWnd);
			break;
		case VK_RETURN:
			g_io.startPressed = true;
			break;
		case VK_SPACE:
			g_io.fire = true;
			break;
		case 'R':
			g_io.testPressed = true;
			break;
		default:
			return DefWindowProcA(hWnd, uMsg, wParam, lParam);
		}
		break;
	case WM_KEYUP:
		switch (wParam) {
		case VK_UP:
			g_io.throttle = 0;
			break;
		case VK_DOWN:
			break;
		case VK_RIGHT:
			g_io.right = false;
			break;
		case VK_LEFT:
			g_io.left = false;
			break;
		case VK_RETURN:
			g_io.startPressed = false;
			break;
		case VK_SPACE:
			g_io.fire = false;
			break;
		case 'R':
			g_io.testPressed = false;
			break;
		default:
			return DefWindowProcA(hWnd, uMsg, wParam, lParam);
		}
		break;
	case WM_TIMER: {
		float time = assGetTime();
		g_io.deltaTime = time - g_io.time;
		g_io.time = time;
		assGameUpdate(&g_io);
		InvalidateRect(hWnd, NULL, false);
		break;
	}
	case WM_DPICHANGED:
	case WM_SIZE: {
		int w = LOWORD(lParam);
		int h = HIWORD(lParam);
		if (g_io.width != w || g_io.height != h) {
			LOG_DEBUG("Window resized to: %u x %u", w, h);
			if (wParam != SIZE_MINIMIZED) {
				g_io.width = w;
				g_io.height = h;
				g_io.resize = true;
				g_io.resize_image = true;
			}
		}
		return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	}

	//case WM_ENTERSIZEMOVE:
	//	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	//case WM_EXITSIZEMOVE:
	//	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC hDC = BeginPaint(hWnd, &paint);
		if (!g_hDC) {
			g_hDC = CreateCompatibleDC(hDC);
			g_io.resize_image = true;
		}
		if (g_io.resize_image) {
			HBITMAP old = g_hBitmap;
			g_io.resize_image = false;
			g_hBitmap = CreateCompatibleBitmap(hDC, g_io.width, g_io.height);
			SelectObject(g_hDC, g_hBitmap);
			if (old) {
				DeleteObject(old);
			}
		}
		BitBlt(hDC, paint.rcPaint.left, paint.rcPaint.top, paint.rcPaint.right, paint.rcPaint.bottom, g_hDC, paint.rcPaint.left, paint.rcPaint.top, SRCCOPY);
		EndPaint(hWnd, &paint);
		break;
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

void assResizeBuffer(BUFFER* buffer, unsigned elementSize, unsigned elementCount)
{
	unsigned newCapacity = elementSize * elementCount;
	if (newCapacity > buffer->capacity) {
		buffer->data = realloc(buffer->data, newCapacity);
		buffer->capacity = newCapacity;
	}
	buffer->elementSize = elementSize;
	buffer->elementCount = elementCount;
}

void assInitBuffer(BUFFER* buffer, unsigned elementSize, unsigned capacity)
{
	buffer->elementCount = 0;
	buffer->elementSize = elementSize;
	buffer->capacity = elementSize * capacity;
	buffer->data = malloc(buffer->capacity);
}

HWND assCreateWindow(int width, int height, const char *title, WNDPROC lpfnWndProc)
{
	if (!g_hClass) {
		WNDCLASSEXA wndClass = { 0 };
		wndClass.cbSize = sizeof(WNDCLASSEXA);
		wndClass.lpfnWndProc = lpfnWndProc;
		wndClass.hbrBackground = GetStockObject(BLACK_BRUSH);
		wndClass.hInstance = g_hInstance;
		wndClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
		wndClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
		wndClass.lpszClassName = "AssteroidsWindowClass";
		g_hClass = RegisterClassExA(&wndClass);
	}

	return CreateWindowExA(WS_EX_APPWINDOW, g_hClass, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, g_hInstance, NULL);
}

int assMessageBox(HWND hWnd, UINT flags, const char* format, ...)
{
	va_list va;
	BUFFER buffer;
	va_start(va, format);
	assInitBuffer(&buffer, 1, STB_SPRINTF_MIN);
	stbsp_vsprintfcb(&assPrintToBufferProc, &buffer, buffer.data, format, va);
	int result = MessageBoxA(hWnd, buffer.data, "Assteroids", flags);
	free(buffer.data);
	va_end(va);
	return result;
}

typedef struct {
	float x;
	float y;
	float z;
	unsigned char color[4];
} stb_font_vertex_t;

void assGsDrawQuad(const VU_VECTOR* vec, const color_t* color)
{
	POINT p[4];
	int w2 = g_io.width >> 1;
	int h2 = g_io.height >> 1;
	for (int i = 0; i < 4; i++) {
		p[i].x = vec[i].x * w2 + w2;
		p[i].y = vec[i].y * h2 + h2;
	}

	COLORREF col = RGB(color->r * color->a / 255, color->g * color->a / 255, color->b * color->a / 255);
	HPEN pen = CreatePen(PS_SOLID, 1, col);
	HBRUSH brush = CreateSolidBrush(col);
	SelectObject(g_hDC, pen);
	SelectObject(g_hDC, brush);
	Polygon(g_hDC, &p, 4);
	DeleteObject(pen);
	DeleteObject(brush);
}

void assGsDrawText(float x, float y, const char* str, const color_t* color, const VU_MATRIX* m)
{
	// fill color with 1.0f constant to override w component
	const static float one = 1.0f;

	stb_font_vertex_t buffer[4096];
	VU_VECTOR* temp = (VU_VECTOR*)&buffer[0];

	int vertices = stb_easy_font_print(x, -y, (char*)str, (unsigned char*)&one, buffer, sizeof(buffer)) * 4;

	// transform
	for (int i = 0, j = 0; i < vertices; i++) {
		temp[i].y *= -1.0f;
		Vu0ApplyMatrix((VU_MATRIX*)m, temp + i, temp + i);
		if ((i & 3) == 3) {
			assGsDrawQuad(temp + j, color);
			j += 4;
		}
	}
}

void assGsDrawMesh(const mesh_t* mesh, const color_t* color, const VU_MATRIX* m)
{
	const int max_alpha = 127;
	HPEN pen = CreatePen(PS_SOLID, 1, RGB(color->r * color->a / max_alpha, color->g * color->a / max_alpha, color->b * color->a / max_alpha));
	SelectObject(g_hDC, pen);
	for (unsigned i = 0; i < mesh->count - 1; i++) {
		unsigned j = i + 1;
		VU_VECTOR v0;
		VU_VECTOR v1;
		Vu0ApplyMatrix(m, &mesh->data[i], &v0);
		Vu0ApplyMatrix(m, &mesh->data[j], &v1);
		v0.x *= g_io.width >> 1;
		v1.x *= g_io.width >> 1;
		v0.x += g_io.width >> 1;
		v1.x += g_io.width >> 1;
		v0.y *= g_io.height >> 1;
		v1.y *= g_io.height >> 1;
		v0.y += g_io.height >> 1;
		v1.y += g_io.height >> 1;
		MoveToEx(g_hDC, v0.x, v0.y, NULL);
		LineTo(g_hDC, v1.x, v1.y);
	}
	DeleteObject(pen);
}

void assGsClear(const color_t* color)
{
	HBRUSH brush = CreateSolidBrush(RGB(color->r, color->g, color->b));
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = g_io.width;
	rect.bottom = g_io.height;
	FillRect(g_hDC, &rect, brush);
	DeleteObject(brush);
}

void assDebug(const char* format, const char* func, int line, ...)
{
	if (IsDebuggerPresent()) {
		va_list va;
		BUFFER buffer1;
		BUFFER buffer2;
		va_start(va, line);
		assInitBuffer(&buffer1, 1, STB_SPRINTF_MIN);
		assInitBuffer(&buffer2, 1, STB_SPRINTF_MIN);
		stbsp_vsprintfcb(&assPrintToBufferProc, &buffer1, buffer1.data, format, va);
		stbsp_sprintfcb(&assPrintToBufferProc, &buffer2, buffer2.data, "[%.9f] [%s:%d] %s\n", assGetTime(), func, line, buffer1.data);
		OutputDebugStringA(buffer2.data);
		free(buffer1.data);
		free(buffer2.data);
	}
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	g_hWnd = assCreateWindow(640, 480, "Assteroids", &MainWindowProc);
	SetTimer(g_hWnd, 0, 10, NULL);
	MSG message;
	BOOL bRet;
	while (bRet = GetMessageA(&message, NULL, 0, 0)) {
		if (bRet == -1) {
			assMessageBox(g_hWnd, MB_ICONERROR, "Error: %s", GetLastErrorAsString());
			return EXIT_FAILURE;
		} else {
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}

	return EXIT_SUCCESS;
}
