// OpenGL Grapher
// by Felix An

// Enable visual styles for the program so that the buttons look better
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "exprtk.hpp"

#include <algorithm>
#include <Windows.h>
#include <WinUser.h>
#include <commdlg.h>
#include <shellapi.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glut.h>
#include <string>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <vector>
#include "InputBox.h"

typedef exprtk::symbol_table<double> SymbolTableType;
typedef exprtk::expression<double> ExpressionType;
typedef exprtk::parser<double> ParserType;

bool started = false;

using std::exit;

double calcX;
SymbolTableType symTable;
ParserType parser;

// A class to store the state of the calculator
class CalculatorState
{
public:
	// The function to graph
	std::vector<ExpressionType> functions;
	// Function strings
	std::vector<std::string> functionStrings;
	// The range of the x-axis
	double xMin = 0.0, xMax = 0.0;
	// The range of the y-axis
	double yMin = 0.0, yMax = 0.0;
	// The dimensions of the window
	int width = 0, height = 0;
};

// The global state of the calculator
CalculatorState state;

// Adds a function to the graph.
// Returns true if successful and false otherwise.
bool addFunction(const std::string& fn)
{
	ExpressionType expression;
	expression.register_symbol_table(symTable);
	if (!parser.compile(fn, expression))
		return false;
	state.functions.push_back(expression);
	state.functionStrings.push_back(fn);
	if (started)
		glutPostRedisplay();
	return true;
}

// Deletes the previously added function.
void promptDeleteFunction()
{
	unsigned int choice;
	std::stringstream promptss;
	promptss << "Choose the function you want to delete (enter option number and press OK).\r\n";
	for (size_t i = 0; i < state.functionStrings.size(); ++i)
	{
		promptss << i << ". " << state.functionStrings[i] << "\r\n";
	}
pdStart:
	std::string choiceStr = (InputBox(promptss.str().c_str(), "OpenGLGrapher: Delete Function", ""));
	if (choiceStr.size() == 0)
		return;
	try
	{
		choice = std::stoul(choiceStr);
	}
	catch (const std::invalid_argument&)
	{
		MessageBoxA(GetForegroundWindow(), "You didn't enter an integer! Try again!", "OpenGLGrapher: Invalid Input", MB_ICONWARNING);
		goto pdStart;
	}
	if (choice > state.functionStrings.size())
	{
		MessageBoxA(GetForegroundWindow(), "Invalid choice! Try again!", "OpenGLGrapher: Invalid Input", MB_ICONWARNING);
		goto pdStart;
	}
	state.functions.erase(state.functions.begin() + choice);
	state.functionStrings.erase(state.functionStrings.begin() + choice);
	glutPostRedisplay();
}

// Prompts to add a function
void promptAddFunction()
{
	// Get input from user
addStart:
	std::string userFunctionString = (InputBox("Enter the function you would like to add in terms of x, e.g. \"sqrt(x)\".", "OpenGLGrapher: Add Function", ""));
	if (userFunctionString.size() == 0)
		return;
	for (auto& c : userFunctionString)
	{
		if (c < 32 || c > 127)
		{
			MessageBoxA(NULL, "Non-ASCII characters were entered. Please re-enter the function, and make sure not to enter non-ASCII characters.", "OpenGLGrapher: Invalid Input", MB_ICONHAND);
			goto addStart;
		}
	}

	if (!addFunction(userFunctionString))
	{
		MessageBoxA(NULL, "There is an error with the function you entered. Please try again.", "OpenGLGrapher: Error", MB_ICONHAND);
		goto addStart;
	}
}

// Global variables to store the current position of the graph
double graphX = 0.0;
double graphY = 0.0;

// Mouse x, y
int startX, startY;

void drawGraph();
void drawGrid();
void drawAxes();

// Zoom in
inline void zoomIn()
{
	state.xMin += 2.0;
	state.xMax -= 2.0;
	state.yMin += 2.0;
	state.yMax -= 2.0;
	glutPostRedisplay();
}

// Zoom out
inline void zoomOut()
{
	state.xMin -= 2.0;
	state.xMax += 2.0;
	state.yMin -= 2.0;
	state.yMax += 2.0;
	glutPostRedisplay();
}


// Saves a screenshot of the graphing window as a .bmp file.
void saveScreenshotBmp(const char* filename) {
	HDC hdc = wglGetCurrentDC(); // Get the device context for the current OpenGL rendering context
	HDC hdcScreenshot = CreateCompatibleDC(hdc); // Create a device context to use for printing
	HBITMAP hbmScreenshot = CreateCompatibleBitmap(hdc, state.width, state.height); // Create a bitmap to store the screenshot

	// Select the bitmap into the device context and copy the pixels from the current OpenGL rendering context
	HGDIOBJ hbmOld = SelectObject(hdcScreenshot, hbmScreenshot);
	BitBlt(hdcScreenshot, 0, 0, state.width, state.height, hdc, 0, 0, SRCCOPY);

	// Save the bitmap to a file
	BITMAPFILEHEADER bmfHeader;
	ZeroMemory(&bmfHeader, sizeof(BITMAPFILEHEADER));
	bmfHeader.bfType = 0x4D42; // "BM"
	bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + state.width * state.height * 3;
	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPINFOHEADER bi;
	ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = state.width;
	bi.biHeight = state.height;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwBytesWritten;
		WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);

		// Write the pixel data to the file
		for (int y = state.height - 1; y >= 0; y--) {
			for (int x = 0; x < state.width; x++) {
				COLORREF pixel = GetPixel(hdcScreenshot, x, y);
				BYTE r = (BYTE)pixel;
				BYTE g = (BYTE)(pixel >> 8);
				BYTE b = (BYTE)(pixel >> 16);
				WriteFile(hFile, (LPSTR)&b, 1, &dwBytesWritten, NULL);
				WriteFile(hFile, (LPSTR)&g, 1, &dwBytesWritten, NULL);
				WriteFile(hFile, (LPSTR)&r, 1, &dwBytesWritten, NULL);
			}
		}

		CloseHandle(hFile);
	}

	// Clean up
	SelectObject(hdcScreenshot, hbmOld);
	DeleteObject(hbmScreenshot);
	DeleteDC(hdcScreenshot);

}


// Prompts to save a screenshot.
void promptSaveScreenshot(void)
{
	OPENFILENAMEA ofn;
	char fileName[1024];
	ZeroMemory(&ofn, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetForegroundWindow();
	ofn.lpstrFile = fileName;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(fileName);
	ofn.lpstrFilter = "Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
	ofn.lpstrDefExt = "bmp";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = "OpenGLGrapher: Save Graph";
	ofn.Flags = OFN_OVERWRITEPROMPT;

	GetSaveFileNameA(&ofn);
	if (strlen(ofn.lpstrFile) > 0) saveScreenshotBmp(ofn.lpstrFile);
}


// Motion function
void motionFunc(int x, int y)
{
	// Calculate the change in x and y
	double dx = (double)x - (double)startX;
	double dy = (double)y - (double)startY;

	// Update the start x and y
	startX = x;
	startY = y;

	// Calculate the new range of the x-axis
	double xScale = (state.xMax - state.xMin) / state.width;
	state.xMin -= dx * xScale;
	state.xMax -= dx * xScale;

	// Calculate the new range of the y-axis
	double yScale = (state.yMax - state.yMin) / state.height;
	state.yMin += dy * yScale;
	state.yMax += dy * yScale;

	// Set the projection matrix to an orthographic projection with the updated x and y range
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(state.xMin, state.xMax, state.yMin, state.yMax, -1.0, 1.0);

	// Redisplay the graph
	glutPostRedisplay();
}


// The reshape function
void reshape(int width, int height)
{
	// Update the dimensions of the window
	state.width = width;
	state.height = height;

	// Set the viewport
	glViewport(0, 0, width, height);

	// Set the projection matrix to an orthographic projection with the updated x and y range
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(state.xMin, state.xMax, state.yMin, state.yMax, -1.0, 1.0);
}


// Mouse function
void mouseFunc(int button, int state, int x, int y)
{
	// Update the position of the graph based on the current position when the mouse button is pressed again
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		startX = x;
		startY = y;

		// Redisplay the graph
		glutPostRedisplay();
	}
}


// Keyboard function
void keyboardFunc(unsigned char key, int x, int y)
{
	// Open another graphing calculator when the "n" key is pressed
	if (key == 'n' || key == 'N')
	{
		CHAR pathToProgram[1024];
		GetModuleFileNameA(NULL, pathToProgram, 1024);
		ShellExecuteA(NULL, "open", pathToProgram, NULL, NULL, SW_SHOW);
	}
	// Save screenshot button
	else if (key == 's' || key == 'S')
	{
		promptSaveScreenshot();
	}
	// Add function button
	else if (key == 'a' || key == 'A')
	{
		promptAddFunction();
	}
	// Delete function button
	else if (key == 'd' || key == 'D')
	{
		promptDeleteFunction();
	}
	// Zoom in
	else if (key == '+' || key == '=')
	{
		zoomIn();
	}
	// Zoom out
	else if (key == '-')
	{
		zoomOut();
	}
}


// Special key function
void specialFunc(int key, int x, int y)
{
	if (key == GLUT_KEY_F1)
	{
		// Show the help message in a MessageBox
		HWND hwnd = GetForegroundWindow();
		MessageBoxA(hwnd, "*** HELP ***\r\n\r\nDrag the graph around with your mouse to pan the graph.\r\nPress the + or - key to zoom in or out.\r\nPress N to open a new graph.\r\nPress A to add another function to the current graph.\r\nPress D to delete a function from the current graph.\r\nPress S to save the currently selected graph as a .bmp file.\r\nPress F4 to close the graph.\r\nPress F1 to display this Help message.", "OpenGLGrapher: Help", MB_ICONINFORMATION);
	}
	else if (key == GLUT_KEY_F4)
	{
		// Exit the program
		exit(0);
	}
}


// Initialize the calculator
void init()
{
	// Set the background color to white
	glClearColor(1.0, 1.0, 1.0, 1.0);

	// Set the projection matrix to an orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(state.xMin, state.xMax, state.yMin, state.yMax, -1.0, 1.0);

	// Set the modelview matrix to the identity matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Store the current mouse position
	startX = glutGet(GLUT_WINDOW_X);
	startY = glutGet(GLUT_WINDOW_Y);

	// Mouse function
	glutMouseFunc(mouseFunc);

	// Motion function
	glutMotionFunc(motionFunc);

	// Keyboard function
	glutKeyboardFunc(keyboardFunc);

	// Special key function
	glutSpecialFunc(specialFunc);
}


// Draw the grid
void drawGrid()
{
	// Set the color to a light gray
	glColor3f(0.9, 0.9, 0.9);

	// Draw vertical lines
	for (double x = floor(state.xMin); x <= ceil(state.xMax); x++) {
		glBegin(GL_LINES);
		glVertex2d(x, state.yMin);
		glVertex2d(x, state.yMax);
		glEnd();
	}

	// Draw horizontal lines
	for (double y = floor(state.yMin); y <= ceil(state.yMax); y++) {
		glBegin(GL_LINES);
		glVertex2d(state.xMin, y);
		glVertex2d(state.xMax, y);
		glEnd();
	}
}


// Draw the x-axis and y-axis
void drawAxes()
{
	// Set the color to black
	glColor3f(0.0, 0.0, 0.0);

	// Draw the x-axis
	glBegin(GL_LINES);
	glVertex2d(state.xMin, 0.0);
	glVertex2d(state.xMax, 0.0);
	glEnd();

	// Draw the labels for the x-axis
	for (double x = floor(state.xMin); x <= ceil(state.xMax); x++) {
		// Set the raster position for bitmap rendering
		glRasterPos2f(x, 0.1);
		// Convert the x value to a string with the appropriate precision
		std::stringstream xLabel;
		int xPrecision = std::max(0, static_cast<int>(std::log10(round(x))) - static_cast<int>(std::log10(x)));
		xLabel << std::fixed << std::setprecision(xPrecision) << x;
		// Draw the x label
		for (char c : xLabel.str()) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
		}
	}

	// Draw the y-axis
	glBegin(GL_LINES);
	glVertex2d(0.0, state.yMin);
	glVertex2d(0.0, state.yMax);
	glEnd();

	// Draw the labels for the y-axis
	for (double y = floor(state.yMin); y <= ceil(state.yMax); y++) {
		// Set the raster position for bitmap rendering
		glRasterPos2f(0.0, y + 0.1);
		// Convert the y value to a string with the appropriate precision
		std::stringstream yLabel;
		int yPrecision = std::max(0, static_cast<int>(std::log10(round(y))) - static_cast<int>(std::log10(y)));
		yLabel << std::fixed << std::setprecision(yPrecision) << y;
		// Draw the y label
		for (char c : yLabel.str()) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
		}
	}
}


// Draw the graph of the function
void drawGraph()
{
	// Set the color to red
	glColor3f(1.0, 0.0, 0.0);

	// Iterate through all the functions
	for (auto& function : state.functions)
	{
		// Iterate over a set of x values
		for (double x = state.xMin; x <= state.xMax; x += 0.01) {
			double y;
			// Evaluate the function at x
			calcX = x;
			y = function.value();

			// Check if the function is defined at x
			if (std::isnan(y) || std::isinf(y)) {
				continue;
			}

			// Draw a line segment from (x, y) to (x + 0.01, f(x + 0.01))
			glBegin(GL_LINES);
			glVertex2d(x, y);
			calcX = x + 0.01;
			y = function.value();
			glVertex2d(x + 0.01, y);
			glEnd();
		}
	}
}

// The display function
void display()
{
	// Clear the window
	glClear(GL_COLOR_BUFFER_BIT);

	// Set the projection matrix to an orthographic projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(state.xMin, state.xMax, state.yMin, state.yMax, -1.0, 1.0);

	// Set the modelview matrix to the identity matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Draw the grid
	drawGrid();

	// Draw the x-axis and y-axis
	drawAxes();

	// Draw the graph of the function
	drawGraph();

	// Swap the front and back buffers
	glutSwapBuffers();
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	symTable.add_variable("x", calcX);

	// Get input from user
enterStart:
	std::string userFunctionString = (InputBox("Welcome to the OpenGLGrapher.\r\n\r\nPlease enter the function you would like to graph in terms of x, e.g. \"sqrt(x)\", and click OK or press Enter.\r\nClick Cancel to exit.\r\n\r\nOnce the graph is shown:\r\nDrag the graph to pan it around.\r\nPress the + or - key to zoom in or out.\r\nPress N when the graphing window appears to open another graph.\r\nPress A to add another function to the current graph.\r\nPress D to delete a function from the current graph.\r\nPress S to save the currently selected graph as a .bmp file.\r\nPress F4 to close the graph.\r\nPress F1 to display Help.", "OpenGLGrapher: Enter Function", ""));
	if (userFunctionString.size() == 0)
		return 0;
	for (auto &c : userFunctionString)
	{
		if (c < 32 || c > 127)
		{
			MessageBoxA(NULL, "Non-ASCII characters were entered. Please re-enter the function, and make sure not to enter non-ASCII characters.", "OpenGLGrapher: Invalid Input", MB_ICONHAND);
			goto enterStart;
		}
	}

	if (!addFunction(userFunctionString))
	{
		MessageBoxA(NULL, "There is an error with the function you entered. Please try again.", "OpenGLGrapher: Error", MB_ICONHAND);
		goto enterStart;
	}

	// Initialize the calculator state
	state.xMin = -10.0;
	state.xMax = 10.0;
	state.yMin = -10.0;
	state.yMax = 10.0;
	state.width = 700;
	state.height = 700;


	// Initialize GLUT
	glutInit(&__argc, __argv);

	// Set up the display mode
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

	// Set the window size and position
	glutInitWindowSize(state.width, state.height);
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - 700) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - 700) / 2);

	// Create the window
	glutCreateWindow("OpenGLGrapher");

	// Set the display function
	glutDisplayFunc(display);

	// Set the reshape function
	glutReshapeFunc(reshape);

	// Initialize the calculator
	init();

	started = true;

	// Enter the main loop
	glutMainLoop();

	return 0;
}
