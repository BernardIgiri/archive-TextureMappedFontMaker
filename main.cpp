#define WIN32_LEAN_AND_MEAN		// trim the excess fat from Windows

#define VK_Z 0x5a
#define VK_X 0x58
#define VK_C 0x43
#define VK_V 0x56

/*******************************************************************
*	Program: Chapter 12 Bitmap Font Example 1
*	Author: Kevin Hawkins
*	Description: Renders bitmap font text in the center of the
*			   window.
********************************************************************/

////// Includes
#include <windows.h>			// standard Windows app include
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gl/gl.h>				// standard OpenGL include
#include <gl/glu.h>				// OpenGL utilties
#include <gl/glaux.h>
#include <stdio.h>
#include <string.h>
// strcpy

#include "LoadImageData.h"
#include "fontStuff.h"
#include "log.h"

#define THEHEIGHT 10
#define THEWIDTH 10

CTypeFace typeFace;

////// Global Variables
HDC g_HDC;						// global device context
bool fullScreen = true;		// true = fullscreen; false = windowed
bool keyPressed[256]={0};			// holds true for keys that are pressed	
bool oldKeyPressed[256]={0};
int g_scrnWidth=800,
	g_scrnHeight=600,
	g_scrnBpp=32;
char g_fontName[1024]={0};
GLuint texID, tex2ID, testList;

unsigned char *pDLImage = NULL;

unsigned int listBase;				// display list base
CLOG cLog("log.txt");

unsigned int CreateBitmapFont(char *fontName, int fontSize,bool isBold,bool isItalic,bool isSymbol=false)
{
	HFONT hFont;         // windows font
	unsigned int base;

	base = glGenLists(96);      // create storage for 96 characters

	hFont = CreateFont(fontSize, 0, 0, 0, (isBold ? FW_BOLD : FW_NORMAL),
					   (isItalic ? TRUE : FALSE), FALSE, FALSE,
					   (isSymbol ? SYMBOL_CHARSET : ANSI_CHARSET), 
					   OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
					   FF_DONTCARE | DEFAULT_PITCH, fontName);

	if (!hFont)
		return 0;

	SelectObject(g_HDC, hFont);
	wglUseFontBitmaps(g_HDC, 32, 96, base);

	return base;
}

void ClearFont(unsigned int base)
{
	if (base != 0)
		glDeleteLists(base, 96);
}

void PrintString(unsigned int base, char *str)
{
	if ((base == 0) || (str == NULL))
		return;

	glPushAttrib(GL_LIST_BIT);
		glListBase(base - 32);
		glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	glPopAttrib();
}


void CleanUp()
{
	ClearFont(listBase);
	glDeleteTextures(1,&texID);
	delete [] pDLImage;
	pDLImage = NULL;
}

void ChangeColor(int texture,float r,float g,float b)
{
	CBMI bitmap;
	bitmap.m_data = NULL;
	glBindTexture(GL_TEXTURE_2D,texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&bitmap.m_width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&bitmap.m_height);
	bitmap.m_bpp = 32;
	bitmap.m_size = bitmap.m_width*bitmap.m_height*(bitmap.m_bpp/8);
	bitmap.m_data = (unsigned char*)malloc(bitmap.m_size);
	if (!bitmap.m_data)
		return;
	glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,bitmap.m_data);
	// modify data
	int i=bitmap.m_size;
	/*unsigned char ir=(int)(255.0f*r),
				  ig=(int)(255.0f*g),
				  ib=(int)(255.0f*b);*/
	while (i)
	{
		i-=4; // assume m_bpp=32
		*(bitmap.m_data+i+0)=(int)((float)*(bitmap.m_data+i+0) * r);
		*(bitmap.m_data+i+1)=(int)((float)*(bitmap.m_data+i+1) * g);
		*(bitmap.m_data+i+2)=(int)((float)*(bitmap.m_data+i+2) * b);
	}

	// apply mods
	/*glTexSubImage2D(GL_TEXTURE_2D,0,0,0,bitmap.m_width,bitmap.m_height,
		GL_RGBA,GL_UNSIGNED_BYTE,bitmap.m_data);*/
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,bitmap.m_width,bitmap.m_height,GL_RGBA,GL_UNSIGNED_BYTE,bitmap.m_data);
	free(bitmap.m_data);
	bitmap.m_data = NULL;
}

void SetFontName(const char *font,int size,bool isBold,bool isItalic,bool isSymbol=false)
{
	strcpy(g_fontName,font);
	sprintf(g_fontName,"%s%i",g_fontName,size);
	if (isBold)
		strcat(g_fontName,"Bold");
	if (isItalic)
		strcat(g_fontName,"Italic");
	if (isSymbol)
		strcat(g_fontName,"Symbol");
}

bool SaveFont(const char *path,const char *fontName,const CTypeFace *pFont)
{
	char fPath[1024]={0};
	strcpy(fPath,path);
	strcat(fPath,fontName);
	strcat(fPath,".font");
	FILE *fptr=fopen(fPath,"wb");
	if (!fptr)
		return false; // failed to create file
	bool result = fwrite(pFont,sizeof(CTypeFace),1,fptr)!=NULL;
	fclose(fptr);
	return result;
}

bool LoadFont(const char *path,const char *fontName,CTypeFace *pFont)
{
	char fPath[1024]={0};
	strcpy(fPath,path);
	strcat(fPath,fontName);
	strcat(fPath,".font");
	FILE *fptr=fopen(fPath,"rb");
	if (!fptr)
		return false; // failed to open file
	bool result = fread(pFont,sizeof(CTypeFace),1,fptr)!=NULL;
	fclose(fptr);
	return result;
}

inline unsigned char *ScrollImageData(int x,int y,int width,int height,int bpp,unsigned char *data)
{
	bpp/=8; // bpp is now bytes/pixel
	return data +(x*bpp)+(y*bpp)*width;
}

unsigned char *CreateGLLuminanceAlpha(unsigned size,int bpp,const unsigned char *data,unsigned char minVal=0)
{
	if (bpp==16)
		return NULL; // 16 bit color is not supported
	bpp/=8; // bpp is now bytes/pixel
	unsigned outDataSize = 2*(size/bpp);
	unsigned char *output = (unsigned char *)malloc(outDataSize);
	if (output==NULL)
		return NULL;
	int inDataIdx = size,
		outDataIdx = outDataSize;
	while (inDataIdx)
	{
		// update data indexes
		inDataIdx-=bpp;
		outDataIdx-=2;
		// get brightness data
		unsigned char brightness=0;
		switch(bpp)
		{
		case 1: // brighteness = alpha
			*(output+outDataIdx) = 
				*(output+outDataIdx+1) = *(data+inDataIdx);
			break;
		case 2:
			// not supported
			break;
		case 3: // brighteness = alpha
			*(output+outDataIdx) = 
				*(output+outDataIdx+1) = (*(data+inDataIdx)+
										 *(data+inDataIdx+1)+
										 *(data+inDataIdx+2))/3; // average all three colors;
			break;
		case 4:
			*(output+outDataIdx) = (*(data+inDataIdx)+
								   *(data+inDataIdx+1)+
								   *(data+inDataIdx+2))/3; // average all three colors;
			*(output+outDataIdx+1) = *(data+inDataIdx+3); // copy alpha value
			break;
		default:
			// error
			break;
		};
		if (minVal)
		{
			if (*(output+outDataIdx)<minVal)
				*(output+outDataIdx)=0;
			if (*(output+outDataIdx+1)<minVal)
				*(output+outDataIdx+1)=0;
		}
	}
	return output;
}

// Initialize
// desc: initializes OpenGL
void Initialize()
{
	glClearColor(0.75f, 0.75f, 0.75f, 1.0f);		// clear to black

	//glPointSize(5.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, g_scrnWidth, 0, g_scrnHeight);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	
	//////////////////////// \/ E D I T   M E \/ ////////////////////////////
	// font that I'm working on
	listBase = CreateBitmapFont("Times New Roman", 24,false,false);
	// font that I'm saving and testing
	SetFontName("Times New Roman", 24,false,false);
	const char * const fPath = "fonts/";
	//////////////////////// /\ E D I T   M E /\ ////////////////////////////

	// load texture
	glEnable(GL_TEXTURE_2D);
		glGenTextures(1,&texID);
		glBindTexture(GL_TEXTURE_2D,texID);
		/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		int width,height,bpp;
		size_t size;
		char buff[1024]={0};
		strcpy(buff,fPath);
		strcat(buff,g_fontName);
		strcat(buff,".jpg");
		unsigned char *data=LoadImageData(buff,&width,&height,&bpp,&size);
		//fonts/Times New Roman.jpg EYE.BMP
		if (data)
		{
			
			{
				pDLImage = new unsigned char[THEWIDTH*THEHEIGHT*4];
				if (pDLImage)
				{
					memset(pDLImage,NULL,THEWIDTH*THEHEIGHT*4);
					for (int y=0; y<THEWIDTH; y++)
						for (int x=0; x<THEWIDTH; x++)
						{
							unsigned char *pixel = ScrollImageData(x,y,THEWIDTH,THEHEIGHT,32,data);
							int a = THEWIDTH*THEHEIGHT*4, b = ((x)+(y)*THEWIDTH)*4+3;
							pDLImage[((x)+(y)*THEWIDTH)*4+3] = *pixel;
						}
				}
			}
			//GLenum format = GL_LUMINANCE_ALPHA;
			unsigned char *pLumAlphaData=CreateGLLuminanceAlpha(size,bpp,data,128);
			if (pLumAlphaData)
			{
				glTexImage2D(GL_TEXTURE_2D,0,2,width,height,0,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,pLumAlphaData);
				//gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE_ALPHA,width,height,GL_LUMINANCE_ALPHA,GL_UNSIGNED_BYTE,pLumAlphaData);
				free(pLumAlphaData);
				pLumAlphaData=NULL;
			}
			/*glTexImage2D(GL_TEXTURE_2D,0,bpp/8,width,height,0,format,GL_UNSIGNED_BYTE,data);
			gluBuild2DMipmaps(GL_TEXTURE_2D,format,width,height,format,GL_UNSIGNED_BYTE,data);*/
			//glTexImage2D(GL_TEXTURE_2D,0,3,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
		}
		{ // search image data
			//CTypeFace typeFace;
			/*typeFace.m_typeWidth = 20; // for times new roman 24
			typeFace.m_typeHeight = 20;*/
			//////////////////////// \/ E D I T   M E \/ ////////////////////////////
			typeFace.m_typeWidth = 19;
			typeFace.m_typeHeight = 20;
			typeFace.m_lineHeight = 5;
			typeFace.m_charSpacing = 1;
			//////////////////////// /\ E D I T   M E /\ ////////////////////////////
			//////////////// N O   M O R E ! ! !
			typeFace.m_texWidth = width;
			typeFace.m_texHeight = height;
			typeFace.m_width = typeFace.m_typeWidth*16;
			typeFace.m_height = typeFace.m_typeHeight*6;

			int currChar=96;
			int currMinX,currMaxX,initialY,initialX,endY,endX;
			// loop throug all chars
			while (currChar--)
			{
				initialY=(currChar/16)*typeFace.m_typeHeight;
				initialX=(currChar%16)*typeFace.m_typeWidth;
				endY=initialY+typeFace.m_typeHeight;
				endX=initialX+typeFace.m_typeWidth;
				currMinX=typeFace.m_width;
				currMaxX=0;
				// scan pixels to calc text width
				for (int y=initialY; y<endY; y++)
					for (int x=initialX; x<endX; x++)
					{
						unsigned char *pixel = ScrollImageData(x,y,width,height,bpp,data);
						if (*pixel>128)
						{
							if (x>currMaxX)
								currMaxX=x;
							if (x<currMinX)
								currMinX=x;
						}
					}
				// scan complete, record text width
				typeFace.m_chars[currChar].m_width=currMaxX-currMinX;
				if (typeFace.m_chars[currChar].m_width<0)
				{
					typeFace.m_chars[currChar].m_width=typeFace.m_typeWidth/4; // space chars = 1/4th max width
					currMaxX=currMinX=0; // set left and right to zero
				}
				else
					typeFace.m_chars[currChar].m_width++; // add 1 to all other widths
				// physical width calc completed

				// calc tex coords
				typeFace.m_chars[currChar].m_left= ((float)currMinX)/((float)typeFace.m_texWidth);
				typeFace.m_chars[currChar].m_right= ( (float)(currMinX+typeFace.m_chars[currChar].m_width) )
													/((float)typeFace.m_texWidth);

				typeFace.m_chars[currChar].m_top= ((float)(initialY))/((float)typeFace.m_texHeight);
				typeFace.m_chars[currChar].m_bottom= ((float)(endY))/((float)typeFace.m_texHeight);

				/*cLog.LPrintf("width: % 3i char: '%c' index: % 3i left: %1.4f right: %1.4f top: %1.4f bottom: %1.4f",typeFace.m_chars[currChar].m_width,currChar+32,currChar,
					typeFace.m_chars[currChar].m_left,
					typeFace.m_chars[currChar].m_right,
					typeFace.m_chars[currChar].m_top,
					typeFace.m_chars[currChar].m_bottom);*/
			}
		}
		free(data);
		data=NULL;
	glDisable(GL_TEXTURE_2D);

	{ //********** Create Another Texture ***********
		// load texture
		int width,height,bpp;
		size_t size;
		unsigned char *data=LoadImageData("EYE.BMP",&width,&height,&bpp,&size);
		GLenum format;
		switch (bpp)
		{
		case 8:
			format = GL_LUMINANCE;
			break;
		case 16:
			// error
			format = GL_LUMINANCE;
			break;
		case 24:
			format = GL_RGB;
			break;
		case 32:
			format = GL_RGBA;
			break;
		default:
			// error
			format = GL_LUMINANCE;
			break;
		}
		// create tex name
		glGenTextures(1,&tex2ID);
		// create texture
		glBindTexture(GL_TEXTURE_2D,tex2ID);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		gluBuild2DMipmaps(GL_TEXTURE_2D,format,width,height,format,GL_UNSIGNED_BYTE,data);
		// free bitmap data
		free(data);
		data = NULL;
	}
	SaveFont(fPath,g_fontName,&typeFace);
	memset(&typeFace,NULL,sizeof(typeFace));
	LoadFont(fPath,g_fontName,&typeFace);

	// Create Display List
	if (pDLImage)
	{
		testList = glGenLists(1);
		glNewList(testList, GL_COMPILE);
			glDrawPixels(THEWIDTH,THEHEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,pDLImage);
		glEndList();
	}

	//ChangeColor(texID,1.0f,0.0f,0.0f);
}

// Render
// desc: handles drawing of scene
void Render()
{
	// clear screen and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
	glLoadIdentity();

	// move one unit into the screen
	glTranslatef(0.0f, 0.0f, -1.0f);
	
	static char string[2]={0};
	register int i=256, c=0,
		cSel=0; // index into xAdjust during update loop
	static int cWidth = 30,
		cHeight = 30,
		xOffset = 10,
		yOffset = 10,
		txtElvt = 0,
		mode = 0,
		selChar = 0,
		xAdjust[96]={0};
	static bool gridIsOn = true;
	{ // input
		/*if (keyPressed[VK_]&&!oldKeyPressed[VK_])
			;*/
		if (keyPressed[VK_UP]&&!oldKeyPressed[VK_UP])
			cHeight++;
		if (keyPressed[VK_DOWN]&&!oldKeyPressed[VK_DOWN])
			cHeight--;
		if (keyPressed[VK_LEFT]&&!oldKeyPressed[VK_LEFT])
			cWidth++;
		if (keyPressed[VK_RIGHT]&&!oldKeyPressed[VK_RIGHT])
			cWidth--;
		if (keyPressed[VK_PRIOR]&&!oldKeyPressed[VK_PRIOR])
			txtElvt++;
		if (keyPressed[VK_NEXT]&&!oldKeyPressed[VK_NEXT])
			txtElvt--;
		if (keyPressed[VK_SPACE]&&!oldKeyPressed[VK_SPACE])
			gridIsOn = (!gridIsOn);

		if (keyPressed[VK_X]&&!oldKeyPressed[VK_X])
		{
			selChar++;
			if (selChar>95)
				selChar=0;
		}
		if (keyPressed[VK_Z]&&!oldKeyPressed[VK_Z])
		{
			selChar--;
			if (selChar<0)
				selChar=95;
		}
		if (keyPressed[VK_V]&&!oldKeyPressed[VK_V])
			xAdjust[selChar]++;
		if (keyPressed[VK_C]&&!oldKeyPressed[VK_C])
			xAdjust[selChar]--;


		if (keyPressed[VK_NUMPAD0]&&!oldKeyPressed[VK_NUMPAD0])
		{
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
			mode = 0;
		}
		if (keyPressed[VK_NUMPAD1]&&!oldKeyPressed[VK_NUMPAD1])
		{
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			mode = 1;
		}
		if (keyPressed[VK_NUMPAD2]&&!oldKeyPressed[VK_NUMPAD2])
		{
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			mode = 2;
		}

		memcpy(oldKeyPressed,keyPressed,sizeof(keyPressed));
	}

	switch(mode)
	{
	case 0: // create font mode
		{
			glPushMatrix();
			glColor3f(0.0f,0.0f,0.0f);
				glBegin(GL_QUADS);
					glVertex2i(xOffset, g_scrnHeight-yOffset-2*cHeight);
					glVertex2i(xOffset, g_scrnHeight-(yOffset+(8)*cHeight));
					glVertex2i(xOffset+(256/16)*cWidth, g_scrnHeight-(yOffset+(8)*cHeight));
					glVertex2i(xOffset+(256/16)*cWidth, g_scrnHeight-yOffset-2*cHeight);
				glEnd();
			glPopMatrix();
			glColor3f(1.0f, 1.0f, 1.0f);

			while (i--)
			{
				c++;
				if (c>16&&gridIsOn)
				{
					c=0;
					glBegin(GL_LINES);
						if (i>31 && i<128)
						{
							glVertex2i(xOffset,g_scrnHeight-(yOffset+(1+(i/16))*cHeight));
							glVertex2i(xOffset+(256/16)*cWidth,g_scrnHeight-(yOffset+(1+(i/16))*cHeight));
						}
						glVertex2i(xOffset+(i%16)*cWidth,g_scrnHeight-yOffset-2*cHeight);
						glVertex2i(xOffset+(i%16)*cWidth,g_scrnHeight-(yOffset+(8)*cHeight));
					glEnd();
				}
				if (i>31&&i<128)
					cSel=i-32;
				string[0]=(char)i;
				glRasterPos2i(xAdjust[cSel]+xOffset+(i%16)*cWidth,g_scrnHeight-(yOffset+(1+(i/16))*cHeight)+txtElvt);
				PrintString(listBase, string);
			}
			static char buff[100]={0};
			glRasterPos2i(xOffset,g_scrnHeight-(yOffset+cHeight)+txtElvt);
			sprintf(buff,"C W: %i C H: %i O W: %i O H: %i",cWidth,cHeight,
				cWidth*16,cHeight*16);
			PrintString(listBase, buff);
			glRasterPos2i(xOffset,g_scrnHeight-(yOffset+2*cHeight)+txtElvt);
			sprintf(buff,"Txt el: %i SelChar:%c X-Adjust: %i",txtElvt,selChar+32,xAdjust[selChar]);
			PrintString(listBase, buff);
		}
		break;

	case 1:
		{
			glColor3f(0.0f,0.0f,0.0f);
			glBindTexture(GL_TEXTURE_2D,texID);
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f);
				glVertex2i(xOffset, g_scrnHeight-yOffset-2*cHeight);
				glTexCoord2f(0.0f, 1.0f);
				glVertex2i(xOffset, g_scrnHeight-(yOffset+(8)*cHeight));
				glTexCoord2f(1.0f, 1.0f);
				glVertex2i(xOffset+(16)*cWidth, g_scrnHeight-(yOffset+(8)*cHeight));
				glTexCoord2f(1.0f, 0.0f);
				glVertex2i(xOffset+(16)*cWidth, g_scrnHeight-yOffset-2*cHeight);
			glEnd();

			glBindTexture(GL_TEXTURE_2D,tex2ID);
			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f);
				glVertex2i(xOffset, 0);
				glTexCoord2f(0.0f, 1.0f);
				glVertex2i(xOffset, cHeight*2);
				glTexCoord2f(1.0f, 1.0f);
				glVertex2i(xOffset+cHeight*2, cHeight*2);
				glTexCoord2f(1.0f, 0.0f);
				glVertex2i(xOffset+cHeight*2, 0);
			glEnd();
		}
		break;
	case 2:
		{
			{
				if (glIsList(testList)==GL_TRUE)
				{
					glRasterPos2i(200,200);
					glCallList(testList);
				}
			}
			glBindTexture(GL_TEXTURE_2D,texID);
			static const char * const text = "Test of the newly created font. 5>2 1<10 3=3\n"
				"Across Multiple lines of text. It'd be great if this just works.\nThis for example, should be "
				"on a new line.\nAnd this should be\ttabbed|||d@|.";
			static int width,height,x=0,y=g_scrnHeight/2;
			if (gridIsOn)
			{
				glDisable(GL_TEXTURE_2D);
				glColor3f(0.0f,0.0f,0.0f);
				typeFace.CalcPrintArea(width,height,text,true);
				glBegin(GL_QUADS);
					glVertex2i(x,y);
					glVertex2i(x,y-height);
					glVertex2i(x+width,y-height);
					glVertex2i(x+width,y);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}

			typeFace.PrintF(x,y,text,true);
			
			if (gridIsOn)
			{
				glDisable(GL_TEXTURE_2D);
				glColor3f(1.0f,0.0f,0.0f);
				static int top,left,right,bottom;
				typeFace.CalcCharCoords(left,top,right,bottom,txtElvt,text,true);
				glBegin(GL_QUADS);
					glVertex2i(x+left,y+top);
					glVertex2i(x+left,y+bottom);
					glVertex2i(x+right,y+bottom);
					glVertex2i(x+right,y+top);
				glEnd();
				static char buff[100]={0};
				int cx=cWidth+x,cy=-cHeight+y,
					i=typeFace.GetSelectedChar(cx-x,cy-y,text,true);
				int rows,cols;
				typeFace.EstCharSpace(rows,cols,g_scrnWidth,g_scrnHeight);
				sprintf(buff,"x:%03i y:%03i %i %c MaxRows:%i MaxCols:%i bx1:%i by1:%i bx2:%i by2:%i",
					cx,cy,i,text[i],rows,cols,x+left,y+top,x+right,y+bottom);
				glBegin(GL_POINTS);
					glVertex2i(cx,cy);
				glEnd();
				glBegin(GL_LINES);
					glVertex2i(0,y-typeFace.m_typeHeight+typeFace.m_lineHeight-1);
					glVertex2i(g_scrnWidth,y-typeFace.m_typeHeight+typeFace.m_lineHeight-1);
				glEnd();
				glEnable(GL_TEXTURE_2D);
				typeFace.Print(2,80,buff);
			}
			switch (selChar%7)
			{
			case 0:
				typeFace.Print(2,40,"~}|{zyxwvutsrqpo");
				break;
			case 1:
				typeFace.Print(2,40,"nmlkjihgfedcba`_");
				break;
			case 2:
				typeFace.Print(2,40,"^]\\[ZYXWVUTSRQPO");
				break;
			case 3:
				typeFace.Print(2,40,"NMLKJIHGFEDCBA@?");
				break;
			case 4:
				typeFace.Print(2,40,">=<;:9876543210/");
				break;
			case 5:
				typeFace.Print(2,40,".-,+*)('&%$#\"! ");
				break;
			default:
				typeFace.Print(2,40,"End.");
				break;
			};
		}
	default:
		break;
	}
	glFlush();
	SwapBuffers(g_HDC);			// bring backbuffer to foreground
}

// function to set the pixel format for the device context
void SetupPixelFormat(HDC hDC)
{
	int nPixelFormat;					// our pixel format index

	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// size of structure
		1,								// default version
		PFD_DRAW_TO_WINDOW |			// window drawing support
		PFD_SUPPORT_OPENGL |			// OpenGL support
		PFD_DOUBLEBUFFER,				// double buffering support
		PFD_TYPE_RGBA,					// RGBA color mode
		16,								// 32 bit color mode
		0, 0, 0, 0, 0, 0,				// ignore color bits, non-palettized mode
		0,								// no alpha buffer
		0,								// ignore shift bit
		0,								// no accumulation buffer
		0, 0, 0, 0,						// ignore accumulation bits
		16,								// 16 bit z-buffer size
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main drawing plane
		0,								// reserved
		0, 0, 0 };						// layer masks ignored

	nPixelFormat = ChoosePixelFormat(hDC, &pfd);	// choose best matching pixel format

	SetPixelFormat(hDC, nPixelFormat, &pfd);		// set pixel format to device context
}

// the Windows Procedure event handler
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HGLRC hRC;					// rendering context
	static HDC hDC;						// device context
	int width, height;					// window width and height

	switch(message)
	{
		case WM_CREATE:					// window is being created

			hDC = GetDC(hwnd);			// get current window's device context
			g_HDC = hDC;
			SetupPixelFormat(hDC);		// call our pixel format setup function

			// create rendering context and make it current
			hRC = wglCreateContext(hDC);
			wglMakeCurrent(hDC, hRC);

			return 0;
			break;

		case WM_CLOSE:					// windows is closing

			// deselect rendering context and delete it
			wglMakeCurrent(hDC, NULL);
			wglDeleteContext(hRC);

			// send WM_QUIT to message queue
			PostQuitMessage(0);

			return 0;
			break;

		case WM_SIZE:
			height = HIWORD(lParam);		// retrieve width and height
			width = LOWORD(lParam);

			if (height==0)					// don't want a divide by zero
			{
				height=1;					
			}

			glViewport(0, 0, width, height);	// reset the viewport to new dimensions
			glMatrixMode(GL_PROJECTION);		// set projection matrix current matrix
			glLoadIdentity();					// reset projection matrix

			// calculate aspect ratio of window
			gluPerspective(54.0f,(GLfloat)width/(GLfloat)height,1.0f,1000.0f);

			glMatrixMode(GL_MODELVIEW);			// set modelview matrix
			glLoadIdentity();					// reset modelview matrix

			return 0;
			break;

		case WM_KEYDOWN:					// is a key pressed?
			keyPressed[wParam] = true;
			return 0;
			break;

		case WM_KEYUP:
			keyPressed[wParam] = false;
			return 0;
			break;

		default:
			break;
	}

	return (DefWindowProc(hwnd, message, wParam, lParam));
}

// the main windows entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEX windowClass;		// window class
	HWND	   hwnd;			// window handle
	MSG		   msg;				// message
	bool	   done;			// flag saying when our app is complete
	DWORD	   dwExStyle;		// Window Extended Style
	DWORD	   dwStyle;			// Window Style
	RECT	   windowRect;

	// temp var's
	int width = g_scrnWidth;
	int height = g_scrnHeight;
	int bits = g_scrnBpp;

	//fullScreen = TRUE;

	windowRect.left=(long)0;						// Set Left Value To 0
	windowRect.right=(long)width;					// Set Right Value To Requested Width
	windowRect.top=(long)0;							// Set Top Value To 0
	windowRect.bottom=(long)height;					// Set Bottom Value To Requested Height

	// fill out the window class structure
	windowClass.cbSize			= sizeof(WNDCLASSEX);
	windowClass.style			= CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc		= WndProc;
	windowClass.cbClsExtra		= 0;
	windowClass.cbWndExtra		= 0;
	windowClass.hInstance		= hInstance;
	windowClass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);	// default icon
	windowClass.hCursor			= LoadCursor(NULL, IDC_ARROW);		// default arrow
	windowClass.hbrBackground	= NULL;								// don't need background
	windowClass.lpszMenuName	= NULL;								// no menu
	windowClass.lpszClassName	= "MyClass";
	windowClass.hIconSm			= LoadIcon(NULL, IDI_WINLOGO);		// windows logo small icon

	// register the windows class
	if (!RegisterClassEx(&windowClass))
		return 0;

	if (fullScreen)								// fullscreen?
	{
		DEVMODE dmScreenSettings;					// device mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);	
		dmScreenSettings.dmPelsWidth = width;		// screen width
		dmScreenSettings.dmPelsHeight = height;		// screen height
		dmScreenSettings.dmBitsPerPel = bits;		// bits per pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// 
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// setting display mode failed, switch to windowed
			MessageBox(NULL, "Display mode failed", NULL, MB_OK);
			fullScreen=FALSE;	
		}
	}

	if (fullScreen)								// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;				// Window Extended Style
		dwStyle=WS_POPUP;						// Windows Style
		ShowCursor(FALSE);						// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;					// Windows Style
	}

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// class registered, so now create our window
	hwnd = CreateWindowEx(NULL,									// extended style
						  "MyClass",							// class name
						  "Displaying Text Example 1: Bitmap Fonts", // app name
						  dwStyle | WS_CLIPCHILDREN |
						  WS_CLIPSIBLINGS,
						  0, 0,									// x,y coordinate
						  windowRect.right - windowRect.left,
						  windowRect.bottom - windowRect.top,	// width, height
						  NULL,									// handle to parent
						  NULL,									// handle to menu
						  hInstance,							// application instance
						  NULL);								// no extra params

	// check if window creation failed (hwnd would equal NULL)
	if (!hwnd)
		return 0;

	ShowWindow(hwnd, SW_SHOW);			// display the window
	UpdateWindow(hwnd);					// update the window

	done = false;						// intialize the loop condition variable
	Initialize();						// initialize OpenGL

	// main message loop
	while (!done)
	{
		PeekMessage(&msg, hwnd, NULL, NULL, PM_REMOVE);

		if (msg.message == WM_QUIT)		// do we receive a WM_QUIT message?
		{
			done = true;				// if so, time to quit the application
		}
		else
		{
			if (keyPressed[VK_ESCAPE])
				done = true;
			else
			{
				Render();

				TranslateMessage(&msg);		// translate and dispatch to event queue
				DispatchMessage(&msg);
			}
		}
	}

	CleanUp();

	if (fullScreen)
	{
		ChangeDisplaySettings(NULL,0);		// If So Switch Back To The Desktop
		ShowCursor(TRUE);					// Show Mouse Pointer
	}
	
	return msg.wParam;
}
