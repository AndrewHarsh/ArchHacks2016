/*This source code copyrighted by Lazy Foo' Productions (2004-2015)
and may not be redistributed without written permission.*/

//Using SDL, SDL_image, standard IO, and strings
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include "Network.h"
#include <vector>

using namespace std;

//Screen dimension constants
const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

int dotIndex = 0;
const int dotMaxIndex = 5000;

struct Distance
{
	int ID1;
	int ID2;
	int RSSi;
};

typedef unsigned char byte;

int Serialize(Distance distance, byte* array, int length)
{
	if (length < sizeof(Distance))
	{
		return 0;
	}

	for (int i = 0; i < sizeof(Distance); i++)
	{
		array[i] = ((byte*)(&distance))[i];
	}

	return sizeof(Distance);
}

int Deserialize(byte* array, int length, Distance* distance)
{
	if (length < sizeof(Distance))
	{
		return 0;
	}

	for (int i = 0; i < sizeof(Distance); i++)
	{
		array[i] = ((byte*)(&distance))[i];
	}

	return sizeof(Distance);
}


//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

	//Loads image at specified path
	bool loadFromFile(std::string path);

#ifdef _SDL_TTF_H
	//Creates image from font string
	bool loadFromRenderedText(std::string textureText, SDL_Color textColor);
#endif

	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};

//The application time based timer
class LTimer
{
public:
	//Initializes variables
	LTimer();

	//The various clock actions
	void start();
	void stop();
	void pause();
	void unpause();

	//Gets the timer's time
	Uint32 getTicks();

	//Checks the status of the timer
	bool isStarted();
	bool isPaused();

private:
	//The clock time when the timer started
	Uint32 mStartTicks;

	//The ticks stored when the timer was paused
	Uint32 mPausedTicks;

	//The timer status
	bool mPaused;
	bool mStarted;
};

//The dot that will move around on the screen
class Dot
{
public:
	//The dimensions of the dot
	static const int DOT_WIDTH = 5;
	static const int DOT_HEIGHT = 5;

	//Maximum axis velocity of the dot
	static const int DOT_VEL = 5;

	//Initializes the variables
	Dot();

	//Takes key presses and adjusts the dot's velocity
	void handleEvent(SDL_Event& e);

	//checks for incoming data from beacons
	void computeUserCoordinates(std::vector<std::vector<Distance>> data);

	//Moves the dot
	void move();

	//Shows the dot on the screen
	void render(int x, int y);

	//The X and Y offsets of the dot
	int mPosX, mPosY;

	//The velocity of the dot
	int mVelX, mVelY;
};

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

//Scene textures
LTexture gDotTexture;
LTexture gDotTexture2;

LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

#ifdef _SDL_TTF_H
bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor)
{
	//Get rid of preexisting texture
	free();

	//Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);
	if (textSurface != NULL)
	{
		//Create texture from surface pixels
		mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
		if (mTexture == NULL)
		{
			printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}
	else
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}


	//Return success
	return mTexture != NULL;
}
#endif

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

double RSSiToDistance(int RSSi)
{
	return abs(RSSi);
}

Dot::Dot()
{
	//Initialize the offsets
	mPosX = 0;
	mPosY = 0;

	//Initialize the velocity
	mVelX = 0;
	mVelY = 0;
}

void Dot::handleEvent(SDL_Event& e)
{
	//If a key was pressed
	if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
	{
		//Adjust the velocity
		switch (e.key.keysym.sym)
		{
		case SDLK_UP: mVelY -= DOT_VEL; break;
		case SDLK_DOWN: mVelY += DOT_VEL; break;
		case SDLK_LEFT: mVelX -= DOT_VEL; break;
		case SDLK_RIGHT: mVelX += DOT_VEL; break;
		}
	}
	//If a key was released
	else if (e.type == SDL_KEYUP && e.key.repeat == 0)
	{
		//Adjust the velocity
		switch (e.key.keysym.sym)
		{
		case SDLK_UP: mVelY += DOT_VEL; break;
		case SDLK_DOWN: mVelY -= DOT_VEL; break;
		case SDLK_LEFT: mVelX += DOT_VEL; break;
		case SDLK_RIGHT: mVelX -= DOT_VEL; break;
		}
	}
}

double GetDistance(int B1, int B2, std::vector<std::vector<Distance>> data)
{
	for (int i = 0; i < data.size(); i++)
	{
		for (int ii = 0; ii < data[i].size(); ii++)
		{
			if ((data[i][ii].ID1 == B1 && data[i][ii].ID2 == B2) ||
				(data[i][ii].ID1 == B2 && data[i][ii].ID2 == B1))
			{
				return RSSiToDistance(data[i][ii].RSSi);
			}
		}
	}

	return 0;
}

int g_P1_ID = 0;
int g_BID[9];
int g_B1_ID = 0;
int g_B2_ID = 0;
int g_B3_ID = 0;

double circArr[3][10] = { 0 };
int circIndex = 0;
int frameCount = 0;

double maxNum = 1.0;

void Dot::computeUserCoordinates(std::vector<std::vector<Distance>> data) {
	
	double r1 = 0, r2 = 0, r3 = 0, d = maxNum, pi = maxNum, pj = maxNum;
	int px = 0, py = 0;
	
	if (data.size() >= 1 && data[0].size() >= 1) 
	{
		r1 = GetDistance(g_P1_ID, data[0][g_BID[0]].ID1, data);
		r2 = GetDistance(g_P1_ID, data[0][g_BID[1]].ID1, data);
		r3 = GetDistance(g_P1_ID, data[0][g_BID[2]].ID1, data);

		if (r1 > maxNum)
			maxNum = r1;
		if (r2 > maxNum)
			maxNum = r2;
		if (r3 > maxNum)
			maxNum = r3;

		circArr[0][circIndex] = r1;
		circArr[1][circIndex] = r2;
		circArr[2][circIndex] = r3;

		circIndex++;
		if (circIndex >= 10)
			circIndex = 0;

		r1 = 0;
		r2 = 0;
		r3 = 0;
		for (int i = 0; i < 10; i++) {
			r1 += circArr[0][i];
			r2 += circArr[1][i];
			r3 += circArr[2][i];
		}

		r1 /= 10;
		r2 /= 10;
		r3 /= 10;

		if (frameCount > 30) {
			printf("Distances: %f, %f, %f\n", r1, r2, r3);
			frameCount = 0;
		}
		else {
			frameCount++;
		}

		//triangulate position using trilateration
		double tempx = ((r1*r1) - (r2*r2) + (d*d)) / (2*d);
		double tempy = (((r1*r1) - (r3*r3) + (pi*pi) + (pj*pj)) / (2 * pj)) - ((pi / pj) * tempx);

		//clamp values
		if (tempx > d)
			tempx = d;
		if (tempx < 0)
			tempx = 0;
		if (tempy > pj)
			tempy = pj;
		if (tempy < 0)
			tempy = 0;

		//convert from cartesian coordinate system to screen space
		px = (int)((tempx / d) * SCREEN_WIDTH);
		py = (int)((tempy / pj) * SCREEN_HEIGHT);

		//Move the dot left or right
		mPosX = px;
		//Move the dot up or down
		mPosY = py;
	}
}

void Dot::move()
{
	//Move the dot left or right
	mPosX += mVelX;

	//If the dot went too far to the left or right
	if ((mPosX < 0) || (mPosX + DOT_WIDTH > SCREEN_WIDTH))
	{
		//Move back
		mPosX -= mVelX;
	}

	//Move the dot up or down
	mPosY -= mVelY;

	//If the dot went too far up or down
	if ((mPosY < 0) || (mPosY + DOT_HEIGHT > SCREEN_HEIGHT))
	{
		//Move back
		mPosY += mVelY;
	}
}

void Dot::render(int x, int y)
{
	//Show the dot
	gDotTexture.render(x, SCREEN_HEIGHT - y - DOT_HEIGHT);
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Load dot texture
	if (!gDotTexture.loadFromFile("dot.bmp"))
	{
		printf("Failed to load dot texture!\n");
		success = false;
	}

	if (!gDotTexture2.loadFromFile("dot2.bmp"))
	{
		printf("Failed to load dot texture 2!\n");
		success = false;
	}

	return success;
}

void close()
{
	//Free loaded images
	gDotTexture.free();

	//Destroy window	
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* args[])
{
	Connection_t connection;

	connection.Listen(9000);

	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Load media
		if (!loadMedia())
		{
			printf("Failed to load media!\n");
		}
		else
		{
			//Main loop flag
			bool quit = false;
			bool KeyPressed = false;

			//Event handler
			SDL_Event e;

			//The dot that will be moving around on the screen
			Dot dot;
			int dotX[dotMaxIndex] = { 0 };
			int dotY[dotMaxIndex] = { 0 };

			std::vector<std::vector<Distance>> recievedArr;

			SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
			SDL_RenderClear(gRenderer);

			int counter2 = 0;

			//While application is running
			while (!quit)
			{
				// Get all connected devices
				std::vector<int> ConnectionIndexes = connection.GetIndexes();

				while (recievedArr.size() < ConnectionIndexes.size())
				{ 
					recievedArr.push_back(std::vector<Distance>());
				}

				for (unsigned int i = 0; i < ConnectionIndexes.size(); i++)
				{
					Distance recieved;
					while (connection.Recv(ConnectionIndexes[i], (char*)&recieved, sizeof(Distance)) > 0)
					{
						g_P1_ID = recieved.ID2;

						bool found = false;
						for (unsigned int ii = 0; ii < recievedArr[i].size(); ii++)
						{
							/*if (i == 0)
							{
								g_P1_ID = recieved.ID2;
								if (ii == 0 && g_B1_ID == 0)
								{
									g_B1_ID = recievedArr[i][ii].ID1;
								}
								else if (ii == 1 && g_B2_ID == 0)
								{
									g_B2_ID = recievedArr[i][ii].ID1;
								}
								else if (ii == 2 && g_B3_ID == 0)
								{
									g_B3_ID = recievedArr[i][ii].ID1;
								}

							}*/

							if (recievedArr[i][ii].ID1 == recieved.ID1 &&
								recievedArr[i][ii].ID2 == recieved.ID2)
							{
								found = true;
								recievedArr[i][ii].RSSi = recieved.RSSi;
							}
						}

						if (!found)
						{
							recievedArr[i].push_back(recieved);
						}

						if (counter2 > 30) {
							printf("ConnectionID: %d\n\tID1: %d\n\tID2: %d\n\tRSSi: %d\n", ConnectionIndexes[i], recieved.ID1, recieved.ID2, recieved.RSSi);
							counter2 = 0;
						}
						else {
							counter2++;
						}
					}
				}

				//Handle events on queue
				while (SDL_PollEvent(&e) != 0)
				{
					//User requests quit
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_c)
					{
						SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
						SDL_RenderClear(gRenderer);
						for (int i = 0; i < dotMaxIndex; i++) {
							dotX[i] = dotX[dotMaxIndex - 1];
							dotY[i] = dotY[dotMaxIndex - 1];
						}
					}


					if (e.type == SDL_KEYDOWN && (e.key.keysym.sym >= SDLK_0 || e.key.keysym.sym <= SDLK_9))
					{
						KeyPressed = true;
						g_BID[e.key.keysym.sym - SDLK_0 - 1]++;
						if (g_BID[e.key.keysym.sym - SDLK_0 - 1] >= recievedArr[0].size())
						{
							g_BID[e.key.keysym.sym - SDLK_0 - 1] = 0;
						}
						printf("Beacon %d ID: %d\n", e.key.keysym.sym - SDLK_0 - 1, g_BID[e.key.keysym.sym - SDLK_0 - 1]);
					}
					else
					{
						KeyPressed = false;
					}

					//Handle input for the dot
					dot.handleEvent(e);
				}

				dot.computeUserCoordinates(recievedArr);

				//Move the dot
				dot.move();

				dotX[dotIndex] = dot.mPosX;
				dotY[dotIndex] = dot.mPosY;

				//Clear screen
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
				SDL_RenderClear(gRenderer);

				//Render objects
				for (int i = 0; i < dotMaxIndex; i++) {
					dot.render(dotX[i], dotY[i]);
				}

				if (dotX[dotIndex - 1] != dot.mPosX || dotY[dotIndex - 1] != dot.mPosY) {
					gDotTexture2.render(dotX[dotIndex] - 2.5, SCREEN_HEIGHT - dotY[dotIndex] - 10 + 2.5);
					dotIndex++;
				}

				if (dotIndex >= dotMaxIndex)
					dotIndex = 1;

				gDotTexture2.render(dotX[dotIndex-1] - 2.5, SCREEN_HEIGHT - dotY[dotIndex-1] - 10 + 2.5);

				//Update screen
				SDL_RenderPresent(gRenderer);
			}
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}