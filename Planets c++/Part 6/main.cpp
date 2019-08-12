#include "main.h"
#include <time.h>
#include <list>

GraphicsObject* graphics = new GraphicsObject();

// Global Windows/Drawing variables
HBITMAP hbmp = NULL;
HWND hwnd = NULL;
HDC hdcMem = NULL;
// The window's DC
HDC wndDC = NULL;
HBITMAP hbmOld = NULL;

POINT MousePos = POINT();

vec3 camera = vec3(0x80,0x80,0x80), speed = vec3(), direction = zup::forward, up = zup::up;
miliseconds lastframe = getmiliseconds();
miliseconds mselapsed;
seconds selapsed;
std::vector<planet*> planets;
int Run() 
{
	GenPlanets();
	while (DoEvents())//next frame
	{
		ProcessInput();//process events from user
		// Do stuff with graphics->colors
		Draw();
		// Draw graphics->colors to window
		BitBlt(wndDC, 0, 0, graphics->width, graphics->height, hdcMem, 0, 0, SRCCOPY);
	}
	return 0;
}
const int beginplanetcount = 50;
void DeletePlanets()
{
	for (planet* p:planets) 
	{
		p->Remove();
	}
	planets.clear();
}
void GenPlanets()
{
	if (planets.size())DeletePlanets();
	srand(getmiliseconds());
	for (int i = 0; i < beginplanetcount; i++)
	{
		planets.push_back(planet::randplanet());
	}
}
void Draw() 
{
	graphics->ClearColor(0);
	graphics->ClearDepthBuffer(rendersettings::s3d::maxdistance);
	mat4x4 transform = mat4x4::lookat(camera, camera + direction, up);
	transform = mat4x4::cross(mat4x4::perspectiveFov(90, graphics->width, graphics->height, 0.1, 0x100), transform);
	int maxsize = max(graphics->width, graphics->height);
	static ViewFrustum frustum = ViewFrustum();
	frustum.update(transform);
	for (planet* p : planets) 
	{
		if (frustum.isBoxInFrustum(p->pos - vec3(p->maxradius), vec3(p->maxradius * 2)))
		{
			p->Draw(graphics, transform, 10, 5, camera, direction, maxsize);
		}
	}
}

void ProcessInput()
{
	miliseconds now = getmiliseconds();
	mselapsed = now - lastframe;
	selapsed = getseconds(mselapsed);
	lastframe = now;
	POINT p;
	if (GetCursorPos(&p))
	{
		//cursor position now in p.x and p.y
		if (ScreenToClient(hwnd, &p))
		{
			MousePos = p;
			//p.x and p.y are now relative to hwnd's client area
		}
	}
	const fp rotstep = M_PI * selapsed;//half rotation in a sec
	if (pressed('A'))
	{
		//rotate over up axis(yaw)
		direction = vec3::rotate(direction, up, rotstep);
	}
	if (pressed('D'))
	{
		//rotate over up axis(yaw)
		direction = vec3::rotate(direction, up, -rotstep);
	}
	const vec3 prep = vec3::cross(direction, up);
	if (pressed('W'))
	{
		//rotate over prep axis(pitch)
		direction = vec3::rotate(direction,prep, rotstep);
		up = vec3::rotate(up, prep, rotstep);
	}
	if (pressed('S'))
	{
		//rotate over prep axis(pitch)
		direction = vec3::rotate(direction, prep, -rotstep);
		up = vec3::rotate(up, prep, -rotstep);
	}
	if (pressed('Q'))
	{
		//rotate over forward axis(roll)
		up = vec3::rotate(up, direction, -rotstep);
	}
	if (pressed('E'))
	{
		//rotate over forward axis(roll)
		up = vec3::rotate(up, direction, rotstep);
	}
	const fp step = selapsed;//1 acceleration per sec
	if (pressed(VK_UP))//go forward
	{
		speed += direction * step;
	}
	if (pressed(VK_SPACE))
	{
		speed *= 0.9;
	}
	if (pressed(VK_RETURN))
	{
		GenPlanets();
	}
	camera += speed;
	speed *= .95;
}
void MakeSurface(HWND hwnd) {
	/* Use CreateDIBSection to make a HBITMAP which can be quickly
	 * blitted to a surface while giving 100% fast access to graphics->colors
	 * before blit.
	 */
	 // Desired bitmap properties
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);//sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = graphics->width;
	bmi.bmiHeader.biHeight = -graphics->height; // Order graphics->colors from top to bottom
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // last byte not used, 32 bit for alignment
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;// graphics->width* graphics->height * 4;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	bmi.bmiColors[0].rgbBlue = 0;
	bmi.bmiColors[0].rgbGreen = 0;
	bmi.bmiColors[0].rgbRed = 0;
	bmi.bmiColors[0].rgbReserved = 0;
	HDC hdc = GetDC(hwnd);
	graphics->colors = nullptr;
	// Create DIB section to always give direct access to colors
	hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)& graphics->colors, NULL, 0);
	
	DeleteDC(hdc);
	// Give plenty of time for main thread to finish setting up
	Sleep(50);//time??? without sleep, it finishes
	ShowWindow(hwnd, SW_SHOW);
	// Retrieve the window's DC
	wndDC = GetDC(hwnd);
	// Create DC with shared colors to variable 'graphics->colors'
	hdcMem = CreateCompatibleDC(wndDC);//HDC must be wndDC!! :)
	hbmOld = (HBITMAP)SelectObject(hdcMem, hbmp);

	graphics->depthbuffer = (fp*)calloc(graphics->width * graphics->height, sizeof(fp));
	//set random seed
	srand(getmiliseconds());
}
LRESULT CALLBACK WndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:
	{
		MakeSurface(hwnd);
	}
	break;
	case WM_MOUSEMOVE:
	{
		
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		// Draw graphics->colors to window when window needs repainting
		BitBlt(hdc, 0, 0, graphics->width, graphics->height, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
	}
	break;
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);
	}
	break;
	case WM_DESTROY:
	{
		SelectObject(hdcMem, hbmOld);
		DeleteDC(wndDC);
		PostQuitMessage(0);
	}
	break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	//get global vars
	GetDesktopResolution(graphics->width, graphics->height);
	//graphics->width = 1280;
	//graphics->height = 640;
	
	WNDCLASSEX wc;
	//MSG msg;
	// Init wc
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = CreateSolidBrush(0);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"animation_class";
	wc.lpszMenuName = NULL;
	wc.style = 0;
	// Register wc
	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Failed to register window class.", L"Error", MB_OK);
		return 0;
	}
	// Make window
	hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		L"animation_class",
		L"Animation",
		WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_CAPTION,
		300, 200, graphics->width, graphics->height,
		NULL, NULL, hInstance, NULL);
	RECT rcClient, rcWindow;
	POINT ptDiff;
	// Get window and client sizes
	GetClientRect(hwnd, &rcClient);
	GetWindowRect(hwnd, &rcWindow);
	// Find offset between window size and client size
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	// Resize client
	MoveWindow(hwnd, rcWindow.left, rcWindow.top, graphics->width + ptDiff.x, graphics->height + ptDiff.y, false);
	UpdateWindow(hwnd);

	return Run();
}
